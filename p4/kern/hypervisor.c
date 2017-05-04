/**
 * @file hypervisor.c
 * @author Newton Xie (ncx)
 * @author Qiaoyu Deng (qdeng)
 * @bug No known bugs
 */

/* libc include */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <ureg.h>                       /* ureg_t */
#include <distorm/disassemble.h>        /* MAX_INS_LENGTH, disassemble */
#include <common_kern.h>                /* USER_MEM_START */
#include <cr.h>                         /* set_crX */
#include <multiboot.h>
#include <assert.h>

/* x86 include */
#include <x86/seg.h>                    /* CS, DS */
#include <x86/interrupt_defines.h>      /*  */
#include <x86/timer_defines.h>          /* TIMER_IDT_ENTRY */
#include <x86/video_defines.h>
#include <x86/asm.h>                    /* disable_interrupts() */
#include <keyhelp.h>                    /* KEY_IDT_ENTRY */

#include <simics.h>                     /* debug */

/* user include */
#include "syscalls/syscalls.h"          /* kern_vanish */
#include "hypervisor.h"
#include "drivers/timer_driver.h"
#include "task.h"                       /* thread_t */
#include "scheduler.h"                  /* get_cur_tcb() */
#include "vm.h"                         /* read_guest */
#include "console.h"                    /* set_cursor */
#include "asm_kern_to_user.h"
#include "asm_page_inval.h"

/* internal function */
static int _simulate_instr(char *instr, ureg_t *ureg);

static int _handle_out(guest_info_t *guest_info, ureg_t *ureg);
static void _handle_cli(guest_info_t *guest_info);
static void _handle_sti(guest_info_t *guest_info);
static int _handle_in(guest_info_t *guest_info, ureg_t *ureg);
static void _handle_hlt(guest_info_t *guest_info, ureg_t *ureg);
static int _handle_jmp_far(guest_info_t *guest_info, char *instr);
static void _handle_iret(guest_info_t *guest_info, ureg_t *ureg);

/* used by _handle_out */
static int _handle_timer_init(ureg_t *ureg);
static int _handle_set_cursor(ureg_t *ureg);
static void _handle_int_ack(guest_info_t *guest_info);

/* debug */
void _exn_print_ureg(ureg_t *ureg);

/* used by set_user_handler */
static uint32_t _get_handler_addr(int idt_idx);
static uint32_t _get_descriptor_base_addr(uint16_t seg_sel);

extern uint64_t init_gdt[GDT_SEGS];
guest_info_t *cur_guest_info;

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
    struct multiboot_info *mb_info = (void *)(0x15410 + USER_MEM_START);
    memset(mb_info, 0, sizeof(struct multiboot_info));
    mb_info->flags = MULTIBOOT_MEMORY;
    mb_info->mem_lower = 637;
    mb_info->mem_upper = GUEST_MEM_SIZE / 1024 - 1024;

    /*
     * We set the guest console memory region to read only so that we can
     * detect console writes. The code below relies on CONSOLE_MEM_BASE being
     * page-aligned and CONSOLE_MEM_SIZE fitting within a single page.
     */
    uint32_t frame = get_pte(GUEST_CONSOLE_BASE) & PAGE_ALIGN_MASK;
    // this set_pte call will not fail
    set_pte(GUEST_CONSOLE_BASE, frame, PTE_USER | PTE_PRESENT);

    if (backup_main_console() < 0) return -1;
    clear_console();

    // update fname for simics symbolic debugging
    sim_reg_process(task->page_dir, header->e_fname);

    set_esp0(thread->kern_sp);

    disable_interrupts();
    cur_guest_info = thread->task->guest_info;
    // MAGIC_BREAK;
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

    guest_info->pic_ack_flag = TIMER_NOT_ACKED;
    guest_info->inter_en_flag = DISABLED;

    /* virtual timer */
    guest_info->timer_init_stat = TIMER_UNINT;
    guest_info->internal_ticks = 0;
    guest_info->timer_interval = 0;

    /* virtual keyboard */
    memset(guest_info->keycode_buf, 0, KC_BUF_LEN);
    guest_info->buf_start = 0;
    guest_info->buf_end = 0;

    /* virtual console registers */
    guest_info->cursor_data = SIGNAL_CURSOR_NORMAL;
    guest_info->cursor_idx = 0;

    guest_info->console_state = NULL;

    return guest_info;
}

