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

#define MSB_HANDLER(x) ((x) & 0xffff0000)
#define LSB_HANDLER(x) ((x) & 0x0000ffff)

#define GUEST_MEM_SIZE (24 * 1024 * 1024)
#define GUEST_FRAMES (GUEST_MEM_SIZE / PAGE_SIZE)
#define GUEST_CONSOLE_BASE (CONSOLE_MEM_BASE + USER_MEM_START)

/* translate sensitive instruction */
#define MAX_INSTR_LENGTH 32
#define MAX_DECODED_LENGTH 64
#define MS_PER_S 1000
#define KC_BUF_LEN 32

/* device type */
#define KEYBOARD_DEVICE 0
#define TIMER_DEVICE 1

#define GDT_INDEX(x) (((x) & 0xfff8) >> 3)

typedef struct guest_info_t {
    /* use disable_interrupt to protect ? */
    int pic_ack_flag;
    int inter_en_flag;
    /* virtual timer */
    int timer_init_stat;
    uint32_t internal_ticks;
    uint32_t timer_interval;

    /* virtual keyboard */
    uint32_t keycode_buf[KC_BUF_LEN];
    int buf_start;
    int buf_end;

    /* virtual cursor registers */
    int cursor_data;
    uint32_t cursor_idx;

    /* virtual console status info */
    console_state_t *console_state;
} guest_info_t;

/* pic_ack_flag */
#define ACKED 0
#define KEYBOARD_NOT_ACKED 1
#define TIMER_NOT_ACKED 2
#define TIMER_KEYBOARD_NOT_ACKED 3
#define KEYBOARD_TIMER_NOT_ACKED 4

/* inter_en_flag */
#define ENABLED 0
#define DISABLED 1
#define DISABLED_TIMER_PENDING 2
#define DISABLED_KEYBOARD_PENDING 3

/* timer_status */
#define TIMER_UNINT 0
#define TIMER_FREQUENCY_PART1 2
#define TIMER_FREQUENCY_PART2 3
#define TIMER_INTED 4

/* cursor_status */
#define SIGNAL_CURSOR_NORMAL 0
#define SIGNAL_CURSOR_LSB_IDX 1
#define SIGNAL_CURSOR_MSB_IDX 2

#define IDT_ENTRY_LENGTH 8

// TODO docs

void hypervisor_init();

int guest_init(simple_elf_t *header);

int load_guest_section(const char *fname, unsigned long start,
                       unsigned long len, unsigned long offset);

guest_info_t *guest_info_init();

void guest_info_destroy(guest_info_t *guest_info);

int guest_console_mov(uint32_t *dest, ureg_t *ureg);

int handle_sensi_instr(ureg_t *ureg);

void set_user_handler(int device_type);

/* helper */
// uint32_t get_descriptor_base_addr(uint16_t seg_sel);

#endif /* _HYPERVISOR_H_ */
