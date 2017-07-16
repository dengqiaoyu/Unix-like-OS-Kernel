/** @file traceback.c
 *  @brief The traceback function that can handle with wild pointers or other
 *         unstardand stack frames
 *  @author Qiaoyu Deng
 *  @andrew_id qdeng
 *  @bug When the caller of traceback itself encounter a segmentation fault, the
 *       fault will be catched by traceback's segmentation handler instead of
 *       itself, so this might be some problem.
 */

/* -- Includes -- */

/* libc includes. */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <setjmp.h> /* for setjmp() and longjmp() */
#include <signal.h> /* for struct sigaction and sigaction() */

/* User defined includes */
#include "util_asm.h" /*.for get_ebp() */
#include "util.h"
#include "traceback_internal.h" /* for predefinitions and struct functsym_t */

jmp_buf recover; /* Used by setjmp() and longjmp() */

void segv_handler(int sig) {
    /* It will return 1 to where setjmp() was first called. */
    longjmp(recover, 1);
}

void traceback(FILE *fp) {
    struct sigaction segv;
    struct sigaction segv_def;
    segv.sa_handler = segv_handler;
    /* Use our own define segv handler */
    sigaction(SIGSEGV, &segv, &segv_def);
    if (setjmp(recover)) {
        /*
         * If we can get there, it means that we have return from signal handler
         * and encounter a segmentation fault.
         */
        /* Restore the default signal handler.
         */
        sigaction(SIGSEGV, &segv_def, &segv);
        fwrite("\nFATAL\n\0", sizeof(char), 8, fp);
        return;
    }

    /* The lowest and highest address of function in the function table */
    void *lowest_addr = NULL;
    void *highest_addr = NULL;
    int func_num = 0;
    void *ebp = NULL;
    void *caller_ret_addr = NULL;
    char ifn_hit_main = 1;

    get_addr_range_and_func_num(&lowest_addr, &highest_addr, &func_num);

    ebp = get_ebp();
    while (ifn_hit_main) {
        void *old_ebp = NULL;
        /* for storing the index of given function */
        int index = 0;
        char func_name[FUNCTS_MAX_NAME + 1] = {'\0'};
        char argv_description[MAX_ARGV_DESCRIPTION + 1] = {'\0'};
        char print_line[MAX_LINE + 1] = {'\0'};
        int print_line_len = 0;
        int ret = 0;

        if (ebp == NULL)
            break;
        caller_ret_addr = get_caller_ret_addr(ebp);
        if (caller_ret_addr == ebp) {
            fwrite("\nFATAL\n\0", sizeof(char), 8, fp);
            return;
        }

        index = get_func_name(func_name,
                              caller_ret_addr,
                              lowest_addr,
                              highest_addr,
                              func_num);
        if (!strncmp(func_name, "__libc_start_main", FUNCTS_MAX_NAME)) {
            ifn_hit_main = 0;
        }

        /* Get the previous ebp for reading arguements */
        old_ebp = get_old_ebp(ebp);
        if (old_ebp == ebp) {
            fwrite("\nFATAL\n\0", sizeof(char), 8, fp);
            return;
        }

        get_argv_description(index, old_ebp, argv_description);

        if (ifn_hit_main) {
            snprintf(print_line, MAX_LINE, "Function %s%s, in\n",
                     func_name, argv_description);
        } else {
            snprintf(print_line, MAX_LINE, "Function %s%s\n",
                     func_name, argv_description);
        }

        print_line_len = strlen(print_line);
        ret = fwrite(print_line, sizeof(char), print_line_len, fp);
        if (ret < print_line_len) {
            fwrite("\nFATAL\n", sizeof(char), 8, fp);
            return;
        }

        ebp = get_old_ebp(ebp);
    }
    return;
}


