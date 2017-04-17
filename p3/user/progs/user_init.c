#include <syscall.h>
#include <stdio.h>
#include <simics.h>

int main() {
    int pid, exitstatus;
    char test[] = "shell";
    /*
    char test[] = "cho";
    char test[] = "fork_exit_bomb";
    char test[] = "fork_wait_bomb";
    char test[] = "actual_wait";
    char test[] = "stack_test1";
    */
    char * args[] = {test, 0};

    while (1) {
        pid = fork();
        if (!pid)
            exec(test, args);

        while (pid != wait(&exitstatus));

        lprintf("Shell exited with status %d; starting it back up...",
                exitstatus);
        MAGIC_BREAK;
    }
}
