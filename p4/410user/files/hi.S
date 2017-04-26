/* hi
 * cc -m32 -DASSEMBLER -c hi.S
 * ld -T kernel.lds -static -Ttext 100000 --fatal-warnings -melf_i386 -o hi hi.o
 */

#define SIM_PUTS            0x04100002

.text
    .long 0xCCCCCCCC

.global _start
_start:
    leal hi, %ecx
    movl $SIM_PUTS, %ebx
    xchg %ebx, %ebx
1:
	jmp 1b

.section .rodata
hi:
    .asciz "Hi!"

.data
    .long 0xCCCCCCCC

.bss
    .long 0x00000000
