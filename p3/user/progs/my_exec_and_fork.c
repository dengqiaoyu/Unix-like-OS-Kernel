#include <stdio.h>
#include <simics.h>   /* lprintf() */
#include <syscall.h>

int main() {
    exec("my_exec_and_fork_helper", NULL);
}