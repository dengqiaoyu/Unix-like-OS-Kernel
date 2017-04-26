/* byenow
 * cc -m32 -DASSEMBLER -c byenow.S
 * ld -T kernel.lds -static -Ttext 100000 --fatal-warnings -melf_i386 -o byenow byenow.o
 */

.text
    .long 0xCCCCCCCC
    .long 0xCCCCCCCC
    .long 0xCCCCCCCC
    .long 0xCCCCCCCC
.global _start
_start:
    xorl %eax,%eax
    cli
    hlt
.section .rodata
    .long 0xCCCCCCCC
.data
    .long 0xCCCCCCCC
.bss
    .long 0x00000000
