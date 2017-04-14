/** @file syscalls.h
 *
 */

#ifndef _SYSCALLS_H_
#define _SYSCALLS_H_

int kern_fork(void);

int kern_exec(void);

int kern_wait(void);

int kern_yield(void);

int kern_deschedule(void);

int kern_make_runnable(void);

int kern_gettid(void);

int kern_new_pages(void);

int kern_remove_pages(void);

int kern_sleep(void);

int kern_readline(void);

int kern_print(void);

int kern_set_term_color(void);

int kern_set_cursor_pos(void);

int kern_get_cursor_pos(void);

int kern_thread_fork(void);

unsigned int kern_get_ticks(void);

void kern_halt(void);

void kern_set_status(void);

void kern_vanish(void);

// software exception handler

#endif
