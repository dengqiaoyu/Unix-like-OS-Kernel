/** @file verify_test.c
 *
 * Test output format for the traceback function
 *
 * This test calls a few functions to test a bunch
 * of different outputs.
 *
 * @author Michael Ashley-Rollman(mpa)
 */

#include "traceback.h"

void f4(int i, float f) {
    traceback(stdout);
}

void f3(char c, char *str) {
    f4(5, 35.0);
}

void f2(void) {
    f3('k', "test");
}

void f1(char *array, char ** array1, char ** array2, char ** array3, char **array4, char **array5, char **array6) {
    f2();
}

int main() {
    char arg1[39] = "this string has more than 25 characters";
    char *arg2[] = {"this string has more than 25 characters",
                    "this string has more than.",
                    NULL
                   };
    char *sadkf[] = {"this string has \1ore than 25 characters",
                     "this string has \4ore than"
                    };
    char *sadkf1[] = {"this string has \1ore than 25 characters",
                      "this string has more than ",
                      NULL
                     };
    char *sadkf2[] = {"this string has \1ore than 25 characters"
                     };
    char *sadkf3[] = {"this string has \1ore than 25 characters",
                     };
    char *sadkf4[] = {"foo", "bar", "baz", "bletch"};

    f1(arg1, arg2, sadkf, sadkf1, sadkf2, sadkf3, sadkf4);

    return 0;
}
