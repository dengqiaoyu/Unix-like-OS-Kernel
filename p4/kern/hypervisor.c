/**
 * @file hypervisor.c
 * @author Newton Xie (ncx)
 * @author Qiaoyu Deng (qdeng)
 * @bug No known bugs
 */

/* libc include */
#include <stdlib.h>
#include <string.h>

#include <ureg.h>                       /* ureg_t */
#include <distorm/disassemble.h>        /* MAX_INS_LENGTH, disassemble */
#include <common_kern.h>                /* USER_MEM_START */
#include <cr.h>                         /* set_crX */
#include <multiboot.h>

/* x86 include */
#include <x86/seg.h>                    /* CS, DS */
#include <x86/interrupt_defines.h>      /*  */
#include <x86/timer_defines.h>
#include <x86/video_defines.h>

#include <simics.h>                     /* debug */

/* user include */
#include "hypervisor.h"
#include "task.h"                       /* thread_t */
#include "scheduler.h"                  /* get_cur_tcb() */
#include "vm.h"                         /* read_guest */
#include "console.h"                    /* set_cursor */
#include "asm_kern_to_user.h"

/* internal function */
static int _simulate_instr(char *instr, ureg_t *ureg);
static int _timer_init(ureg_t *ureg);
static int _set_cursor(ureg_t *ureg);

extern uint64_t init_gdt[GDT_SEGS];

void hypervisor_init() {
    init_gdt[SEGSEL_SPARE0_IDX] = SEGDES_GUEST_CS;
    init_gdt[SEGSEL_SPARE1_IDX] = SEGDES_GUEST_DS;
}

int guest_init(simple_elf_t *header) {
    thread_t *thread = get_cur_tcb();
    task_t *task = thread->task;

    task->guest_info = guest_info_init();
    if (task->guest_info == NULL) return -1;

    uint32_t addr = USER_MEM_START;
    int pte_flags = PTE_USER | PTE_WRITE | PTE_PRESENT;

    if (dec_num_free_frames(GUEST_FRAMES) < 0) return -1;
    int i;
    for (i = 0; i < GUEST_FRAMES; i++) {
        uint32_t frame_addr = get_frame();
        if (set_pte(addr, frame_addr, pte_flags) < 0) {
            inc_num_free_frames(GUEST_FRAMES);
            // freeing of resources is deferred to caller
            return -1;
        }
        addr += PAGE_SIZE;
    }

    int ret = 0;
    /* load text */
    ret = load_guest_section(header->e_fname,
                             header->e_txtstart,
                             header->e_txtlen,
                             header->e_txtoff);
    if (ret < 0) return -1;

    /* load dat */
    ret = load_guest_section(header->e_fname,
                             header->e_datstart,
                             header->e_datlen,
                             header->e_datoff);
    if (ret < 0) return -1;

    /* load rodat */
    ret = load_guest_section(header->e_fname,
                             header->e_rodatstart,
                             header->e_rodatlen,
                             header->e_rodatoff);
    if (ret < 0) return -1;

    /* bss is already "loaded" */

    // TODO macro multiboot numbers
    struct multiboot_info *mb_info = (void *)0x15410;
    memset(mb_info, 0, sizeof(struct multiboot_info));
    mb_info->flags = MULTIBOOT_MEMORY;
    mb_info->mem_lower = 637;
    mb_info->mem_upper = GUEST_MEM_SIZE / 1024 - 1024;

    /* 
     * We set the guest console memory region to read only so that we can
     * detect console writes. The code below relies on CONSOLE_MEM_BASE being
     * page-aligned and CONSOLE_MEM_SIZE fitting within a single page.
     */
    uint32_t guest_console_mem = USER_MEM_START + CONSOLE_MEM_BASE;
    uint32_t frame = get_pte(guest_console_mem) & PAGE_ALIGN_MASK;
    // this set_pte call will not fail
    set_pte(guest_console_mem, frame, PTE_USER | PTE_PRESENT);

    backup_main_console();
    clear_console();

    // update fname for simics symbolic debugging
    sim_reg_process(task->page_dir, header->e_fname);

    set_esp0(thread->kern_sp);
    kern_to_guest(header->e_entry);

    return 0;
}

int load_guest_section(const char *fname, unsigned long start,
                       unsigned long len, unsigned long offset) {
    uint32_t high = (uint32_t)(start + len);
    if (high >= GUEST_MEM_SIZE) return -1;

    char *linear_start = (char *)start + USER_MEM_START;
    getbytes(fname, offset, len, linear_start);
    return 0;
}

guest_info_t *guest_info_init() {
    guest_info_t *guest_info = calloc(1, sizeof(guest_info_t));
    if (guest_info == NULL) return NULL;

    guest_info->io_ack_flag = 0;
    /* virtual timer */
    guest_info->timer_init_stat = TIMER_UNINT;
    guest_info->timer_init_stat = 0;

    guest_info->cursor_data = SIGNAL_CURSOR_NORMAL;
    guest_info->cursor_idx = 0;

    guest_info->console_state = NULL;

    return guest_info;
}

void guest_info_destroy(guest_info_t *guest_info) {
    //TODO
    free(guest_info);
}

