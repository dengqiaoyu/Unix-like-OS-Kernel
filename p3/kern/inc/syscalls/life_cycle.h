#ifndef _LIFE_CYCLE_H_
#define _LIFE_CYCLE_H_

int kern_fork(void);

int kern_thread_fork(void);

int kern_exec(void);

void kern_set_status(void);

void kern_vanish(void);

int kern_wait(void);

void kern_halt(void);

#endif