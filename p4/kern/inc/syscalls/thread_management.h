#ifndef _THREAD_MANAGEMENT_H_
#define _THREAD_MANAGEMENT_H_

int kern_gettid(void);

int kern_yield(void);

int kern_deschedule(void);

int kern_make_runnable(void);

int kern_sleep(void);

int kern_swexn(void);

#endif
