/**
 * @file util.c
 * @breif Function implementations that are used in traceback.c
 * @author Qiaoyu Deng
 * @andrew_id qdeng
 * @bug If there are some string arrays that do not end with NULL, and their
 *      length is less than 3, the traceback cannot tell whether the string
 *      arrays has come an end, so it might to read other memory does not
 *      belong to the data, and there is no way to detect whether it has
 *      overread the memory.
 */

/* -- Includes -- */

/* libc includes. */
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>   /* for sysconf(_SC_PAGESIZE) */
#include <ctype.h>    /* for isprint() */
#include <sys/mman.h> /* for mprotect() */

/* user defined includes */
#include "util.h"               /* for function prototypes and predefinitions */
#include "traceback_internal.h" /* for functsym_t functions[] */

inline void *get_caller_ret_addr(void *ebp) {
    if (!is_readable(ebp + sizeof(void *))) {
        return ebp;
    }
    return (*((void **)(ebp + sizeof(void *))));
}

inline void *get_old_ebp(void *ebp) {
    if (!is_readable(ebp)) {
        return ebp;
    }
    return *((void **)ebp);
}

void get_addr_range_and_func_num(void **lowest_addr_ptr,
                                 void **highest_addr_ptr,
                                 int *func_num) {
    int i = 0;
    while (strncmp(functions[i].name, "", FUNCTS_MAX_NUM)) {
        i++;
    }

    *lowest_addr_ptr = functions[0].addr;
    *highest_addr_ptr = functions[i - 1].addr;
    *func_num = i;

    return;
}

int get_func_name(char func_name[FUNCTS_MAX_NAME],
                  void *caller_ret_addr,
                  void *lowest_addr,
                  void *highest_addr,
                  int func_num) {
    void *seek_begin_addr = NULL;
    void *seek_end_addr = NULL;
    void *min_addr = NULL;
    void *addr_rover = NULL;

    if (caller_ret_addr == lowest_addr) {
        snprintf(func_name, FUNCTS_MAX_NAME, "%s", functions[0].name);
        return -1;
    } else if (caller_ret_addr == highest_addr) {
        snprintf(func_name, FUNCTS_MAX_NAME, "%s",
                 functions[func_num - 1].name);
    }

    min_addr = caller_ret_addr - MAX_FUNCTION_SIZE_BYTES;
    if (caller_ret_addr < lowest_addr || min_addr > highest_addr) {
        snprintf(func_name, FUNCTS_MAX_NAME, "%p", caller_ret_addr);
        return -1;
    }

    /*
     * Get reasonable search range of address.
     */
    if (caller_ret_addr > highest_addr && min_addr > lowest_addr) {
        seek_begin_addr = highest_addr;
        seek_end_addr = min_addr;
    } else if (caller_ret_addr > highest_addr && min_addr < lowest_addr) {
        seek_begin_addr = highest_addr;
        seek_end_addr = lowest_addr;
    } else if (caller_ret_addr < highest_addr && min_addr > lowest_addr) {
        seek_begin_addr = caller_ret_addr;
        seek_end_addr = min_addr;
    } else if (caller_ret_addr < highest_addr && min_addr < lowest_addr) {
        seek_begin_addr = caller_ret_addr;
        seek_end_addr = lowest_addr;
    }

    /*
     * From low address to high address to search the function name.
     */
    addr_rover = seek_begin_addr;
    for (; addr_rover >= seek_end_addr; addr_rover--) {
        int index = func_search(addr_rover, 0, func_num - 1);
        if (index != -1) {
            char is_print = 1;
            int i;

            /*
             * Check whether the name of function can be printed.
             */
            for (i = 0; i < FUNCTS_MAX_NAME; i++) {
                if (functions[index].name[i] == 0) {
                    break;
                }
                if (!isprint(functions[index].name[i])) {
                    is_print = 0;
                    break;
                }
            }

            if (is_print) {
                snprintf(func_name, FUNCTS_MAX_NAME, "%s",
                         functions[index].name);
            } else {
                snprintf(func_name, FUNCTS_MAX_NAME, "unprintable");
            }

            return index;
        }
    }

    snprintf(func_name, FUNCTS_MAX_NAME, "%p", caller_ret_addr);
    return -1;
}

int func_search(void *addr_rover, int right, int left) {
    /*
     * TODO use binary search.
     */
    int index = -1;
    int i;
    for (i = right; i <= left; i++) {
        if (addr_rover == functions[i].addr) {
            index = i;
            break;
        }
    }

    return index;
}

