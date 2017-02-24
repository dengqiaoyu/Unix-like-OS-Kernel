/* If you want to use assembly language instead of C,
 * delete this autostack.c and provide an autostack.S
 * instead.
 */

#include <malloc.h>
#include <syscall.h>
#include <stdio.h>
#include <simics.h>
#include "autostack_internal.h"

swexn_handler_t swexn_handler = autostack;
excepetion_stack_info_t global_excepetion_stack_info = {0};
char exeception_stack_bottom[EXECEPTION_STACK_SIZE + WORD_SIZE] = {0};

void
install_autostack(void *stack_high, void *stack_low) {
    // lprintf("thread %d installs autostack\n", gettid());
    // excepetion_stack_info_t *excepetion_stack_info =
    //     &global_excepetion_stack_info;
    // excepetion_stack_info->exeception_stack_bottom = exeception_stack_bottom;
    // excepetion_stack_info->esp3 =
    //     excepetion_stack_info->exeception_stack_bottom
    //     + INITIAL_STACK_SIZE + WORD_SIZE - 1;
    // excepetion_stack_info->stack_low = stack_low;
    // excepetion_stack_info->stack_high = stack_high;

    // swexn(excepetion_stack_info->esp3,
    //       swexn_handler,
    //       excepetion_stack_info,
    //       NULL);
}

void autostack(void *arg, ureg_t *ureg) {
    lprintf("trapped!\n");
    excepetion_stack_info_t *excepetion_stack_info = arg;
    void *page_fault_addr = (void *)ureg->cr2;
    if (ureg->cause != SWEXN_CAUSE_PAGEFAULT && (ureg->error_code & 0x1) != 0) {
        printf("Thread encounter fatal error, crashing the whole task.\n");
        task_vanish(-1);
    } else if (page_fault_addr == NULL) {
        printf("Cannot derefrence NULL pointer, crashing the whole task.\n");
        task_vanish(-1);
    }

    lprintf("page_fault_addr: %p\n", page_fault_addr);
    // while (1) {};
    void *stack_high = excepetion_stack_info->stack_high;
    lprintf("stack_high: %p\n", stack_high);
    void *stack_low = excepetion_stack_info->stack_low;
    lprintf("stack_low: %p\n", stack_low);
    unsigned int stack_size = INITIAL_STACK_SIZE;
    unsigned int max_stack_size = (unsigned int)stack_high / 2;
    unsigned int required_stack_size = stack_high - page_fault_addr;
    lprintf("required_stack_size: %u\n", required_stack_size);
    if (required_stack_size > max_stack_size) {
        printf("Required size of stack is too large, crashing the whole task.\n");
        task_vanish(-1);
    }
    while (stack_size < required_stack_size) {
        stack_size *= 2;
    }
    unsigned int len = stack_size - (stack_high - stack_low);
    lprintf("len: %u\n", len);
    unsigned int rounded_len = MIN(ROUNDED(len, PAGE_SIZE),
                                   (unsigned int)stack_high);
    lprintf("rounded_len: %d\n", rounded_len);
    if ((unsigned int)stack_low < rounded_len) {
        printf("Required size of stack is too large, crashing the whole task.\n");
        task_vanish(-1);
    }
    void *start_addr = stack_low - rounded_len;
    lprintf("start_addr: %p\n", start_addr);
    excepetion_stack_info->stack_low = start_addr;
    int ret = new_pages(start_addr, rounded_len);
    if (ret != SUCCESS) {
        printf("Cannot allocate more space for stack, crashing the whole task.\n");
        task_vanish(-1);
    }
    // sim_breakpoint();
    swexn(excepetion_stack_info->esp3,
          swexn_handler, excepetion_stack_info, ureg);
}

void vanish_gracefully(void *arg, ureg_t *ureg) {
    task_vanish(-1);
}
