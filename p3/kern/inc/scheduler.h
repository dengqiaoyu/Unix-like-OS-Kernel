#ifndef __H_SCHEDULER__
#define __H_SCHEDULER__

#include <mutex.h>

#include "list.h"
#include "task.h"

#define GET_TCB_FROM_TAB(tcb_tb_node)\
        ((thread_t *)((tcb_tb_node)->data))
#define GET_TCB_FROM_SCHE(sche_node)\
        ((thread_t *)(((tcb_tb_node_t *)((sche_node)->data))->data))
#define GET_SCHE_NODE_FROM_TCB(tcb_ptr)\
        ((sche_node_t *)(((void *)(tcb_ptr)) - 16))
#define GET_TAB_NODE_FROM_TCB(tcb_ptr)\
        ((tcb_tb_node_t *)(((void *)(tcb_ptr)) - 8))

typedef struct schedule_list_struct schedule_t;

struct schedule_list_struct {
    list_t active_list;
};

typedef node_t sche_node_t;
typedef node_t tcb_tb_node_t;

int scheduler_init();
void set_cur_run_thread(thread_t *tcb_ptr);
void sche_yield(int if_suspend);
thread_t *get_cur_tcb();
void sche_push_back(thread_t *tcb_ptr);
void sche_push_front(thread_t *tcb_ptr);

#endif
