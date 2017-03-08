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

#include "handlers.h"
#include "vm.h"
#include "task.h"
#include "asm_switch.h"

/** @brief Kernel entrypoint.
 *  
 *  This is the entrypoint for the kernel.
 *
 * @return Does not return
 */
int kernel_main(mbinfo_t *mbinfo, int argc, char **argv, char **envp)
{
    lprintf( "Hello from a brand new kernel!" );

    install_handlers();

    vm_init();

    /*
    task_t *init = task_init("ck1");
    kern_to_user(init->main_thread->user_sp, init->main_thread->ip);
    */

    task_t *init = task_init("init");
    kern_to_user(init->main_thread->user_sp, init->main_thread->ip);

    task_t *idle = task_init("idle");
    kern_to_user(idle->main_thread->user_sp, idle->main_thread->ip);

    while (1) {
        continue;
    }

    return 0;
}
