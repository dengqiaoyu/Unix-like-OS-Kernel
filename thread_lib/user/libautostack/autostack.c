/* If you want to use assembly language instead of C,
 * delete this autostack.c and provide an autostack.S
 * instead.
 */

#include <malloc.h>
#include <syscall.h>
#include <stdio.h>
#include <simics.h>
#include "autostack_internal.h"

static swexn_handler_t autostack_handler = autostack;
static swexn_handler_t vanish_gracefully_handler = vanish_gracefully;
static excepetion_stack_info_t global_excepetion_stack_info = {0};
static char exeception_stack_bottom[EXECEPTION_STACK_SIZE + WORD_SIZE] = {0};

void
install_autostack(void *stack_high, void *stack_low) {
    excepetion_stack_info_t *excepetion_stack_info =
        &global_excepetion_stack_info;
    excepetion_stack_info->exeception_stack_bottom = exeception_stack_bottom;
    excepetion_stack_info->esp3 =
        excepetion_stack_info->exeception_stack_bottom
        + INITIAL_STACK_SIZE + WORD_SIZE - 1;
    excepetion_stack_info->stack_low = stack_low;
    excepetion_stack_info->stack_high = stack_high;

    swexn(excepetion_stack_info->esp3,
          autostack_handler,
          excepetion_stack_info,
          NULL);
}

void autostack(void *arg, ureg_t *ureg) {
    excepetion_stack_info_t *excepetion_stack_info = arg;
    void *page_fault_addr = (void *)ureg->cr2;
    if (ureg->cause != SWEXN_CAUSE_PAGEFAULT && (ureg->error_code & 0x1) != 0) {
        vanish_gracefully(excepetion_stack_info->esp3, ureg);
    } else if (page_fault_addr == NULL) {
        printf("Cannot derefrence NULL pointer, crashing the whole task.\n");
        task_vanish(-1);
    }

    void *stack_high = excepetion_stack_info->stack_high;
    void *stack_low = excepetion_stack_info->stack_low;
    unsigned int stack_size = INITIAL_STACK_SIZE;
    unsigned int max_stack_size = (unsigned int)stack_high / 2;
    unsigned int required_stack_size = stack_high - page_fault_addr;
    if (required_stack_size > max_stack_size) {
        printf("Required size of stack is too large, crashing the whole task.\n");
        task_vanish(-1);
    }
    while (stack_size < required_stack_size) {
        stack_size *= 2;
    }
    unsigned int len = stack_size - (stack_high - stack_low);
    unsigned int rounded_len = MIN(ROUNDED(len, PAGE_SIZE),
                                   (unsigned int)stack_high);
    if ((unsigned int)stack_low < rounded_len) {
        printf("Required size of stack is too large, crashing the whole task.\n");
        task_vanish(-1);
    }
    void *start_addr = stack_low - rounded_len;
    excepetion_stack_info->stack_low = start_addr;
    int ret = new_pages(start_addr, rounded_len);
    if (ret != SUCCESS) {
        printf("Cannot allocate more space for stack, crashing the whole task.\n");
        task_vanish(-1);
    }
    swexn(excepetion_stack_info->esp3,
          autostack_handler, excepetion_stack_info, ureg);
}

void install_vanish_gracefully(void *stack_base) {
    void *esp3 = stack_base + WORD_SIZE;
    swexn(esp3, vanish_gracefully_handler, esp3, NULL);
}

void vanish_gracefully(void *arg, ureg_t *ureg) {
    int cause = ureg->cause;
    switch (cause) {
    case SWEXN_CAUSE_DEBUG:
    case SWEXN_CAUSE_BREAKPOINT:
    case SWEXN_CAUSE_OVERFLOW:
    case SWEXN_CAUSE_BOUNDCHECK:
        swexn(arg, vanish_gracefully_handler, NULL, ureg);
        break;
    default:
        print_error_msg(cause, (void *)(ureg->eip));
        task_vanish(-1);
    }
}

void print_error_msg(int cause, void *addr) {
    printf("At address %p, ", addr);
    switch (cause) {
    case SWEXN_CAUSE_DIVIDE:
        printf("SWEXN_CAUSE_DIVIDE ERROR\n");
        break;
    case SWEXN_CAUSE_DEBUG:
        printf("SWEXN_CAUSE_DEBUG ERROR\n");
        break;
    case SWEXN_CAUSE_BREAKPOINT:
        printf("SWEXN_CAUSE_BREAKPOINT ERROR\n");
        break;
    case SWEXN_CAUSE_OVERFLOW:
        printf("SWEXN_CAUSE_OVERFLOW ERROR\n");
        break;
    case SWEXN_CAUSE_BOUNDCHECK:
        printf("SWEXN_CAUSE_BOUNDCHECK ERROR\n");
        break;
    case SWEXN_CAUSE_OPCODE:
        printf("SWEXN_CAUSE_OPCODE ERROR\n");
        break;
    case SWEXN_CAUSE_NOFPU:
        printf("SWEXN_CAUSE_NOFPU ERROR\n");
        break;
    case SWEXN_CAUSE_SEGFAULT:
        printf("SWEXN_CAUSE_SEGFAULT ERROR\n");
        break;
    case SWEXN_CAUSE_STACKFAULT:
        printf("SWEXN_CAUSE_STACKFAULT ERROR\n");
        break;
    case SWEXN_CAUSE_PROTFAULT:
        printf("SWEXN_CAUSE_PROTFAULT ERROR\n");
        break;
    case SWEXN_CAUSE_PAGEFAULT:
        printf("SWEXN_CAUSE_PAGEFAULT ERROR\n");
        break;
    case SWEXN_CAUSE_FPUFAULT:
        printf("SWEXN_CAUSE_FPUFAULT ERROR\n");
        break;
    case SWEXN_CAUSE_ALIGNFAULT:
        printf("SWEXN_CAUSE_ALIGNFAULT ERROR\n");
        break;
    case SWEXN_CAUSE_SIMDFAULT:
        printf("SWEXN_CAUSE_SIMDFAULT ERROR\n");
        break;
    default:
        printf("UNKNOWN ERROR\n");
    }
}
