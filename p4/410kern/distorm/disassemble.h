/** @file disassemble.h
 *  @author Rob Arnold (rdarnold) S'2009
 *  @bugs None known
 */

#ifndef _DISASSEMBLE_H_
#define _DISASSEMBLE_H_

#include <stddef.h>

// Maximum encoded x86 instruction size in bytes
// Determined via newsgroup post
#define MAX_INS_LENGTH 15

size_t disassemble(void *instruction, size_t codeLen, char *buffer, int length);

#endif /* _DISASSEMBLE_H_ */
