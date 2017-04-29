/**
 *  @file   kernel.c
 *  @brief  An initial kernel.c
 *  @author Newton Xie (ncx)
 *  @author Qiaoyu Deng (qdeng)
 *  @bug    No known bugs.
 */

#include <common_kern.h>

/* libc includes. */
#include <stdio.h>
#include <simics.h>                     /* lprintf() */
#include <console.h>                    /* clear_console */

/* multiboot header file */
#include <multiboot.h>                  /* boot_info */

/* x86 specific includes */
#include <x86/asm.h>                    /* enable_interrupts() */
#include <x86/cr.h>
#include <x86/eflags.h>

#include "handlers.h"                   /* handler_init */
#include "vm.h"                         /* vm_init */
#include "task.h"                       /* task_init, thread_init */
#include "asm_kern_to_user.h"           /* kern_to_user */
#include "scheduler.h"                  /* scheduler_init */
#include "utils/tcb_hashtab.h"          /* tcb_hashtab_init */
#include "drivers/keyboard_driver.h"    /* keyboard_init */
#include "inc/hypervisor.h"

// will need to find a better way to do this eventually
extern kern_mutex_t malloc_mutex;
extern kern_mutex_t print_mutex;

extern thread_t *idle_thread;
extern thread_t *init_thread;

/* internal functions */
void mutexes_init();
void helper_init();
thread_t *setup_task(const char *fname);

/** @brief Kernel entrypoint.
 *
 *  This is the entrypoint for the kernel.
 *
 * @return Does not return
 */
int kernel_main(mbinfo_t *mbinfo, int argc, char **argv, char **envp) {
    /* clear the messy console after booting */
    clear_console();

    /* install exception handler, device driver and all of syscalls */
    handler_init();
    /* set up kernel page directory, and set up physical memory allocator */
    vm_init();
    /* initialize basic mutexes that are need before kernel start running */
    mutexes_init();
    /* initialize scheduler's list */
    scheduler_init();
    /* set up keyboard input buffer */
    keyboard_init();
    /* set up other essentials like tcb table, tid counter */
    helper_init();

    // TODO
    hypervisor_init();

    /* set up an idle task for thread to switch when there is no more thread */
    idle_thread = setup_task("idle");

    /* step up the first real task running */
    init_thread = setup_task("init");
    set_cur_run_thread(init_thread);
    set_esp0(init_thread->kern_sp);
    kern_to_user(init_thread->cur_sp, init_thread->ip);

    while (1) {
        MAGIC_BREAK;
        continue;
    }

    return 0;
}

/**
 * Initialize malloc mutex making it thread safe, and print_mutex to prevent
 * interleaving.
 */
void mutexes_init() {
    kern_mutex_init(&malloc_mutex);
    kern_mutex_init(&print_mutex);
}

/**
 * helper function initialization
 */
void helper_init() {
    /* initialize tcb hash table that is used to search tcb according tid */
    tcb_hashtab_init();
    /* initialize tid counter */
    id_counter_init();
}

thread_t *setup_task(const char *fname) {
    task_t *task = task_init();
    if (task == NULL) return NULL;
    task->parent_task = NULL;

    int ret = 0;
    /* validate the memory map for the kernel memory, 16MB */
    ret += maps_insert(task->maps, 0, PAGE_SIZE * NUM_KERN_PAGES - 1, 0);
    /* validate the memory map for last page used to access physical frames */
    ret += maps_insert(task->maps, RW_PHYS_VA, RW_PHYS_VA + (PAGE_SIZE - 1), 0);
    if (ret < 0) {
        task_destroy(task);
        return NULL;
    }

    /* set page directory for loading program data */
    set_cr3((uint32_t)task->page_dir);

    /* load program from memory */
    simple_elf_t elf_header;
    ret += elf_load_helper(&elf_header, fname);
    ret += load_program(&elf_header, task->maps);
    if (ret < 0) {
        task_destroy(task);
        return NULL;
    }

    thread_t *thread = thread_init();
    if (thread == NULL) {
        task_destroy(task);
        return NULL;
    }
    thread->task = task;
    thread->status = INITIALIZED;
    thread->ip = elf_header.e_entry;

    /* add thread into task's living thread list */
    add_node_to_head(task->live_thread_list, TCB_TO_LIST_NODE(thread));
    task->task_id = thread->tid;

    // register new task for simics symbolic debugging
    sim_reg_process(task->page_dir, fname);

    return thread;
}
