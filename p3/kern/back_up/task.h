#ifndef __H_TASK__
#define __H_TASK__

#include <stdint.h>
#include "list.h"

typedef struct task_struct task_t;
typedef struct thread_struct thread_t;
typedef struct contex_struct context_t;

struct contex_struct {
    /* Do we need store registers? */
    int errno;
    void *eip;
    uint16_t cs;
    uint16_t padding_0;
    unsigned int eflags;
    void *esp;
    uint16_t ss;
    uint16_t padding_1;
};

struct task_struct {
    unsigned int pid;
    unsigned int parent_pid;
    void *kernel_stack;
    list_t child_thr_list;
    void *page_dir_base;
    thread_t *thr;
    list_t heap_list;
};

struct thread_struct {
    unsigned int tid;
    context_t contex;
    void *kernel_stack;
};

#endif /* __H_TASK__ */