void get_argv_description(int index, void *ebp, char *argv_description) {
    /*
     * Function is not included in th table.
     */
    if (index == -1) {
        snprintf(argv_description, MAX_ARGV_DESCRIPTION, "(...)");
        return;
    }

    int description_len = 0;
    int len = 0;
    int i = 0;

    snprintf(argv_description, MAX_ARGV_DESCRIPTION - description_len, "(");
    description_len = 1;

    argsym_t *args_array = (argsym_t *)functions[index].args;
    while (strncmp(args_array[len].name, "", ARGS_MAX_NAME)) {
        len++;
        if (len == ARGS_MAX_NUM) {
            break;
        }
    }
    if (len == 0) {
        strncat(argv_description, "void)",
                MAX_ARGV_DESCRIPTION - description_len);
        return;
    }

    /*
     * Get every arguement description in the table.
     */
    for (i = 0; i < len; i++) {
        char item_description[MAX_ITEM_DESCRIPTION + 1] = {'\0'};
        char type_name[MAX_TYPE_DESCRIPTION + 1] = {'\0'};
        char argv_name[ARGS_MAX_NAME + 1] = {'\0'};
        char value_str[MAX_VALUE_DESCRIPTION + 1] = {'\0'};

        get_type_name(type_name, args_array[i].type);

        get_argv_name(argv_name, args_array[i].name);

        void *value_ptr = ebp + args_array[i].offset;

        /*
         * If arguement's data type is invalid, we need to check its
         * validation first.
         */
        if (!is_readable(*(void **)value_ptr)) {
            snprintf(value_str, MAX_VALUE_DESCRIPTION, "%p", value_ptr);
        } else {
            get_value_str(value_str, value_ptr, args_array[i].type);
        }

        snprintf(item_description, MAX_ITEM_DESCRIPTION,
                 "%s%s=%s", type_name, argv_name, value_str);
        strncat(argv_description, item_description,
                MAX_ARGV_DESCRIPTION - description_len);
        description_len += strlen(item_description);
        argv_description[description_len] = ',';
        description_len++;
        argv_description[description_len] = ' ';
        description_len++;
    }
    argv_description[description_len - 2] = ')';
    argv_description[description_len - 1] = '\0';

    return;
}

inline void get_type_name(char *type_name, int type_code) {
    switch (type_code) {
    case 0:
        snprintf(type_name, MAX_TYPE_DESCRIPTION, "char ");
        break;
    case 1:
        snprintf(type_name, MAX_TYPE_DESCRIPTION, "int ");
        break;
    case 2:
        snprintf(type_name, MAX_TYPE_DESCRIPTION, "float ");
        break;
    case 3:
        snprintf(type_name, MAX_TYPE_DESCRIPTION, "double ");
        break;
    case 4:
        snprintf(type_name, MAX_TYPE_DESCRIPTION, "char *");
        break;
    case 5:
        snprintf(type_name, MAX_TYPE_DESCRIPTION, "char **");
        break;
    case 6:
        snprintf(type_name, MAX_TYPE_DESCRIPTION, "void *");
        break;
    case -1:
        snprintf(type_name, MAX_TYPE_DESCRIPTION, "UNKNOWN ");
        break;
    default :
        snprintf(type_name, MAX_TYPE_DESCRIPTION, "UNKNOWN ");
        break;
    }

    return;
}

inline void get_argv_name(char *argv_name, char *name) {
    char is_print = 1;
    int i;
    for (i = 0; i < ARGS_MAX_NAME; i++) {
        if (name[i] == 0) {
            break;
        }
        if (!isprint(name[i])) {
            is_print = 0;
            break;
        }
    }
    if (is_print) {
        snprintf(argv_name, ARGS_MAX_NAME, "%s", name);
    } else {
        snprintf(argv_name, ARGS_MAX_NAME, "unprintable");
    }

    return;
}

