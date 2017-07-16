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

/* 
 * Abstract typedef for a map list structure, which will serve to keep track
 * of a task's memory regions.
 */
typedef struct map_list map_list_t;

/** @brief  Initializes a map list.
 *  @return A pointer to a map_list_t structure, or NULL on failure.
 */
map_list_t *maps_init();

/** @brief  Destroys a map list, freeing all stored maps.
 *  @param  maps    A pointer to a map_list_t structure.
 */
void maps_destroy(map_list_t *maps);

/** @brief  Clears a map list, leaving it in its initialized state.
 *  @param  maps    A pointer to a map_list_t structure.
 */  
void maps_clear(map_list_t *maps);

/** @brief  Inserts a mapped region into a map list.
 *  @param  maps    A pointer to a map_list_t structure.
 *          low     The start of the mapped region.
 *          high    The end of the mapped region (inclusive).
 *          perms   An OR of permission flags, as defined above.
 *  @return 0 on success and negative on failure.
 */
int maps_insert(map_list_t *maps, uint32_t low, uint32_t high, int perms);

/** @brief  Finds a map in a map list which intersects with a given region.
 *  @param  maps    A pointer to a map_list_t structure.
 *          low     The start of the search region.
 *          high    The end of the search region (inclusive).
 *  @return A pointer to an overalpping map_t, or NULL if none exist.
 */
map_t *maps_find(map_list_t *maps, uint32_t low, uint32_t high);

/** @brief  Deletes a mapped region from a map list.
 *  @param  maps    A pointer to a map_list_t structure.
 *          low     Must be equal to the low value of a map in the list.
 */
void maps_delete(map_list_t *maps, uint32_t low);

/** @brief  Makes a copy of a map list.
 *  @param  from    A pointer to a map_list_t structure.
 *          to      A pointer to a map list which must be empty (initialized).
 *  @return 0 on success and negative on failure.
 */
int maps_copy(map_list_t *from, map_list_t *to);

/** @brief  Prints a map list to Simics (debugging tool only).
 *  @param  maps    A pointer to a map_list_t structure.
 */
void maps_print(map_list_t *maps);

#endif
