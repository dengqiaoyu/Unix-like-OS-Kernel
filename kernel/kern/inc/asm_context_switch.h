#ifndef _ASM_CONTEXT_SWITCH_H_
#define _ASM_CONTEXT_SWITCH_H_

#include <stdint.h>

void asm_switch_to_runnable(uint32_t *cur_sp, uint32_t new_sp);
void asm_switch_to_initialized(uint32_t *cur_sp,
                               uint32_t new_sp,
                               uint32_t new_ip);
void asm_switch_to_forked(uint32_t *cur_sp,
                          uint32_t new_sp,
                          uint32_t new_ip);

#endif
