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
#include "return_type.h"

extern sche_node_t *cur_sche_node;

// will need to ginf a better way to do this eventually
extern mutex_t malloc_mutex;

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

    // TODO find a better way to init mutexes
    mutex_init(&malloc_mutex);

    /*
    task_t *init = task_init("peon");
    task_t *merchant_2 = task_init("peon");
    cur_sche_node = get_mainthr_sche_node(init);
    append_to_scheduler(get_mainthr_sche_node(merchant_2));
    */

    task_t *init = task_init();
    thread_t *thread = thread_init();
    thread->task = init;
    add_node_to_head(init->thread_list, TCB_TO_NODE(thread));

    init->task_id = thread->tid;
    init->parent_task = NULL;
    init->maps = maps_init();
    maps_insert(init->maps, 0, PAGE_SIZE * NUM_KERN_PAGES, 0);
    maps_insert(init->maps, RW_PHYS_VA, PAGE_SIZE, 0);
    // /*
    const char *fname = "my_fork_test";
    simple_elf_t elf_header;
    elf_load_helper(&elf_header, fname);
    thread->ip = elf_header.e_entry;

    // register new task for simics symbolic debugging
    sim_reg_process(init->page_dir, fname);

    set_cr3((uint32_t)init->page_dir);
    load_program(&elf_header, init->maps);
    // */

    cur_sche_node = GET_SCHE_NODE(thread);

    thread->status = RUNNABLE;
    set_esp0(thread->kern_sp);
    kern_to_user(thread->cur_sp, thread->ip);

    while (1) {
        continue;
    }

    return 0;
}
