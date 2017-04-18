/** @file exceptions.h
 *
 */

#ifndef _EXCEPTIONS_H_
#define _EXCEPTIONS_H_

#define NO_ERROR_CODE 0
#define ERROR_CODE 1

void pagefault_handler();

void hwerror_handler(int cause, int ec_flag);

void exn_handler(int cause, int ec_flag);

#endif
