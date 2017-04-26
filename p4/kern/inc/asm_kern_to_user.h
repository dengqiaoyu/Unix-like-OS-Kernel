/** @file asm_kern_to_user.h
 *
 */

#ifndef _ASM_SWITCH_H_
#define _ASM_SWITCH_H_
#include <stdlib.h>

void kern_to_user(uint32_t sp, uint32_t ip);

#endif
