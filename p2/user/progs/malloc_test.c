/** @file user/progs/malloc_test.c
 *  @brief Initial program.
 *  @covers malloc free
 */

#include <syscall.h>
#include <stdio.h>
#include <stdlib.h>

#define A_SIZE 4
#define B_SIZE 8

int main()
{
  int *array = (int *)malloc(A_SIZE * sizeof(int));
  int i;
  for (i = 0; i < A_SIZE; i++) {
    array[i] = i;
    printf("array[%d] = %d\n", i, array[i]);
  }

  array = (int *)realloc((void *)array, B_SIZE * sizeof(int));
  for (i = A_SIZE; i < B_SIZE; i++) {
    array[i] = i;
  }
  for (i = 0; i < B_SIZE; i++) {
    printf("array[%d] = %d\n", i, array[i]);
  }

  free(array);

  array = (int *)calloc(A_SIZE, sizeof(int));
  for (i = 0; i < A_SIZE; i++) {
    printf("array[%d] = %d\n", i, array[i]);
  }

  free(array);
}
