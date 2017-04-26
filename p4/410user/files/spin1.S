/* spin1
 * cc -m32 -DASSEMBLER -c spin1.S
 * ld -T kernel.lds -static -Ttext 100000 --fatal-warnings -melf_i386 -o spin1 spin1.o
 */

.text

    .long 0xCCCCCCCC
    .long 0xCCCCCCCC
    .long 0xCCCCCCCC
    .long 0xCCCCCCCC
.global _start
_start:
    movl $_start,%eax
    jmp *%eax

.section .rodata
    .long 0xCCCCCCCC
.data
    .long 0xCCCCCCCC
.bss
    .long 0x00000000
