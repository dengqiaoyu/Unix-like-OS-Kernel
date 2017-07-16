#include <stdio.h>
#include "traceback.h"

void bar(int x, int y) {
    int z;
    z = x + y;
    (void) z;
    traceback(stdout);
}

void foo(char **argu1) {
    bar (5, 17);
}

int main (int argc, char **argv) {
    // char *arg1[] = {"foo", "bar", "baz", "bletch"};
    // char *arg2[] = {"this string has more than 25 characters",
    //                 "this string has more than.",
    //                 "this string has more than..."
    //                };
    // char *sadkf[] = {"this string has \1ore than 25 characters",
    //                  "this string has \4ore than",
    //                  "this string has more than"
    //                 };
    // char *sadkf1[] = {"this string has \1ore than 25 characters",
    //                   "this string has more than ",
    //                   "this string has more than"
    //                  };

    char *arg1[] = {"this string has more than 25 characters"};
    foo(arg1);
    return 0;
}
