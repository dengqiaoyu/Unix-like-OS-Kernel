/** @file task.h
 *  @author Newton Xie (ncx)
 *  @bug No known bugs.
 */

#ifndef _TASK_H_
#define _TASK_H_

#include <stdint.h>
#include <elf_410.h>

#include "list.h"
#include "mutex.h"
#include "maps.h"

#define KERN_STACK_SIZE 0x1000

#define USER_STACK_LOW 0xFFB00000
#define USER_STACK_SIZE 0x1000
#define USER_STACK_START 0xFFB00E00

#define NODE_TO_TCB(thread_node) ((thread_t *)(thread_node->data))
#define TCB_TO_NODE(thread) ((thread_node_t *)((char *)thread - 8))

#define NODE_TO_PCB(task_node) ((task_t *)(task_node->data))
#define PCB_TO_NODE(task) ((task_node_t *)((char *)task - 8))

typedef node_t task_node_t;
typedef node_t thread_node_t;

typedef struct task {
    int task_id;
    int status;
    uint32_t *page_dir;
    map_list_t *maps;

    list_t *thread_list;
    mutex_t thread_list_mutex;

    struct task *parent_task;
    list_t *child_task_list;
    mutex_t child_task_list_mutex;

    list_t *zombie_task_list;
    list_t *waiting_thread_list;
    mutex_t wait_mutex;
} task_t;

typedef struct thread {
    int tid;
    int status;
    task_t *task;

    uint32_t kern_sp;
    uint32_t cur_sp;
    uint32_t ip;
} thread_t;

/* status define */
#define INITIALIZED 0
#define RUNNABLE 1
#define SUSPENDED 2
#define FORKED 3
#define BLOCKED_MUTEX 4

int id_counter_init();

int gen_thread_id();

task_t *task_init();

int task_lists_init(task_t *task);

void task_lists_destroy(task_t *task);

int task_mutexes_init(task_t *task);

void task_mutexes_destroy(task_t *task);

uint32_t *page_dir_init();

thread_t *thread_init();

int load_program(simple_elf_t *header, map_list_t *maps);

int load_elf_section(const char *fname, unsigned long start, unsigned long len,
                     long offset, int pte_flags);

#endif
