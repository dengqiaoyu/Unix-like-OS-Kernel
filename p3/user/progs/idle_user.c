/** @file 410user/progs/idle.c
 *  @author ?
 *  @brief Idle program.
 *  @public yes
 *  @for p2 p3
 *  @covers
 *  @status done
 */

#include <stdio.h>
#include <simics.h>   /* lprintf() */

int main() {
    int i = 0;
    while (1) {
        i++;
        if (i % 1000000 == 0)
            lprintf("idle : %d\n", i);
    }
}
