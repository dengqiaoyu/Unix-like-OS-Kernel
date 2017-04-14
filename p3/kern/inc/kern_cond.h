#ifndef _KERN_COND_H_
#define _KERN_COND_H_

#include <mutex.h>
#include "list.h"
#include "task.h"

typedef list_t kcond_wlist_t;

typedef struct kern_cond {
    char is_active;
    mutex_t mutex;
    kcond_wlist_t *wait_list;
} kern_cond_t;

typedef struct kcond_wlist_node {
    node_t node;
    int tid;
} kcond_wlist_node_t;

int kern_cond_init(kern_cond_t *cv);

void kern_cond_wait(kern_cond_t *cv, mutex_t *mp);

void kern_cond_signal(kern_cond_t *cv);

void kern_cond_destroy(kern_cond_t *cv);

#endif