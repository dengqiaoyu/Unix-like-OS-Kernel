/** @file maps.h
 *  @author Newton Xie (ncx)
 *  @bug No known bugs.
 */

#ifndef _MAPS_H_
#define _MAPS_H_

#include <stdint.h>

typedef struct map {
    uint32_t map_start;
    uint32_t map_end;
    int perms;
} map_t;

typedef struct map_list map_list_t;

map_list_t *init_maps();

void insert_map(map_list_t *maps, uint32_t addr, uint32_t size, int perms);

map_t *find_map(map_list_t *maps, uint32_t addr);

void delete_map(map_list_t *maps, uint32_t addr);

void destroy_maps(map_list_t *maps);

#endif
