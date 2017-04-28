#ifndef __AUTOSTACK_INTERNAL_H__
#define __AUTOSTACK_INTERNAL_H__

#define INITIAL_STACK_SIZE 4096
#define MAX_STACK_SIZE (INITIAL_STACK_SIZE * INITIAL_STACK_SIZE)
#define EXECEPTION_STACK_SIZE INITIAL_STACK_SIZE
#define WORD_SIZE 4
#define ERROR_CODE_P (1)
#define ERROR_CODE_WR (1 << 1)
#define ERROR_CODE_US (1 << 2)
#define ERROR_CODE_RSVD (1 << 3)

#include <syscall.h>

#define ROUNDED(x, y) ((x) + (y) - 1) / (y) * (y)
#define MIN(x, y) (x) < (y) ? (x) : (y)

#define SUCCESS 0

typedef struct exn_stk_info_t {
    void *exn_stk_bottom;
    void *esp3;
    void *stack_high;
    void *stack_low;
} exn_stk_info_t;

void autostack(void *arg, ureg_t *ureg);
void install_print_and_vanish(void *stack_base);
void print_and_vanish(void *arg, ureg_t *ureg);
void print_error_msg(ureg_t *ureg);
void simu_sensi_instr(ureg_t *ureg);
void print_ureg(ureg_t *ureg);

#endif /* __AUTOSTACK_INTERNAL_H__ */