/* If you want to use assembly language instead of C,
 * delete this autostack.c and provide an autostack.S
 * instead.
 */

#include <malloc.h>
#include <syscall.h>
#include <stdio.h>
#include "autostack_internal.h"

swexn_handler_t swexn_handler = autostack;

void
install_autostack(void *stack_high, void *stack_low) {
    excepetion_stack_info_t *excepetion_stack_info =
        malloc(sizeof(excepetion_stack_info_t));
    excepetion_stack_info->exeception_stack_bottom =
        malloc(INITIAL_STACK_SIZE + WORD_SIZE);
    excepetion_stack_info->esp3 =
        excepetion_stack_info->exeception_stack_bottom
        + INITIAL_STACK_SIZE + WORD_SIZE - 1;
    excepetion_stack_info->stack_low = stack_low;
    excepetion_stack_info->stack_high = stack_high;

    swexn(excepetion_stack_info->esp3,
          swexn_handler,
          excepetion_stack_info,
          NULL);
}

void autostack(void *arg, ureg_t *ureg) {
    excepetion_stack_info_t *excepetion_stack_info = arg;
    if (ureg->cause != SWEXN_CAUSE_PAGEFAULT && (ureg->error_code & 0x1) != 0) {
        printf("Thread encounter fatal error, crashing the whole task.\n");
        task_vanish(-1);
    }
    void *page_fault_addr = (void *)ureg->cr2;
    void *stack_high = excepetion_stack_info->stack_high;
    void *stack_low = excepetion_stack_info->stack_low;
    unsigned int stack_size = INITIAL_STACK_SIZE;
    unsigned int required_stack_size = stack_high - page_fault_addr;
    while (stack_size <= required_stack_size) {
        stack_size *= 2;
    }
    unsigned int len = stack_size - (stack_high - stack_low);
    unsigned int rounded_len = ROUNDED(len, PAGE_SIZE);
    void *start_addr = stack_low - rounded_len;
    int ret = new_pages(start_addr, rounded_len);
    if (ret < SUCCESS) {
        printf("Cannot allocate more space for stack, crashing the whole task.\n");
        task_vanish(-1);
    }
    swexn(excepetion_stack_info->esp3,
          swexn_handler, excepetion_stack_info, ureg);

}