void guest_info_destroy(guest_info_t *guest_info) {
    //TODO
    free(guest_info);
}

void _mirror_console(uint32_t *addr) {
    uint32_t offset = (uint32_t)addr & ~PAGE_ALIGN_MASK;
    if (offset < CONSOLE_MEM_SIZE) {
        *(uint32_t *)(CONSOLE_MEM_BASE + offset) = *addr;
    }
}

int guest_console_mov(uint32_t *dest, ureg_t *ureg) {
    thread_t *thread = get_cur_tcb();
    int ret = 0;
    uint32_t *kern_sp = (uint32_t *)(thread->kern_sp);
    uint32_t eip = *(kern_sp - 5);

    char instr_buf[MAX_INSTR_LENGTH + 1] = {0};
    char decoded_buf[MAX_DECODED_LENGTH + 1] = {0};

    read_guest(instr_buf, eip, MAX_INSTR_LENGTH, SEGSEL_GUEST_DS);

    int eip_offset = disassemble(instr_buf, MAX_INSTR_LENGTH,
                                 decoded_buf, MAX_DECODED_LENGTH);

    uint32_t frame = get_pte(GUEST_CONSOLE_BASE) & PAGE_ALIGN_MASK;
    asm_page_inval((void *)GUEST_CONSOLE_BASE);
    // this set_pte call will not fail
    set_pte(GUEST_CONSOLE_BASE, frame, PTE_USER | PTE_WRITE | PTE_PRESENT);

    char one[] = "MOV BYTE [EAX], 0x";
    int lenone = strlen(one);
    char two[] = "MOV [EAX], 0x";
    int lentwo = strlen(two);
    char three[] = "MOV [EAX], DL";
    int lenthree = strlen(three);

    if (strncmp(decoded_buf, one, lenone) == 0) {
        int val = 0;
        sscanf(decoded_buf, "MOV BYTE [EAX], 0x%x", &val);
        *dest = val;
    } else if (strncmp(decoded_buf, two, lentwo) == 0) {
        int val = 0;
        sscanf(decoded_buf, "MOV [EAX], 0x%x", &val);
        *dest = val;
    } else if (strncmp(decoded_buf, three, lenthree) == 0) {
        *dest = ureg->edx & 0xff;
    } else {
        lprintf(decoded_buf);
        MAGIC_BREAK;
        ret = -1;
    }

    if (ret == 0) _mirror_console(dest);

    /* need to set new return address */
    if (ret == 0) *((uint32_t *)(kern_sp - 5)) = eip + eip_offset;

    // this set_pte call will not fail
    set_pte(GUEST_CONSOLE_BASE, frame, PTE_USER | PTE_PRESENT);
    asm_page_inval((void *)GUEST_CONSOLE_BASE);

    return ret;
}

int handle_sensi_instr(ureg_t *ureg) {
    thread_t *thread = get_cur_tcb();

    uint32_t *kern_sp = (uint32_t *)(thread->kern_sp);
    uint32_t eip = ureg->eip;

    char instr_buf[MAX_INSTR_LENGTH + 1] = {0};
    char decoded_buf[MAX_DECODED_LENGTH + 1] = {0};

    read_guest(instr_buf, eip, MAX_INSTR_LENGTH, SEGSEL_GUEST_DS);

    int eip_offset = disassemble(instr_buf, MAX_INSTR_LENGTH,
                                 decoded_buf, MAX_DECODED_LENGTH);

    // TODO why doesn't fac3 get recognized??
    char fac3[] = {0xfa, 0xc3};
    if (memcmp(fac3, instr_buf, 2) == 0) {
        /*
        lprintf("instr_buf: 0x%x", *(unsigned int *)instr_buf);
        lprintf("eip: %x, eip_offset: %d, decoded_buf: %s",
                (unsigned int)eip, eip_offset, decoded_buf);
        MAGIC_BREAK;
        */

        sprintf(decoded_buf, "CLI ");
        eip_offset = 1;
    }

    /*
    lprintf("eip: %x, eip_offset: %d, instruction: %s",
            (unsigned int)eip, eip_offset, decoded_buf);
            */

    /* need to set new return address first, because call handler need new ip */
    *((uint32_t *)(kern_sp - 5)) = eip + eip_offset;
    int ret = _simulate_instr(decoded_buf, ureg);
    if (ret != 0) {
        /* Not a supported simulated instruction, restore original eip */
        *((uint32_t *)(kern_sp - 5)) = eip;
        return -1;
    }
    // lprintf("line 228 in hypervisor");
    return 0;
}

