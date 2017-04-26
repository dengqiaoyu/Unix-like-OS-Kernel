/* --- Includes --- */
#include <string.h>
#include <stdio.h>
#include <malloc.h>
#include <exec2obj.h>
#include <utils/loader.h>
#include <elf_410.h>


/* --- Local function prototypes --- */


/**
 * Copies data from a file into a buffer.
 *
 * @param filename   the name of the file to copy data from
 * @param offset     the location in the file to begin copying from
 * @param size       the number of bytes to be copied
 * @param buf        the buffer to copy the data into
 *
 * @return returns the number of bytes copied on succes; -1 on failure
 */
int getbytes( const char *filename, int offset, int size, char *buf ) {
    if (offset < 0) return -1;

    int i, j;
    for (i = 0; i < MAX_NUM_APP_ENTRIES; i++) {
        const exec2obj_userapp_TOC_entry *entry = &exec2obj_userapp_TOC[i];
        if (!strcmp(filename, entry->execname)) {
            if (offset > entry->execlen) return -1;
            for (j = 0; j < size; j++) {
                if (offset + j >= entry->execlen) break;
                buf[j] = entry->execbytes[offset + j];
            }
            return j;
        }
    }
    return -1;
}

/*@}*/
