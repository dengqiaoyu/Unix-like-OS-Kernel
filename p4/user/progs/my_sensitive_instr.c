#include <syscall.h>
#include <stdlib.h>
#include <simics.h>
#include <stdio.h>

int main() {
    // to simulate disable_interrupt
    // asm("cli");
    // to simulate outb
    // unsigned short cycles_per_interrupt =
    //    (TIMER_RATE / (MS_PER_S / MS_PER_INTERRUPT));
    //    MS_PER_INTERRUPT is set to 20ms
    /* outb(TIMER_MODE_IO_PORT, TIMER_SQUARE_WAVE); */
    lprintf("outb(TIMER_MODE_IO_PORT, TIMER_SQUARE_WAVE)");
    asm("movw $0x43, %dx");
    asm("movb $0x36, %al");
    asm("out %ax, %dx");

    MAGIC_BREAK;
    /* outb(TIMER_PERIOD_IO_PORT, LSB(cycles_per_interrupt)); */
    lprintf("outb(TIMER_PERIOD_IO_PORT, LSB(cycles_per_interrupt)");
    asm("movw $0x40, %dx");
    asm("movb $0x37, %al");
    asm("out %ax, %dx");
    /* outb(TIMER_PERIOD_IO_PORT, MSB(cycles_per_interrupt)); */
    lprintf("outb(TIMER_PERIOD_IO_PORT, MSB(cycles_per_interrupt)");
    asm("movw $0x40, %dx");
    asm("movb $0x5d, %al");
    asm("out %ax, %dx");

    printf("I am virtualized kernel!\n");
}
