/** @file maps_internal.h
 *  @author Newton Xie (ncx)
 *  @bug No known bugs.
 */

#ifndef _MAPS_INTERNAL_H_
#define _MAPS_INTERNAL_H_

typedef struct map_node {
    struct map map;

    struct map_node *left;
    struct map_node *right;
    int height;
} map_node_t;

struct map_list {
    map_node_t *root;
};

int max(int a, int b);

int get_height(map_node_t *node);

void update_height(map_node_t *node);

map_node_t *smallest_node(map_node_t *tree);

int get_balance(map_node_t *tree);

void copy_map(map_node_t *from, map_node_t *to);

map_node_t *make_node(uint32_t addr, uint32_t size, int perms);

map_node_t *tree_insert(map_node_t *tree, map_node_t *node);

map_node_t *tree_find(map_node_t *tree, uint32_t addr, uint32_t size);

map_node_t *rotate_right(map_node_t *root);

map_node_t *rotate_left(map_node_t *root);

#endif
