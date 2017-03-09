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

    task_t *init = task_init("cp1");
    set_cr3((uint32_t)init->page_dir);
    set_esp0(init->main_thread->kern_sp);
    kern_to_user(init->main_thread->user_sp, init->main_thread->ip);

    task_t *idle = task_init("idle");
    set_cr3((uint32_t)idle->page_dir);
    set_esp0(idle->main_thread->kern_sp);
    kern_to_user(idle->main_thread->user_sp, idle->main_thread->ip);

    while (1) {
        continue;
    }

    return 0;
}
