#ifndef _TCB_HASHTAB_H_
#define _TCB_HASHTAB_H_

#include "utils/kern_mutex.h"
#include "utils/list.h"
#include "task.h"


#define HASH_LEN 8

typedef struct tcb_hashtab {
    list_t *tcb_list[HASH_LEN];
    kern_mutex_t mutex[HASH_LEN];
} tcb_hashtab_t;

int tcb_hashtab_init();
void tcb_hashtab_put(thread_t *tcb);
thread_t *tcb_hashtab_get(int tid);
void tcb_hashtab_rmv(thread_t *tcb);

#endif /* _TCB_HASHTAB_H_ */
