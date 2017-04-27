#include <syscall.h>
#include <stdlib.h>
#include <simics.h>
#include "410_tests.h"
#include <report.h>
#include <stdio.h>

DEF_TEST_NAME("readline_basic:");

int main() {
    char ch = 0;
    while (1) {
        ch = getchar();
        printf("%c", ch);
    }
}
