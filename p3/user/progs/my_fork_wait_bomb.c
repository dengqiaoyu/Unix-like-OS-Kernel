/** @file 410user/progs/fork_wait_bomb.c
 *  @author ?
 *  @brief Tests fork, wait, and exit in stress conditions.
 *  @public yes
 *  @for p3
 *  @covers fork wait set_status vanish
 *  @status done
 */

#include <syscall.h>
#include <stdlib.h>
#include "410_tests.h"
#include <simics.h>
#include <report.h>

DEF_TEST_NAME("fork_wait_bomb:");
#define PRINT_LINE (lprintf("in line %d", __LINE__));

int main(int argc, char *argv[]) {
    int pid = 0;
    int count = 0;
    int ret_val;
    int wpid;

    report_start(START_CMPLT);
    report_fmt("parent: %d", gettid());

    while (count < 1000) {
        // PRINT_LINE;
        if ((pid = fork()) == 0) {
            exit(42);
        }
        // PRINT_LINE;
        if (pid < 0) {
            break;
        }
        // PRINT_LINE;
        count++;
        // PRINT_LINE;
        report_fmt("child: %d", pid);
        // PRINT_LINE;
        wpid = wait(&ret_val);
        // PRINT_LINE;
        if (wpid != pid || ret_val != 42) {
            report_end(END_FAIL);
            exit(42);
        }
        // PRINT_LINE;
    }

    report_end(END_SUCCESS);
    exit(42);
}
