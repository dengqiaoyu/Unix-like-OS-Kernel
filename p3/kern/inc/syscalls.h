/** @file syscalls.h
 *
 */

#ifndef _SYSCALLS_H_
#define _SYSCALLS_H_

int kern_gettid(void);

void kern_exec(void);

int kern_fork(void);

int kern_new_pages(void);

int kern_wait(void);

void kern_vanish(void);

void kern_set_status(void);

#endif
