#ifndef _HYPERVISOR_H_
#define _HYPERVISOR_H_

#include <stdint.h>
#include <ureg.h>                       /* ureg_t */

/* translate sensitive instruction */
#define MAX_INS_DECODED_LENGTH 64
#define MS_PER_S 1000

typedef struct guest_t {
    int io_ack_flag;
    /* virtual timer */
    int timer_init_stat;
    uint32_t timer_interval;

    /* virtual_console, maybe? */
    int cursor_state;
    uint32_t cursor_idx;
} guest_t;
/* timer_status */
#define TIMER_UNINT 0
#define TIMER_FREQUENCY_PART1 2
#define TIMER_FREQUENCY_PART2 3
#define TIMER_INTED 4

/* cursor_status */
#define SIGNAL_CURSOR_NORMAL 0
#define SIGNAL_CURSOR_LSB_IDX 1
#define SIGNAL_CURSOR_MSB_IDX 2

guest_t *guest_init();
int handle_sensi_instr(ureg_t *ureg);

#endif /* _HYPERVISOR_H_ */