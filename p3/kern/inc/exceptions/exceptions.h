/** @file exceptions.h
 *
 */

#ifndef _EXCEPTIONS_H_
#define _EXCEPTIONS_H_

#define NO_ERROR_CODE 0
#define ERROR_CODE 1
#define ERROR_CODE_P (1)
#define ERROR_CODE_WR (1 << 1)
#define ERROR_CODE_US (1 << 2)
#define ERROR_CODE_RSVD (1 << 3)


void pagefault_handler();

void hwerror_handler(int cause, int ec_flag);

void exn_handler(int cause, int ec_flag);

#endif
