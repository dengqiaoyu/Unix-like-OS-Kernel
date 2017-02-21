#ifndef __COND_TYPE_INTERNAL_H__
#define __COND_TYPE_INTERNAL_H__

#include <mutex.h>
#include "list.h"

typedef list_t wait_list_t;
typedef node_t wait_list_node_t;
#define SUCCESS 0
#define COND_INIT_FAIL -1

typedef struct wait_list_item_t {
    int tid;
    int is_not_runnable;
} wait_list_item_t;

wait_list_node_t *cond_enq(wait_list_t *wait_list, int tid);
wait_list_item_t *cond_deq(wait_list_t *wait_list);

#endif /* __COND_TYPE_INTERNAL_H__ */