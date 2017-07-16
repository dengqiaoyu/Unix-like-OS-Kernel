/**
 *  @Field util.h
 *  @brief Funtion prototype that are used in treaceback.c
 *
 *  This file should only be included by the files that form the traceback
 *  library.
 *  @author Qiaoyu Deng
 *  @andrew_id qdeng
 *  @bug No known bugs
 */

#ifndef UTIL_H
#define UTIL_H

/* string length definitions */
#define MAX_LINE 4096               /* The maxmum length of one line traceback
        can print */
#define MAX_ARGV_DESCRIPTION 2048    /* The maxmum length of arguement
        description*/
#define MAX_ITEM_DESCRIPTION 256    /* The maxmum length of each type-value pair
        item description */
#define MAX_VALUE_DESCRIPTION 128   /* The maxmum length of value description */
#define MAX_TYPE_DESCRIPTION 16     /* The maxmum length of type description */
#define MAX_PRINT_STR_ARRAY_NUM 3   /* The maxmum length of string array that
        can be printed */
#define MAX_PRINT_STR_LEN 25        /* The maxmum length of string that can be
        printed */

/* Function prototype definitions */

/**
 * Returns the calller's return address that push this epb address.
 * @param  ebp current ebp pointer.
 * @return     caller's return address, or the original input of ebp when cannot
 *             get the caller's return address.
 */
inline void *get_caller_ret_addr(void *ebp);

/**
 * Returns the last frame's ebp address.
 * @param  ebp current ebp pointer.
 * @return     last frame's ebp address, or the original input of ebp when
 *             cannot get last frame's return address.
 */
inline void *get_old_ebp(void *ebp);

/**
 * Get function table's lowest address and highest address and the number of
 * functions it contains.
 * @param loweset_addr_ptr the pointer of address of lowest address in the
 *                         table.
 * @param highest_addr_ptr the pointer of address of highest address in the
 *                         table.
 * @param func_num         the pinter of number of functions in the table.
 */
void get_addr_range_and_func_num(void **loweset_addr_ptr,
                                 void **highest_addr_ptr,
                                 int *func_num);

/**
 * Get the name and its index of function in the function table given
 * function's return address.
 * @param  func_name       the pointer of function name string.
 * @param  caller_ret_addr the return address of given address.
 * @param  loweset_addr    the address of lowest address in the
 *                         table.
 * @param  highest_addr    the address of highest address in the
 *                         table.
 * @param  func_num        the number of functions in the table.
 * @return                 the index of given function in the table, or -1 when
 *                         no function found in the table has the given return
 *                         address.
 */
int get_func_name(char *func_name,
                  void *caller_ret_addr,
                  void *loweset_addr,
                  void *highest_addr,
                  int func_num);

/**
 * Search the function name given its return address.
 * @param  addr_rover the address in the function.
 * @param  right      the start of index.
 * @param  left       the end of index.
 * @return            the index of given address on success, or -1 when does not
 *                    find any function that has given address.
 */
int func_search(void *addr_rover, int right, int left);

/**
 * Get the full description of given function's arguements, like:
 * (char c='k', char *str="test").
 * @param index            the index of given function.
 * @param old_ebp          the previous ebp of current frame.
 * @param argv_description the argurment description to be returned.
 */
void get_argv_description(int index, void *old_ebp, char *argv_description);

/**
 * Get the type name of each arguement given its type type definition code.
 * @param type_name the type name to be returned.
 * @param type_code the type definition code.
 */
inline void get_type_name(char *type_name, int type_code);

/**
 * Get the valid name string given the pointer of arguement name string in the
 * table.
 * @param argv_name the arguement name to be returned, or "unprintable" when its
 *                  name has some unprintable charachters.
 * @param name      the pointer of arguement name string in the table.
 */
inline void get_argv_name(char *argv_name, char *name);

/**
 * Get the valid value string description of given arguement.
 * @param value_str value string description to be returned.
 * @param value_ptr pointer of given value.
 * @param type      the type definition code.
 */
void get_value_str(char *value_str, void *value_ptr, int type);

/**
 * Check the validation of given memory address.
 * @param  addr the memory address.
 * @return      1 as valid, or 0 as invalid.
 */
inline int is_readable(void *addr);

#endif