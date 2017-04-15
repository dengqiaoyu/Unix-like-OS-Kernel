#ifndef _CONSOLE_IO_H_
#define _CONSOLE_IO_H_

#define COLOR_MASK 0x7F

int kern_readline(void);

int kern_print(void);

int kern_set_term_color(void);

int kern_set_cursor_pos(void);

int kern_get_cursor_pos(void);

#endif /* _CONSOLE_IO_H_ */