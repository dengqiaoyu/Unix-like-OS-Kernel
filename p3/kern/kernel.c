/** @file kernel.c
 *  @brief An initial kernel.c
 *
 *  You should initialize things in kernel_main(),
 *  and then run stuff.
 *
 *  @author Harry Q. Bovik (hqbovik)
 *  @author Fred Hacker (fhacker)
 *  @bug No known bugs.
 */

#include <common_kern.h>

/* libc includes. */
#include <stdio.h>
#include <simics.h>                 /* lprintf() */

/* multiboot header file */
#include <multiboot.h>              /* boot_info */

/* x86 specific includes */
#include <x86/asm.h>                /* enable_interrupts() */
#include <x86/cr.h>
#include <x86/eflags.h>

#include "handlers.h"
#include "vm.h"
#include "task.h"
#include "asm_switch.h"
#include "scheduler.h"
#include "return_type.h"

extern sche_node_t *cur_sche_node;

/** @brief Kernel entrypoint.
 *
 *  This is the entrypoint for the kernel.
 *
 * @return Does not return
 */
int kernel_main(mbinfo_t *mbinfo, int argc, char **argv, char **envp) {
    lprintf( "Hello from a brand new kernel!" );
    RETURN_IF_ERROR(handler_init(), ERROR_KERNEL_HANDLER_INIT_FAILED);
    vm_init();
    RETURN_IF_ERROR(scheduler_init(), ERROR_KERNEL_SCHEDULER_INIT_FAILED);
    RETURN_IF_ERROR(id_counter_init(), ERROR_KERNEL_ID_COUNTER_INIT_FAILED);

    // context switch test
    // task_t *init = task_init("ck1_user");
    // task_t *idle = task_init("idle_user");
    // task_t *third_one = task_init("my_user");

    // cur_sche_node = get_mainthr_sche_node(init);
    // append_to_scheduler(get_mainthr_sche_node(idle));
    // append_to_scheduler(get_mainthr_sche_node(third_one));

    // init->main_thread->status = RUNNABLE;
    // set_cr3((uint32_t)init->page_dir);
    // set_esp0(init->main_thread->kern_sp);
    // kern_to_user(init->main_thread->user_sp, init->main_thread->ip);

    // task_t *merchant_1 = task_init("t2");
    
    /*
    task_t *merchant_1 = task_init("peon");
    task_t *merchant_2 = task_init("peon");
    cur_sche_node = get_mainthr_sche_node(merchant_1);
    append_to_scheduler(get_mainthr_sche_node(merchant_2));
    */

    // merchant_1->main_thread->status = RUNNABLE;
    // set_cr3((uint32_t)merchant_1->page_dir);
    // set_esp0(merchant_1->main_thread->kern_sp);
    // kern_to_user(merchant_1->main_thread->user_sp, merchant_1->main_thread->ip);

    task_t *init = task_init("my_fork_test");
    cur_sche_node = get_mainthr_sche_node(init);
    /*
    task_t *init = merchant_1;
    */
    init->main_thread->status = RUNNABLE;
    set_cr3((uint32_t)init->page_dir);
    set_esp0(init->main_thread->kern_sp);
    kern_to_user(init->main_thread->user_sp, init->main_thread->ip);

    while (1) {
        continue;
    }

    return 0;
}
