#include <malloc.h>
#include <syscall.h>
#include <stdio.h>
#include <simics.h>
#include "autostack_internal.h"

static swexn_handler_t autostack_handler = autostack;
static exn_stk_info_t glb_exn_stk_info = {0};
static char exn_stk_bottom[EXECEPTION_STACK_SIZE + WORD_SIZE] = {0};

void install_autostack(void *stack_high, void *stack_low) {
    exn_stk_info_t *exn_stk_info = &glb_exn_stk_info;
    exn_stk_info->exn_stk_bottom = exn_stk_bottom;
    exn_stk_info->esp3 = exn_stk_bottom + INITIAL_STACK_SIZE + WORD_SIZE - 1;
    exn_stk_info->stack_low = stack_low;
    exn_stk_info->stack_high = stack_high;
    swexn(exn_stk_info->esp3, autostack_handler, exn_stk_info, NULL);
}

void autostack(void *arg, ureg_t *ureg) {
    exn_stk_info_t *exn_stk_info = arg;
    void *page_fault_addr = (void *)ureg->cr2;
    if (ureg->cause != SWEXN_CAUSE_PAGEFAULT) {
        print_and_vanish(ureg);
    } else if (page_fault_addr == NULL) {
        printf("NULL pointer derefrence\n");
        print_and_vanish(ureg);
    }

    void *stack_high = exn_stk_info->stack_high;
    void *stack_low = exn_stk_info->stack_low;
    unsigned int stack_size = INITIAL_STACK_SIZE;
    unsigned int max_stack_size = MAX_STACK_SIZE;
    unsigned int dist_to_low = stack_low - page_fault_addr;
    if (dist_to_low > INITIAL_STACK_SIZE * 1024) {
        printf("Invalid memory access\n");
        print_and_vanish(ureg);
    }
    unsigned int required_stack_size = stack_high - page_fault_addr;
    if (required_stack_size > max_stack_size) {
        printf("Required stack too large\n");
        print_and_vanish(ureg);
    }
    while (stack_size < required_stack_size) {
        stack_size *= 2;
    }
    unsigned int len = stack_size - (stack_high - stack_low);
    unsigned int rounded_len = ROUNDED(len, PAGE_SIZE);
    void *start_addr = stack_low - rounded_len;
    exn_stk_info->stack_low = start_addr;
    int ret = new_pages(start_addr, rounded_len);
    if (ret != SUCCESS) {
        printf("No more space for stack\n");
        print_and_vanish(ureg);
    }
    swexn(exn_stk_info->esp3,
          autostack_handler, exn_stk_info, ureg);
}

void print_and_vanish(ureg_t *ureg) {
    print_error_msg(ureg);
    set_status(-2);
    vanish();
}

void print_error_msg(ureg_t *ureg) {
    unsigned int fault_addr = ureg->cr2;
    unsigned int fault_ip_addr = ureg->eip;
    int error_code = ureg->error_code;
    printf("Instruction at 0x%08x accessing 0x%08x got exception\n",
           fault_ip_addr, fault_addr);
    switch (ureg->cause) {
    case SWEXN_CAUSE_DIVIDE:
        printf("DIVIDE ERROR\n");
        break;
    case SWEXN_CAUSE_DEBUG:
        printf("DEBUG ERROR\n");
        break;
    case SWEXN_CAUSE_BREAKPOINT:
        printf("BREAKPOINT ERROR\n");
        break;
    case SWEXN_CAUSE_OVERFLOW:
        printf("OVERFLOW ERROR\n");
        break;
    case SWEXN_CAUSE_BOUNDCHECK:
        printf("BOUNDCHECK ERROR\n");
        break;
    case SWEXN_CAUSE_OPCODE:
        printf("OPCODE ERROR\n");
        break;
    case SWEXN_CAUSE_NOFPU:
        printf("NOFPU ERROR\n");
        break;
    case SWEXN_CAUSE_SEGFAULT:
        printf("SEGFAULT ERROR\n");
        break;
    case SWEXN_CAUSE_STACKFAULT:
        printf("STACKFAULT ERROR\n");
        break;
    case SWEXN_CAUSE_PROTFAULT:
        printf("PROTFAULT ERROR\n");
        break;
    case SWEXN_CAUSE_PAGEFAULT:
        printf("PAGEFAULT ERROR:\n");
        if (error_code & ERROR_CODE_P) printf("non-present page's");
        else printf("non-present page's");
        if (error_code & ERROR_CODE_WR) printf(" write fault");
        else printf(" read fault");
        if (error_code & ERROR_CODE_US) printf(" in user mode");
        else printf(" in supervisor mode");
        if (error_code & ERROR_CODE_RSVD)
            printf(" with reserved bit violation\n");
        else printf("\n");
        break;
    case SWEXN_CAUSE_FPUFAULT:
        printf("FPUFAULT ERROR\n");
        break;
    case SWEXN_CAUSE_ALIGNFAULT:
        printf("ALIGNFAULT ERROR\n");
        break;
    case SWEXN_CAUSE_SIMDFAULT:
        printf("SIMDFAULT ERROR\n");
        break;
    default:
        printf("UNKNOWN ERROR\n");
    }
    print_ureg(ureg);
}

void print_ureg(ureg_t *ureg) {
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
    printf("eip: 0x%08x, esp: 0x%08x\neflags: 0x%08x\n",
           ureg->eip, ureg->esp, ureg->eflags);
}

