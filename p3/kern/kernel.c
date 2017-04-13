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
#include <console.h>

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

// will need to find a better way to do this eventually
extern mutex_t malloc_mutex;

extern thread_t *idle_thread;

thread_t *setup_task(const char *fname) {
    task_t *task = task_init();
    thread_t *thread = thread_init();
    thread->task = task;
    add_node_to_head(task->live_thread_list, TCB_TO_LIST_NODE(thread));
    thread->status = INITIALIZED;

    task->task_id = thread->tid;
    task->parent_task = NULL;

    // TODO can we macro these better?
    maps_insert(task->maps, 0, PAGE_SIZE * NUM_KERN_PAGES - 1, 0);
    maps_insert(task->maps, RW_PHYS_VA, RW_PHYS_VA + (PAGE_SIZE - 1), 0);
    
    simple_elf_t elf_header;
    elf_load_helper(&elf_header, fname);
    thread->ip = elf_header.e_entry;

    // register new task for simics symbolic debugging
    sim_reg_process(task->page_dir, fname);

    set_cr3((uint32_t)task->page_dir);
    load_program(&elf_header, task->maps);

    return thread;
}

/** @brief Kernel entrypoint.
 *
 *  This is the entrypoint for the kernel.
 *
 * @return Does not return
 */
int kernel_main(mbinfo_t *mbinfo, int argc, char **argv, char **envp) {
    lprintf( "Hello from a brand new kernel!" );
    clear_console();

    RETURN_IF_ERROR(handler_init(), ERROR_KERNEL_HANDLER_INIT_FAILED);
    vm_init();
    RETURN_IF_ERROR(scheduler_init(), ERROR_KERNEL_SCHEDULER_INIT_FAILED);
    RETURN_IF_ERROR(id_counter_init(), ERROR_KERNEL_ID_COUNTER_INIT_FAILED);

    // TODO find a better way to init mutexes
    mutex_init(&malloc_mutex);

    idle_thread = setup_task("idle");

    const char *fname = "actual_wait";
    /*
    const char *fname = "remove_pages_test2";
    const char *fname = "fork_wait_bomb";
    const char *fname = "fork_exit_bomb";
    */

    thread_t *first_thread = setup_task(fname);
    set_cur_run_thread(first_thread);
    set_esp0(first_thread->kern_sp);
    kern_to_user(first_thread->cur_sp, first_thread->ip);

    while (1) {
        MAGIC_BREAK;
        continue;
    }

    return 0;
}
