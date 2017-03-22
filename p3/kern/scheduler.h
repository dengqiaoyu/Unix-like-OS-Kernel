#ifndef __H_SCHEDULER__
#define __H_SCHEDULER__

#include "list.h"

typedef struct schedule_list_struct schedule_t;

struct schedule_list_struct {
    list_t active_list;
    list_t deactive_list;
};

#endif