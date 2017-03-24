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
    int ret = SUCCESS;
    lprintf( "Hello from a brand new kernel!" );

    install_handlers();

    vm_init();
    ret = init_scheduler();
    if (ret != SUCCESS) {
        lprintf("Impossible!\n");
        return -1;
    }

    lprintf("!!!before task_init\n");
    task_t *init = task_init("ck1_user");
    task_t *idle = task_init("idle_user");
    lprintf("!!!after task_init\n");

    lprintf("idle: %p\n", idle);
    lprintf("@@@before get_mainthr_sche_node\n");
    cur_sche_node = get_mainthr_sche_node(init);
    lprintf("########cur_sche_node: %p\n", cur_sche_node);
    append_to_scheduler(get_mainthr_sche_node(idle));
    lprintf("@@@after append_to_scheduler\n");
    init->main_thread->status = RUNNABLE;
    lprintf("line 64\n");
    set_cr3((uint32_t)init->page_dir);
    lprintf("line 66\n");
    set_esp0(init->main_thread->kern_sp);
    lprintf("line 68\n");
    kern_to_user(init->main_thread->user_sp, init->main_thread->ip);
    lprintf("line 70\n");

    while (1) {
        continue;
    }

    return 0;
}