int _simulate_instr(char *instr, ureg_t *ureg) {
    guest_info_t *guest_info = get_cur_tcb()->task->guest_info;
    // void *handler_addr = NULL;

    // TODO Need to reorder, jmp far/ long jmp
    int ret = 0;
    if (strcmp(instr, "OUT DX, AL") == 0) {
        // ret = _handle_out(guest_info, ureg);
        // if (ret == 0) return 0;
        // return 0;
        _handle_out(guest_info, ureg);
        return 0;
    } else if (strncmp(instr, "CLI", strlen("CLI")) == 0) {
        _handle_cli(guest_info);
        return 0;
    } else if (strncmp(instr, "STI", strlen("STI")) == 0) {
        _handle_sti(guest_info);
        return 0;
    } else if (strcmp(instr, "IN AL, DX") == 0) {
        // MAGIC_BREAK;
        // ret = _handle_in(guest_info, ureg);
        // if (ret == 0) return 0;
        _handle_in(guest_info, ureg);
        return 0;
    } else if (strncmp(instr, "LGDT", strlen("LGDT")) == 0) {
        return 0;
    } else if (strncmp(instr, "LIDT", strlen("LIDT")) == 0) {
        return 0;
    } else if (strncmp(instr, "LTR", strlen("LTR")) == 0) {
        return 0;
    } else if (strncmp(instr, "HLT", strlen("HLT")) == 0) {
        _handle_hlt(guest_info, ureg);
        return 0;
    } else if (strncmp(instr, "JMP FAR", strlen("JMP FAR")) == 0) {
        ret = _handle_jmp_far(guest_info, instr);
        if (ret == 0) return 0;
    } else if (strncmp(instr, "MOV", strlen("MOV")) == 0) {
        return 0;
    } else if (strncmp(instr, "IRET", strlen("IRET")) == 0) {
        _handle_iret(guest_info, ureg);
        return 0;
    } else {
        MAGIC_BREAK;
    }

    return -1;
}

int _handle_out(guest_info_t *guest_info, ureg_t *ureg) {
    int ret = 0;
    uint16_t outb_param1 = ureg->edx;
    uint8_t outb_param2 = ureg->eax;

    // lprintf("outb_param1: %x, outb_param2: %x", outb_param1, outb_param2);

    if (outb_param1 == TIMER_MODE_IO_PORT
            || outb_param1 == TIMER_PERIOD_IO_PORT) {
        /* guest_info set timer */
        ret = _handle_timer_init(ureg);
        if (ret == 0) return 0;
    } else if (outb_param1 == INT_ACK_CURRENT && outb_param2 == INT_CTL_PORT) {
        /* guest_info ack device interrupt */
        lprintf("handling out");
        _handle_int_ack(guest_info);
        return 0;
    } else if (outb_param1 == CRTC_IDX_REG || ureg->eax == CRTC_DATA_REG) {
        /* set cursor in the console */
        ret = _handle_set_cursor(ureg);
        if (ret == 0) return 0;
    }

    return -1;
}

int _handle_timer_init(ureg_t *ureg) {
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
        if (outb_param1 == TIMER_PERIOD_IO_PORT) {
            guest_info->pic_ack_flag = ACKED;
            guest_info->timer_init_stat = TIMER_INTED;
        } else /* invalid timer init process */
            return -1;
        guest_info->timer_interval += (outb_param2) << 8;
        guest_info->timer_interval =
            MS_PER_S / (TIMER_RATE / guest_info->timer_interval);
        if (guest_info->timer_interval / MS_PER_INTERRUPT < 1) return -1;
        lprintf("guest_info->timer_interval: %d", (int)guest_info->timer_interval);
        break;

    default:
        return -1;
    }

    return 0;
}

