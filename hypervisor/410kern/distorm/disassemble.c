/** @file disassemble.c
 *  @brief This file provides a simple wrapper for diStorm
 *  @author Rob Arnold (rdarnold) S'2009
 *  @bugs None known
 */

#include <stdio.h>
#include "disassemble.h"
#include "distorm.h"

// distorm wants at least 15 instructions it could write into...why?
#define INSTRUCTION_BUFFER 15

size_t disassemble(void *instruction, size_t codeLen, char *buffer, int length) {

	if (!instruction || !length) return 0;

	static _DecodedInst results[INSTRUCTION_BUFFER];
	_OffsetType codeOffset = 0;
	unsigned int usedInstructions;

	if (DECRES_SUCCESS != distorm_decode(codeOffset, instruction, codeLen, Decode32Bits, results, INSTRUCTION_BUFFER, &usedInstructions))
		return 0;

	_DecodedInst *result = &results[0];

	// No way to signal overflow here
	(void)snprintf(buffer, length, "%s %s", result->mnemonic.p, result->operands.p);

	return result->size;
}
