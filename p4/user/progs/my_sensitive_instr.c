#include <syscall.h>
#include <stdlib.h>
#include <simics.h>
#include <stdio.h>

int main() {
    asm("cli");
    printf("I am virtualized kernel!\n");
}