int _handle_set_cursor(ureg_t *ureg) {
    guest_info_t *guest_info = get_cur_tcb()->task->guest_info;

    uint16_t outb_param1 = ureg->edx;
    uint8_t outb_param2 = ureg->eax;

    switch (guest_info->cursor_data) {
    case SIGNAL_CURSOR_NORMAL:
        if (outb_param1 == CRTC_IDX_REG && ureg->eax == CRTC_CURSOR_LSB_IDX) {
            guest_info->cursor_data = SIGNAL_CURSOR_LSB_IDX;
        } else if (outb_param1 == CRTC_IDX_REG
                   && outb_param2 == CRTC_CURSOR_MSB_IDX) {
            guest_info->cursor_data = SIGNAL_CURSOR_MSB_IDX;
        } else return -1;
        break;

    case SIGNAL_CURSOR_LSB_IDX:
        if (outb_param1 == CRTC_DATA_REG) {
            guest_info->cursor_data = SIGNAL_CURSOR_NORMAL;
            outb(CRTC_IDX_REG, CRTC_CURSOR_LSB_IDX);
            outb(CRTC_DATA_REG, outb_param2);
        } else return -1;
        break;

    case SIGNAL_CURSOR_MSB_IDX:
        if (outb_param1 == CRTC_DATA_REG) {
            guest_info->cursor_data = SIGNAL_CURSOR_NORMAL;
            outb(CRTC_IDX_REG, CRTC_CURSOR_MSB_IDX);
            outb(CRTC_DATA_REG, outb_param2);
        } else return -1;
        break;
    }

    return 0;
}

/* TODO how to run guest timer */
void _handle_int_ack(guest_info_t *guest_info) {
    switch (guest_info->pic_ack_flag) {
    case KEYBOARD_NOT_ACKED:
        // lprintf("inter_en_flag: %d, pic_ack_flag: %d in keyboard outb",
        //         guest_info->inter_en_flag, guest_info->pic_ack_flag);
        // MAGIC_BREAK;
        if (guest_info->buf_start != guest_info->buf_end) {
            /* deliver next keyboard interrupt */
            if (guest_info->inter_en_flag == ENABLED) {
                set_user_handler(KEYBOARD_DEVICE);
            } else if (guest_info->inter_en_flag == DISABLED) {
                guest_info->inter_en_flag = DISABLED_KEYBOARD_PENDING;
            }
        }
        guest_info->pic_ack_flag = ACKED;
        lprintf("inter_en_flag: %d, pic_ack_flag: %d in keyboard outb",
                guest_info->inter_en_flag, guest_info->pic_ack_flag);
        break;
    case TIMER_NOT_ACKED:
        if (guest_info->buf_start != guest_info->buf_end) {
            /* deliver next keyboard interrupt */
            if (guest_info->inter_en_flag == ENABLED) {
                set_user_handler(KEYBOARD_DEVICE);
                lprintf("after setting set_user_handler in keyboard outb");
                MAGIC_BREAK;
            } else if (guest_info->inter_en_flag == DISABLED) {
                guest_info->inter_en_flag = DISABLED_KEYBOARD_PENDING;
            }
        }
        guest_info->pic_ack_flag = ACKED;
        break;
    case TIMER_KEYBOARD_NOT_ACKED:
        if (guest_info->buf_start != guest_info->buf_end) {
            /* deliver next keyboard interrupt */
            if (guest_info->inter_en_flag == ENABLED) {
                set_user_handler(KEYBOARD_DEVICE);
                lprintf("after setting set_user_handler in keyboard outb");
            } else if (guest_info->inter_en_flag == DISABLED) {
                guest_info->inter_en_flag = DISABLED_KEYBOARD_PENDING;
            }
        }
        guest_info->pic_ack_flag = TIMER_NOT_ACKED;
        if (guest_info->buf_start != guest_info->buf_end) {
            /* deliver next keyboard interrupt */
            if (guest_info->inter_en_flag == ENABLED) {
                set_user_handler(KEYBOARD_DEVICE);
                // lprintf("after setting set_user_handler in keyboard outb");
                // MAGIC_BREAK;
            } else if (guest_info->inter_en_flag == DISABLED) {
                guest_info->inter_en_flag = DISABLED_KEYBOARD_PENDING;
            }
        }
        guest_info->pic_ack_flag = TIMER_NOT_ACKED;
        break;
    case KEYBOARD_TIMER_NOT_ACKED:
        guest_info->pic_ack_flag = KEYBOARD_NOT_ACKED;
        sche_yield(RUNNABLE);
        break;
    default:
        // assert(0 == 1);
        break;
    }

    enable_interrupts();
    return;
}

