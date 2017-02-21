/** @file cond_type.h
 *  @brief This file defines the type for condition variables.
 */

#ifndef __COND_TYPE_H__
#define __COND_TYPE_H__
#include <mutex.h>
#include "cond_type_internal.h"

typedef struct cond_t {
    wait_list_t *wait_list;
    mutex_t mutex;
    int is_act;
} cond_t;

#endif /* __COND_TYPE_H__ */
