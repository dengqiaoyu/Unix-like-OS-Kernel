#ifndef _HYPERVISOR_H_
#define _HYPERVISOR_H_

#include <stdint.h>
#include <ureg.h>                       /* ureg_t */
#include <elf_410.h>
#include <console.h>

#define SEGDES_GUEST_CS 0x01cffb000000ffff
#define SEGDES_GUEST_DS 0x01cff2000000ffff

#define SEGSEL_GUEST_CS (SEGSEL_SPARE0 | 0x3)
#define SEGSEL_GUEST_DS (SEGSEL_SPARE1 | 0x3)

#define GUEST_MEM_SIZE (24 * 1024 * 1024)
#define GUEST_FRAMES (GUEST_MEM_SIZE / PAGE_SIZE)

/* translate sensitive instruction */
#define MAX_INS_DECODED_LENGTH 64
#define MS_PER_S 1000

typedef struct guest_info_t {
    int io_ack_flag;
    /* virtual timer */
    int timer_init_stat;
    uint32_t timer_interval;

    /* virtual cursor registers */
    int cursor_data;
    uint32_t cursor_idx;

    /* virtual console status info */
    console_state_t *console_state;
} guest_info_t;

/* timer_status */
#define TIMER_UNINT 0
#define TIMER_FREQUENCY_PART1 2
#define TIMER_FREQUENCY_PART2 3
#define TIMER_INTED 4

/* cursor_status */
#define SIGNAL_CURSOR_NORMAL 0
#define SIGNAL_CURSOR_LSB_IDX 1
#define SIGNAL_CURSOR_MSB_IDX 2

void hypervisor_init();
int guest_init(simple_elf_t *header);
int load_guest_section(const char *fname, unsigned long start,
                       unsigned long len, unsigned long offset);
guest_info_t *guest_info_init();
void guest_info_destroy(guest_info_t *guest_info);
int handle_sensi_instr(ureg_t *ureg);

#endif /* _HYPERVISOR_H_ */
