/** @file user/progs/print_test.c
 *  @brief Initial program.
 *  @covers print
 */

#include <syscall.h>
#include <stdio.h>

int main()
{
  int plug = 3;

  printf("1 + 2 = %d; feels good man...", plug);
}
