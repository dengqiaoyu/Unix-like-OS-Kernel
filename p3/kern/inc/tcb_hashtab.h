#ifndef _TCB_HASHTAB_H_
#define _TCB_HASHTAB_H_

#include <mutex.h>
#include "task.h"
#include "list.h"

#define HASH_LEN 8

typedef struct tcb_hashtab {
    list_t *tcb_list[HASH_LEN];
    mutex_t mutex[HASH_LEN];
} tcb_hashtab_t;

int tcb_hashtab_init();
void tcb_hashtab_put(thread_t *node);
thread_t *tcb_hashtab_get(int tid);
int tcb_hashtab_rmv(int tid);

#endif /* _TCB_HASHTAB_H_ */