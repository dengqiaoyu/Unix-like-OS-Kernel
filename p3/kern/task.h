/** @file task.h
 *  @author Newton Xie (ncx)
 *  @bug No known bugs.
 */

#ifndef _TASK_H_
#define _TASK_H_

#include <stdint.h>
#include <elf_410.h>
#include <mutex.h>

#define USER_STACK_LOW 0xFFB00000
#define USER_STACK_HIGH 0xFFB02000
#define USER_STACK_SIZE 0x1000
#define KERN_STACK_SIZE 0x1000

struct thread;

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
    unsigned int child_cnt;

    // list of threads

    // virtual memory mapped regions
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
#define FORKED 1
#define FORKED_START 2
#define RUNNABLE 3
#define SUSPENDED 4

int id_counter_init();

task_t *task_init(const char *fname);

thread_t *thread_init();

int load_program(simple_elf_t *header, uint32_t *page_dir);

int load_elf_section(const char *fname, unsigned long start, unsigned long len,
                     long offset, int pte_flags);

#endif
