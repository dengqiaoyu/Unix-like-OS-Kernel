#ifndef __AUTOSTACK_INTERNAL_H__
#define __AUTOSTACK_INTERNAL_H__

#define INITIAL_STACK_SIZE 4096
#define EXECEPTION_STACK_SIZE INITIAL_STACK_SIZE
#define WORD_SIZE 4

#include <syscall.h>

#define ROUNDED(x, y) ((x) + (y) - 1) / (y) * (y)

#define SUCCESS 0

typedef struct excepetion_stack_info_t {
    void *exeception_stack_bottom;
    void *esp3;
    void *stack_high;
    void *stack_low;
} excepetion_stack_info_t;

void autostack(void *arg, ureg_t *ureg);

#endif /* __AUTOSTACK_INTERNAL_H__ */