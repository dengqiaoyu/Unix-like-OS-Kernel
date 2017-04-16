/** @file utils/maps.h
 *  @author Newton Xie (ncx)
 *  @bug No known bugs.
 */

#ifndef _MAPS_H_
#define _MAPS_H_

#include <stdint.h>

#define MAP_USER 0x1
#define MAP_WRITE 0x2
#define MAP_REMOVE 0x4

typedef struct map {
    uint32_t low;
    uint32_t high;
    int perms;
} map_t;

typedef struct map_list map_list_t;

map_list_t *maps_init();

void maps_destroy(map_list_t *maps);

void maps_clear(map_list_t *maps);

int maps_copy(map_list_t *from, map_list_t *to);

void undo_maps_copy(map_list_t *maps);

void maps_print(map_list_t *maps);

void maps_insert(map_list_t *maps, uint32_t low, uint32_t high, int perms);

map_t *maps_find(map_list_t *maps, uint32_t low, uint32_t high);

void maps_delete(map_list_t *maps, uint32_t low);

#endif
