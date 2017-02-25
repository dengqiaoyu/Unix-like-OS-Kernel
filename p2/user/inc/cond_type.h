/**
 * @file cond_type.h
 * @brief This file defines and structure the type for condition variables.
 * @author Newton Xie(nwx) Qiaoyu Deng(qdeng)
 * @bug No known bugs
 */

#ifndef __COND_TYPE_H__
#define __COND_TYPE_H__
#include <mutex.h>
#include "cond_type_internal.h"
#include "return_type.h"

typedef struct cond_t {
    wait_list_t wait_list; /* queue list */
    mutex_t mutex;         /* mutex that is used to protect the lsit*/
    int is_act;            /* indicate whether the condition variables active */
} cond_t;

#endif /* __COND_TYPE_H__ */