int handle_sensi_instr(ureg_t *ureg) {
    thread_t *thread = get_cur_tcb();

    uint32_t *kern_sp = (uint32_t *)(thread->kern_sp);
    uint32_t cs_value = *((uint32_t *)(kern_sp - 4));
    uint32_t eip_value = *((uint32_t *)(kern_sp - 5));
    lprintf("cs_value: %p, eip_value: %p", (void *)cs_value, (void *)eip_value);

    void *fault_ip = (void *)ureg->eip;
    char instr_buf[MAX_INS_LENGTH + 1] = {0};
    char instr_buf_decoded[MAX_INS_DECODED_LENGTH + 1] = {0};
    lprintf("fault_ip: %p", fault_ip);

    /* BUG just for test not SEGSEL_KERNEL_DS ! */
    read_guest(instr_buf, (uint32_t)fault_ip, MAX_INS_LENGTH, SEGSEL_KERNEL_DS);
    int eip_offset = disassemble(instr_buf, MAX_INS_LENGTH,
                                 instr_buf_decoded, MAX_INS_LENGTH + 1);
    lprintf("eip_offset: %d, instruction: %s",
            eip_offset, instr_buf_decoded);

    // MAGIC_BREAK;
    int ret = _simulate_instr(instr_buf_decoded, ureg);
    if (ret != 0) return -1;
    *((uint32_t *)(kern_sp - 5)) = eip_value + eip_offset;
    eip_value = *((uint32_t *)(kern_sp - 5));
    lprintf("eip_value: %p", (void *)eip_value);

    MAGIC_BREAK;
    return 0;
}

int _simulate_instr(char *instr, ureg_t *ureg) {
    guest_info_t *guest_info = get_cur_tcb()->task->guest_info;
    // void *handler_addr = NULL;

    /* begin to compare supported sensitive instruction */
    int ret = 0;
    if (strcmp(instr, "OUT DX, AX") == 0) {
        uint16_t outb_param1 = ureg->edx;
        uint8_t outb_param2 = ureg->eax;
        lprintf("outb_param1: %x, outb_param2: %x", outb_param1, outb_param2);

        if (outb_param1 == TIMER_MODE_IO_PORT
                || outb_param1 == TIMER_PERIOD_IO_PORT) {
            /* guest_info set timer */
            ret = _timer_init(ureg);
            if (ret == 0) return 0;
            return 0;
        }
        else if (outb_param1 == INT_ACK_CURRENT) {
            /* guest_info ack device interrupt */
            guest_info->io_ack_flag = 1;
            return 0;
        }
        else if (outb_param1 == CRTC_IDX_REG || ureg->eax == CRTC_DATA_REG) {
            /* set cursor in the console */
            ret = _set_cursor(ureg);
            if (ret == 0) return 0;
        }
    }
    return -1;
}

int _timer_init(ureg_t *ureg) {
    uint16_t outb_param1 = ureg->edx;
    uint8_t outb_param2 = ureg->eax;

    guest_info_t *guest_info = get_cur_tcb()->task->guest_info;
    switch (guest_info->timer_init_stat) {
        case TIMER_UNINT:
            if (outb_param1 == TIMER_MODE_IO_PORT)
                guest_info->timer_init_stat = TIMER_FREQUENCY_PART1;
            else /* invalid timer init process */
                return -1;
            break;

        case TIMER_FREQUENCY_PART1:
            if (outb_param1 == TIMER_PERIOD_IO_PORT)
                guest_info->timer_init_stat = TIMER_FREQUENCY_PART2;
            else /* invalid timer init process */
                return -1;
            guest_info->timer_interval = outb_param2;
            break;

        case TIMER_FREQUENCY_PART2:
            if (outb_param1 == TIMER_PERIOD_IO_PORT)
                guest_info->timer_init_stat = TIMER_INTED;
            else /* invalid timer init process */
                return -1;
            guest_info->timer_interval += (outb_param2) << 8;
            guest_info->timer_interval =
                MS_PER_S / (TIMER_RATE / guest_info->timer_interval);
            lprintf("guest_info->timer_interval: %d", (int)guest_info->timer_interval);
            break;

        default:
            return -1;
    }

    return 0;
}

int _set_cursor(ureg_t *ureg) {
    guest_info_t *guest_info = get_cur_tcb()->task->guest_info;

    switch (guest_info->cursor_data) {
        case SIGNAL_CURSOR_NORMAL:
            if (ureg->edx == CRTC_IDX_REG && ureg->eax == CRTC_CURSOR_LSB_IDX) {
                guest_info->cursor_data = SIGNAL_CURSOR_LSB_IDX;
            }
            else if (ureg->edx == CRTC_IDX_REG
                    && ureg->eax == CRTC_CURSOR_MSB_IDX) {
                guest_info->cursor_data = SIGNAL_CURSOR_MSB_IDX;
            }
            else return -1;
            break;

        case SIGNAL_CURSOR_LSB_IDX:
            if (ureg->edx == CRTC_DATA_REG) {
                guest_info->cursor_data = SIGNAL_CURSOR_NORMAL;
                guest_info->cursor_idx = ureg->eax;
            }
            else return -1;
            break;

        case SIGNAL_CURSOR_MSB_IDX:
            if (ureg->edx == CRTC_DATA_REG) {
                guest_info->cursor_data = SIGNAL_CURSOR_NORMAL;
                guest_info->cursor_idx += ureg->eax << 8;
                /* begin set cursor */
                int row = guest_info->cursor_idx / CONSOLE_WIDTH;
                int col = guest_info->cursor_idx % CONSOLE_WIDTH;
                set_cursor(row, col);
            }
            else return -1;
            break;
    }

    return 0;
}
