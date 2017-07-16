#ifndef _SYSCALLS_H_
#define _SYSCALLS_H_

#include <stdint.h>
#include "life_cycle.h"
#include "thread_management.h"
#include "memory_management.h"
#include "console_io.h"

uint32_t asm_get_esi(void);

#endif