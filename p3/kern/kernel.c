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
#include <page.h>
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
#include "tcb_hashtab.h"
#include "return_type.h"

// will need to find a better way to do this eventually
extern mutex_t malloc_mutex;

/** @brief Kernel entrypoint.
 *
 *  This is the entrypoint for the kernel.
 *
 * @return Does not return
 */
int kernel_main(mbinfo_t *mbinfo, int argc, char **argv, char **envp) {
    lprintf( "Hello from a brand new kernel!" );

    handler_init();
    vm_init();
    scheduler_init();
    id_counter_init();

    // TODO find a better way to init mutexes
    mutex_init(&malloc_mutex);
    tcb_hashtab_init();

    /*
    task_t *init = task_init("peon");
    task_t *merchant_2 = task_init("peon");
    append_to_scheduler(get_mainthr_sche_node(merchant_2));
    */


    task_t *init = task_init();
    thread_t *thread = thread_init();
    thread->task = init;
    add_node_to_head(init->live_thread_list, TCB_TO_LIST_NODE(thread));

    init->task_id = thread->tid;
    init->parent_task = NULL;
    maps_insert(init->maps, 0, PAGE_SIZE * NUM_KERN_PAGES, 0);
    maps_insert(init->maps, RW_PHYS_VA, PAGE_SIZE, 0);
    // /*
    const char *fname = "yield_desc_mkrun";
    simple_elf_t elf_header;
    elf_load_helper(&elf_header, fname);
    thread->ip = elf_header.e_entry;

    set_cur_run_thread(thread);
    // register new task for simics symbolic debugging
    sim_reg_process(init->page_dir, fname);
    set_cr3((uint32_t)init->page_dir);
    load_program(&elf_header, init->maps);
    // */
    thread->status = RUNNABLE;
    set_esp0(thread->kern_sp);
    kern_to_user(thread->cur_sp, thread->ip);

    while (1) {
        continue;
    }

    return 0;
}
