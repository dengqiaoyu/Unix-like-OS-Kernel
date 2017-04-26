/* spin2
 * cc -m32 -DASSEMBLER -c spin2.S
 * ld -T kernel.lds -static -Ttext 100000 --fatal-warnings -melf_i386 -o spin2 spin2.o
 */

.text
    .long 0xCCCCCCCC

.global _start
_start:
    movl $_start,%eax
    cli
    jmp *%eax

.section .rodata
    .long 0xCCCCCCCC
.data
    .long 0xCCCCCCCC
.bss
    .long 0x00000000