void get_value_str(char *value_str, void *value_ptr, int type_code) {

    switch (type_code) {
    case TYPE_CHAR: // char
        if (isprint(*(char *)value_ptr)) {
            snprintf(value_str, MAX_VALUE_DESCRIPTION,
                     "\'%c\'", *(char *)value_ptr);
        } else {
            snprintf(value_str, MAX_VALUE_DESCRIPTION,
                     "\'\\%d\'", *(int *)value_ptr);
        }
        break;
    case TYPE_INT: // int
        snprintf(value_str, MAX_VALUE_DESCRIPTION,
                 "%d", *(int *)value_ptr);
        break;
    case TYPE_FLOAT: // float
        snprintf(value_str, MAX_VALUE_DESCRIPTION,
                 "%f", *(float *)value_ptr);
        break;
    case TYPE_DOUBLE: // double
        snprintf(value_str, MAX_VALUE_DESCRIPTION,
                 "%f", *(double *)value_ptr);
        break;
    case TYPE_STRING: { // string
        char *str_ptr = *(char **)value_ptr;
        int len;
        char is_print = 1;

        /*
         * Check whether the strig has a valid ending and whether it is
         * printable, and also determine the length of string.
         */
        for (len = 0; len < MAX_PRINT_STR_LEN + 1; len++) {
            if (!is_readable(str_ptr + len) || !isprint(str_ptr[len])) {
                is_print = 0;
                break;
            }
            if (str_ptr[len] == 0) {
                break;
            }
        }
        if (is_print) {
            if (len < MAX_PRINT_STR_LEN + 1) {
                snprintf(value_str, MAX_VALUE_DESCRIPTION, "\"%s\"", str_ptr);
            } else {
                snprintf(value_str, MAX_VALUE_DESCRIPTION,
                         "\"%25.25s...\"", str_ptr);
            }
        } else {
            snprintf(value_str, MAX_VALUE_DESCRIPTION, "%p", str_ptr);
        }

        break;
    }

    case TYPE_STRING_ARRAY: { // String array
        char ** str_array = *(char ***)value_ptr;
        int array_len = 0;
        int value_str_len = 1;

        /*
         * Check whether string array has a valid ending and determine its
         * length
         */
        for (array_len = 0;
                array_len < MAX_PRINT_STR_ARRAY_NUM + 1;
                array_len++) {
            if (!is_readable(str_array + array_len)) {
                snprintf(value_str, MAX_VALUE_DESCRIPTION, "%p", value_ptr);
                return;
            }
            if (str_array[array_len] == 0) {
                break;
            }
        }

        int scan_len = 0;
        if (array_len <= MAX_PRINT_STR_ARRAY_NUM) {
            scan_len = array_len;
        } else {
            scan_len = MAX_PRINT_STR_ARRAY_NUM;
        }

        value_str[0] = '{';
        int i;
        for (i = 0; i < scan_len; i++) {
            char str_item[MAX_PRINT_STR_LEN + strlen("\"...\"") + 2] = {'\0'};
            /*
             * For every element in the array.
             */
            char *str_ptr =  str_array[i];
            int len = 0;
            char is_print = 1;
            for (len = 0; len < MAX_PRINT_STR_LEN + 1; len++) {
                if (!is_readable(str_ptr + len) || !isprint(str_ptr[len])) {
                    is_print = 0;
                    break;
                }
                if (str_ptr[len] == 0) {
                    break;
                }
            }
            if (is_print) {
                if (len < MAX_PRINT_STR_LEN + 1) {
                    snprintf(str_item,
                             MAX_PRINT_STR_LEN + strlen("\"...\"") + 1,
                             "\"%s\"",
                             str_ptr);
                } else {
                    snprintf(str_item,
                             MAX_PRINT_STR_LEN + strlen("\"...\"") + 1,
                             "\"%25.25s...\"",
                             str_ptr);
                }
            } else {
                snprintf(str_item,
                         MAX_PRINT_STR_LEN + strlen("\"...\"") + 1,
                         "%p", str_ptr);
            }

            strncat(value_str, str_item, MAX_VALUE_DESCRIPTION - value_str_len);
            value_str_len += strlen(str_item);
            strncat(value_str, ", ", MAX_VALUE_DESCRIPTION - value_str_len);
            value_str_len += strlen(", ");
        }
        if (array_len < MAX_PRINT_STR_ARRAY_NUM + 1) {
            value_str[value_str_len - 2] = '\0';
            value_str_len -= strlen(", ");
        } else {
            strncat(value_str, "...", MAX_VALUE_DESCRIPTION - value_str_len);
            value_str_len += strlen("...");
        }

        strncat(value_str, "}", MAX_VALUE_DESCRIPTION - value_str_len);

        break;
    }
    break;
    case TYPE_VOIDSTAR:
        snprintf(value_str, MAX_VALUE_DESCRIPTION, "%p", value_ptr);
        value_str[1] = 'v';
        break;
    case TYPE_UNKNOWN:
        snprintf(value_str, MAX_VALUE_DESCRIPTION, "%p", value_ptr);
        break;
    default :
        break;
    }

    return;
}

inline int is_readable(void *addr) {
    int prot = PROT_READ;
    char ret = 0;

    /*
     * The check address of mprotect must be aligned to page szie.
     */
    int page_size = sysconf(_SC_PAGESIZE);
    void *page_begin_addr = (void *)(((int)addr / page_size) * page_size);

    ret = mprotect(page_begin_addr, page_size, prot);

    return ret == 0;
}