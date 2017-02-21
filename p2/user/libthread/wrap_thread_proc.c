/** @file wrap_thread_proc.c
 *
 */

#include <thread.h>

void wrap_thread_proc(void *(*func)(void *), void *args) {
    void *status = func(args);
    thr_exit(status);
}