void _handle_cli(guest_info_t *guest_info) {
    if (guest_info->inter_en_flag == ENABLED)
        guest_info->inter_en_flag = DISABLED;
    return;
}

void _handle_sti(guest_info_t *guest_info) {
    // MAGIC_BREAK;
    switch (guest_info->inter_en_flag) {
    case ENABLED:
    case DISABLED:
        guest_info->inter_en_flag = ENABLED;
        break;
    case DISABLED_TIMER_PENDING:
        guest_info->inter_en_flag = ENABLED;
        // be ready to call handler
        set_user_handler(TIMER_DEVICE);
        break;
    case DISABLED_KEYBOARD_PENDING:
        guest_info->inter_en_flag = ENABLED;
        // be ready to call handler
        set_user_handler(KEYBOARD_DEVICE);
        break;
    default:
        /* impossible */
        assert(0 == 1);
        break;
    }
    // lprintf("guest_info->inter_en_flag: %d", guest_info->inter_en_flag);
    // MAGIC_BREAK;
    return;
}

int _handle_in(guest_info_t *guest_info, ureg_t *ureg) {
    uint32_t *kern_sp = (uint32_t *)(get_cur_tcb()->kern_sp);
    uint16_t inb_param = ureg->edx;
    if (inb_param == KEYBOARD_PORT) {
        int buf_start = guest_info->buf_start;
        uint8_t keycode = (uint8_t)guest_info->keycode_buf[buf_start];
        /* buf_start should never equal to buf_end in this function */
        buf_start = (buf_start + 1) % KC_BUF_LEN;
        // lprintf("buf_start moves from %u to %u",
        //         (unsigned int)guest_info->buf_start, (unsigned int)buf_start);
        guest_info->buf_start = buf_start;
        // MAGIC_BREAK;
        // *((uint32_t *)(kern_sp - 5)) = handler_addr;
        /* change the eax value that is saved on stack when went into handler */
        *((uint32_t *)(kern_sp - 7)) = (uint32_t)keycode;
        return 0;
    }
    return -1;
}

void _handle_hlt(guest_info_t *guest_info, ureg_t *ureg) {
    if (guest_info->inter_en_flag == ENABLED) kern_halt();
    /* set status to eax */
    thread_t *thread = get_cur_tcb();
    task_t *task = thread->task;
    task->status = ureg->eax;
    kern_vanish();
}

int _handle_jmp_far(guest_info_t *guest_info, char *instr) {
    // JMP FAR 0x10:0x104ef2
    unsigned int cs_value = 0;
    unsigned int memory_addr = 0;
    sscanf(instr, "JMP FAR 0x%x:0x%x", &cs_value, &memory_addr);
    // lprintf("memory_addr: 0x%08x", memory_addr);
    if (cs_value != SEGSEL_KERNEL_CS) return -1;
    memory_addr += _get_descriptor_base_addr(SEGSEL_GUEST_CS);
    // lprintf("memory_addr: 0x%08x", memory_addr);
    /* Why this memory region is not valid? */
    // int ret = validate_user_mem(memory_addr, 1, MAP_USER | MAP_EXECUTE);
    // lprintf("ret: %d", ret);
    // MAGIC_BREAK;
    // if (ret < 0) return -1;
    // MAGIC_BREAK;

    return 0;
}

void _handle_iret(guest_info_t *guest_info, ureg_t *ureg) {
    thread_t *thread = get_cur_tcb();
    uint32_t *kern_sp = (uint32_t *)(thread->kern_sp);

    uint32_t user_esp_value = ureg->esp;
    uint32_t addr_offset = _get_descriptor_base_addr(SEGSEL_GUEST_DS);
    uint32_t eip_to_return = *((uint32_t *)(user_esp_value + addr_offset));

    // lprintf("user_esp_value: %p, eip_to_return: %p",
    //         (void *)user_esp_value, (void *)eip_to_return);
    *((uint32_t *)(kern_sp - 2)) = user_esp_value + 4;
    *((uint32_t *)(kern_sp - 5)) = eip_to_return;
    // MAGIC_BREAK;
    lprintf("return addr: %p", (void *)eip_to_return);

    return;
}

