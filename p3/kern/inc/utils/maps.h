/** @file utils/maps.h
 *  @brief Contains declarations of maps tree functions and structures.
 *  @author Newton Xie (ncx)
 *  @author Qiaoyu Deng (qdeng)
 *  @bug No known bugs.
 */

#ifndef _MAPS_H_
#define _MAPS_H_

#include <stdint.h>

#define MAP_USER 0x1
#define MAP_WRITE 0x2
#define MAP_EXECUTE 0x4
#define MAP_REMOVE 0x8

/** @brief Represents a memory map.
 *
 *  Attributes include the low and high (inclusive) addresses of a memory
 *  region. Also stores the associated permissions in the form of an OR of
 *  flags defined at the top of this header file.
 */
typedef struct map {
    uint32_t low;
    uint32_t high;
    int perms;
} map_t;

typedef struct map_list map_list_t;

map_list_t *maps_init();

void maps_destroy(map_list_t *maps);

void maps_clear(map_list_t *maps);

int maps_insert(map_list_t *maps, uint32_t low, uint32_t high, int perms);

map_t *maps_find(map_list_t *maps, uint32_t low, uint32_t high);

void maps_delete(map_list_t *maps, uint32_t low);

int maps_copy(map_list_t *from, map_list_t *to);

void maps_print(map_list_t *maps);

#endif
