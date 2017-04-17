#include <stdio.h>
#include <simics.h>   /* lprintf() */
#include <syscall.h>

int main() {
    char test[] = "my_exec_and_fork_helper";
    char *args[] = {"my_exec_and_fork_helper", 0};
    exec(test, args);
    return 0;
}