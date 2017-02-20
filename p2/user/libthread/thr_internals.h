/** @file thr_internals.h
 *
 *  @brief This file may be used to define things
 *         internal to the thread library.
 */

#ifndef THR_INTERNALS_H
#define THR_INTERNALS_H

int start_thread( void *stackaddr, void *(*func)(void *), void *args );
void wrap_thread_proc(void *(*func)(void *), void *args);

#endif /* THR_INTERNALS_H */
