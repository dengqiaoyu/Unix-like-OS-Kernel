/** @file task.h
 *  @author Newton Xie (ncx)
 *  @bug No known bugs.
 */

#ifndef _TASK_H_
#define _TASK_H_

#include <stdint.h>
#include <elf_410.h>
#include <mutex.h>
#include <maps.h>

#define KERN_STACK_SIZE 0x1000

#define USER_STACK_LOW 0xFFB00000
#define USER_STACK_SIZE 0x1000
#define USER_STACK_START 0xFFB00F00

typedef struct id_counter {
    int task_id_counter;
    mutex_t task_id_counter_mutex;
    int thread_id_counter;
    mutex_t thread_id_counter_mutex;
} id_counter_t;

typedef struct task {
    int task_id;
    uint32_t *page_dir;
    struct thread *main_thread;
    struct task *parent_task;
    unsigned int child_task_cnt;
    unsigned int thread_cnt;

    // list of threads

    // virtual memory mapped regions
    map_list_t *maps;
} task_t;

typedef struct thread {
    int tid;
    task_t *task;
    uint32_t kern_sp;
    uint32_t user_sp;
    uint32_t curr_esp;
    uint32_t ip;
    int status;
} thread_t;

/* status define */
#define INITIALIZED 0
#define RUNNABLE 1
#define SUSPENDED 2
#define FORKED 3
#define BLOCKED_MUTEX 4

int id_counter_init();

task_t *task_init(const char *fname);

thread_t *thread_init();

int load_program(simple_elf_t *header, map_list_t *maps);

int load_elf_section(const char *fname, unsigned long start, unsigned long len,
                     long offset, int pte_flags);

#endif
