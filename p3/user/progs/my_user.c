#include <stdio.h>
#include <simics.h>   /* lprintf() */

int main() {
    int i = 0;
    while (1) {
        i++;
        if (i % 1000000 == 0)
            lprintf("my_user : %d\n", i);
    }
}