#include <syscall.h>
#include <stdio.h>
#include <simics.h>

int main() {
    int pid, exitstatus;
    char test[] = "shell";
    /*
    char test[] = "actual_wait";
    char test[] = "wild_test1";
    char test[] = "remove_pages_test2";
    char test[] = "cho";
    char test[] = "fork_exit_bomb";
    char test[] = "fork_wait_bomb";
    char test[] = "actual_wait";
    char test[] = "make_crash";
    char test[] = "mem_permissions";
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
