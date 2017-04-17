#ifndef __H_SCHEDULER__
#define __H_SCHEDULER__

#include <mutex.h>

#include "list.h"
#include "task.h"
#include "task_internal.h"

#define GET_TCB_TB_NODE(sche_node)\
        ((tcb_tb_node_t *)((sche_node)->data))
#define GET_TCB(sche_node)\
        ((thread_t *)(((thread_node_t*)(((tcb_tb_node_t *)((sche_node)->data))->data))->data))
#define GET_SCHE_NODE(thread) ((sche_node_t *)((char *)thread - 24))

typedef struct schedule_list_struct {
    list_t *active_list;
    list_t *deactive_list;
    mutex_t sche_list_mutex;
} schedule_t;

typedef node_t sche_node_t;
typedef node_t tcb_tb_node_t;

int scheduler_init();

void append_to_scheduler(sche_node_t *sche_node);

sche_node_t *pop_scheduler_active();

void sche_yield();

#endif
