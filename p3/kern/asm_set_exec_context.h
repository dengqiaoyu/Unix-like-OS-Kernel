#ifndef _ASM_SET_EXEC_CONTEXT_H_
#define _ASM_SET_EXEC_CONTEXT_H_

#include <stdint.h>

void asm_set_exec_context(uint32_t old_kern_sp,
                          uint32_t new_kern_sp,
                          uint32_t *new_curr_esp_ptr,
                          uint32_t *new_curr_eip_ptr);

#endif /* _ASM_SET_EXEC_CONTEXT_H_ */