/** @file create_test.c
 *
 *  @brief simple test of thread creation
 */

#include <thread.h>
#include <syscall.h>
#include <simics.h>
#include <stdio.h>
#include "410_tests.h"
#include <report.h>

DEF_TEST_NAME("startle:");

void *child(void *param);

#define STACK_SIZE 3072

#define NTHREADS 16
#define SOMETIMES 8

int ktids[NTHREADS];

/** @brief Create NTHREADS threads, and test that they all run. */
int
main(int argc, char *argv[])
{
  int t, done = 0;

  thr_init(STACK_SIZE);
  report_start(START_CMPLT);

  for (t = 0; t < NTHREADS; ++t) {
    (void) thr_create(child, (void *)t);
    if (t % SOMETIMES == 0)
      yield(-1);
  }

  while (!done) {
    int nregistered, slot;

    for (nregistered = 0, slot = 0; slot < NTHREADS; ++slot)
      if (ktids[slot] != 0)
        ++nregistered;

    if (nregistered == NTHREADS)
      done = 1;
    else
      sleep(1);
  }

  printf("Success!\n"); lprintf("Success!\n");
  report_end(END_SUCCESS);

  task_vanish(0);
}

/** @brief Declare that we have run, then twiddle thumbs. */
void *
child(void *param)
{
  int slot = (int) param;

  ktids[slot] = gettid();
  printf("%d\n", ktids[slot]);

  while (1) {
    yield(-1);
    sleep(10);
  }
}
