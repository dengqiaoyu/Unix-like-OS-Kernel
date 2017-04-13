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

int kern_readline(void);

void kern_set_status(void);

void kern_vanish(void);

#endif