void set_user_handler(int device_type) {
    disable_interrupts();
    thread_t *thread = get_cur_tcb();
    uint32_t *kern_sp = (uint32_t *)(thread->kern_sp);
    uint32_t handler_addr = 0;
    if (device_type == TIMER_DEVICE) {
        handler_addr =  _get_handler_addr(TIMER_IDT_ENTRY);
    } else if (device_type == KEYBOARD_DEVICE) {
        handler_addr =  _get_handler_addr(KEY_IDT_ENTRY);
    } else {
        /* impossible */
        assert(0 == 1);
    }
    uint32_t addr_offset = _get_descriptor_base_addr(SEGSEL_GUEST_DS);
    // *((uint32_t *)(kern_sp - 5)) = eip_value + eip_offset;
    uint32_t guest_esp_value = *((uint32_t *)(kern_sp - 2));
    guest_esp_value -= 12;

    *((uint32_t *)(guest_esp_value + addr_offset)) =
        *((uint32_t *)(kern_sp - 5));
    *((uint32_t *)(guest_esp_value + 4 + addr_offset)) =
        *((uint32_t *)(kern_sp - 4));
    *((uint32_t *)(guest_esp_value + 8 + addr_offset)) =
        *((uint32_t *)(kern_sp - 3));

    *((uint32_t *)(kern_sp - 2)) = guest_esp_value;
    *((uint32_t *)(kern_sp - 5)) = handler_addr;
    return;
}

uint32_t _get_handler_addr(int idt_idx) {
    uint32_t seg_base = _get_descriptor_base_addr(SEGSEL_GUEST_CS);
    uint32_t ori_idt_addr =
        (uint32_t)((void *)idt_base() + idt_idx * IDT_ENTRY_LENGTH);

    // lprintf("idt_base: %p, idt_idx: %d", idt_base(), idt_idx);
    // lprintf("seg_base: %p, ori_idt_addr: %p",
    //         (void *)seg_base, (void *)ori_idt_addr);
    // MAGIC_BREAK;
    uint32_t *user_idt_addr = (uint32_t *)(ori_idt_addr + seg_base);
    uint32_t handler_addr =
        LSB_HANDLER((*user_idt_addr)) + MSB_HANDLER((*(user_idt_addr + 1 )));
    return handler_addr;
}

uint32_t _get_descriptor_base_addr(uint16_t seg_sel) {
    uint32_t idx_in_gdt = GDT_INDEX(seg_sel);
    uint64_t descriptor;
    descriptor = init_gdt[idx_in_gdt];
    // TODO macro
    uint32_t base = (descriptor & 0x00000000ffff0000) >> 16;
    base += ((descriptor >> 32) & 0xff000000) + ((descriptor >> 32) & 0x000000ff);
    return base;
}

void _exn_print_ureg(ureg_t *ureg) {
    printf("Registers Dump:\n");
    printf("cause: %d, error_code: %d, cr2: 0x%08x\n",
           ureg->cause, ureg->error_code, ureg->cr2);
    printf("cs:  0x%08x, ss:  0x%08x\n", ureg->cs, ureg->ss);
    printf("ds:  0x%08x, es:  0x%08x, fs:  0x%08x, gs:  0x%08x\n",
           ureg->ds, ureg->es, ureg->fs, ureg-> gs);
    printf("edi: 0x%08x, esi: 0x%08x, ebp: 0x%08x\n",
           ureg->edi, ureg->esi, ureg->ebp);
    printf("eax: 0x%08x, ebx: 0x%08x, ecx: 0x%08x, edx: 0x%08x\n",
           ureg->eax, ureg->ebx, ureg->ecx, ureg->edx);
    printf("eip: 0x%08x, esp: 0x%08x\neflags: 0x%08x\n\n",
           ureg->eip, ureg->esp, ureg->eflags);
}
