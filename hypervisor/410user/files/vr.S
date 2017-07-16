/* vr
 * cc -m32 -DASSEMBLER -c vr.S
 * ld -T kernel.lds -static -Ttext 100000 --fatal-warnings -melf_i386 -o vr vr.o
 */

#define CONSOLE_MEM_BASE 0xB8000

.text
.global _start
_start:
    movl nbytes, %ecx                  # ECX: count
    leal bytes, %esi                   # ESI: source
    movl $CONSOLE_MEM_BASE, %edi       # EDI: dest
    rep movsb                          # ECX bytes from DS:ESI to ES:EDI
1:
	jmp 1b

.section .rodata
nbytes:
    .long (endbytes - bytes)
bytes:
	.asciz "H\re\rl\rp\r!\r \r \rI\r'\rm\r \rt\rr\ra\rp\rp\re\rd\r \ri\rn\r \ra\r \rP\rD\rP\r \r1\r1\r/\r7\r0\r!\r"
endbytes:

.data
    .long 0xCCCCCCCC

.bss
    .long 0x00000000
