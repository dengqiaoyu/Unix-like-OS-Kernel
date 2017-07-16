#include <syscall.h>
#include <stdlib.h>
#include <simics.h>
#include <stdio.h>

int main() {
    char ch = 0;
    while (1) {
        ch = getchar();
        printf("%c", ch);
    }
}
