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

/* internal function */
static int _simulate_instr(char *instr, ureg_t *ureg);
static int _timer_init(ureg_t *ureg);
static int _set_cursor(ureg_t *ureg);

guest_t *guest_init() {
    guest_t *guest = calloc(1, sizeof(guest_t));
    guest->io_ack_flag = 0;
    /* virtual timer */
    guest->timer_init_stat = TIMER_UNINT;
    guest->timer_init_stat = 0;

    /* virtual_console, maybe? */
    guest->cursor_state = SIGNAL_CURSOR_NORMAL;
    guest->cursor_idx = 0;

    return guest;
}

void guest_destroy(guest_t *guest) {
    //TODO
    free(guest);
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
    guest_t *guest = get_cur_tcb()->task->guest;
    /* begin to compare supported sensitive instruction */
    // void *handler_addr = NULL;
    int ret = 0;
    if (strcmp(instr, "OUT DX, AX") == 0) {
        uint16_t outb_param1 = ureg->edx;
        uint8_t outb_param2 = ureg->eax;
        lprintf("outb_param1: %x, outb_param2: %x", outb_param1, outb_param2);
        if (outb_param1 == TIMER_MODE_IO_PORT
                || outb_param1 == TIMER_PERIOD_IO_PORT) {
            /* guest set timer */
            ret = _timer_init(ureg);
            if (ret == 0) return 0;
            return 0;
        } else if (outb_param1 == INT_ACK_CURRENT) {
            /* guest ack device interrupt */
            guest->io_ack_flag = 1;
            return 0;
        } else if (outb_param1 == CRTC_IDX_REG || ureg->eax == CRTC_DATA_REG) {
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
    guest_t *guest = get_cur_tcb()->task->guest;
    switch (guest->timer_init_stat) {
    case TIMER_UNINT:
        if (outb_param1 == TIMER_MODE_IO_PORT)
            guest->timer_init_stat = TIMER_FREQUENCY_PART1;
        else /* invalid timer init process */
            return -1;
        break;
    case TIMER_FREQUENCY_PART1:
        if (outb_param1 == TIMER_PERIOD_IO_PORT)
            guest->timer_init_stat = TIMER_FREQUENCY_PART2;
        else /* invalid timer init process */
            return -1;
        guest->timer_interval = outb_param2;
        break;
    case TIMER_FREQUENCY_PART2:
        if (outb_param1 == TIMER_PERIOD_IO_PORT)
            guest->timer_init_stat = TIMER_INTED;
        else /* invalid timer init process */
            return -1;
        guest->timer_interval += (outb_param2) << 8;
        guest->timer_interval =
            MS_PER_S / (TIMER_RATE / guest->timer_interval);
        lprintf("guest->timer_interval: %d", (int)guest->timer_interval);
        break;
    default:
        return -1;
    }
    return 0;
}

int _set_cursor(ureg_t *ureg) {
    guest_t *guest = get_cur_tcb()->task->guest;
    switch (guest->cursor_state) {
    case SIGNAL_CURSOR_NORMAL:
        if (ureg->edx == CRTC_IDX_REG && ureg->eax == CRTC_CURSOR_LSB_IDX) {
            guest->cursor_state = SIGNAL_CURSOR_LSB_IDX;
        } else if (ureg->edx == CRTC_IDX_REG
                   && ureg->eax == CRTC_CURSOR_MSB_IDX) {
            guest->cursor_state = SIGNAL_CURSOR_MSB_IDX;
        } else return -1;
        break;
    case SIGNAL_CURSOR_LSB_IDX:
        if (ureg->edx == CRTC_DATA_REG) {
            guest->cursor_state = SIGNAL_CURSOR_NORMAL;
            guest->cursor_idx = ureg->eax;
        } else return -1;
        break;
    case SIGNAL_CURSOR_MSB_IDX:
        if (ureg->edx == CRTC_DATA_REG) {
            guest->cursor_state = SIGNAL_CURSOR_NORMAL;
            guest->cursor_idx += ureg->eax << 8;
            /* begin set cursor */
            int row = guest->cursor_idx / CONSOLE_WIDTH;
            int col = guest->cursor_idx % CONSOLE_WIDTH;
            set_cursor(row, col);
        } else return -1;
        break;
    }
    return 0;
}
