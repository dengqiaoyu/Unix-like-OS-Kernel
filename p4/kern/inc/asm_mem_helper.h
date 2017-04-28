#ifndef _ASM_MEM_HELPER_H_
#define _ASM_MEM_HELPER_H_

#include <stdlib.h>

uint16_t asm_get_gs();

void asm_set_gs(uint16_t gs_value);

void asm_copy_dw_from_gs(uint8_t *host_dest, uint8_t *guest_src);

void asm_copy_b_from_gs(uint8_t *host_dest, uint8_t *guest_src);

void asm_copy_dw_to_gs(uint8_t *guest_dest, uint8_t *host_src);

void asm_copy_b_to_gs(uint8_t *guest_dest, uint8_t *host_src);

#endif /* _ASM_MEM_HELPER_H_ */