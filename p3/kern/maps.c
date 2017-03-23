/** @file
 *
 */

#include <stdlib.h>
#include <assert.h>
#include "maps.h"
#include "maps_internal.h"

#define MAP_START(node) node->data.map_start
#define MAP_END(node) node->data.map_end

map_list_t *init_maps()
{
    map_list_t *list = malloc(sizeof(map_list_t));
    list->root = NULL;
    return list;
}

void insert_map(map_list_t *list, uint32_t addr, uint32_t size, int perms)
{
    map_node_t *node = make_node(addr, size, perms);
    list->root = tree_insert(list->root, node);
}

map_t *find_map(map_list_t *list, uint32_t addr)
{
    map_node_t *node = tree_find(list->root, addr);
    return &(node->data);
}

void delete_map(map_list_t *list, uint32_t addr)
{
    return;
}

void destroy_maps(map_list_t *list)
{
    return;
}

int max(int a, int b)
{
    return (a > b) ? a : b;
}

int get_height(map_node_t *node)
{
    if (node == NULL) return 0;
    return node->height;
}

// assumes children all have proper heights
void update_height(map_node_t *node)
{
    int tallest_child = max(get_height(node->left), get_height(node->right));
    node->height = tallest_child + 1;
}

map_node_t *make_node(uint32_t addr, uint32_t size, int perms)
{
    map_node_t *node = malloc(sizeof(map_node_t));
    node->data.map_start = addr;
    node->data.map_end = addr + size;
    node->data.perms = perms;

    node->left = node->right = NULL;
    node->height = 1;
    return node;
}

map_node_t *tree_insert(map_node_t *tree, map_node_t *node)
{
    if (tree == NULL) return node;
    
    if (MAP_START(tree) < MAP_START(node)) {
        tree->right = tree_insert(tree->right, node);
    }
    else {
        assert(MAP_START(tree) > MAP_START(node));
        tree->left = tree_insert(tree->left, node);
    }

    update_height(tree);
    int balance = get_height(tree->right) - get_height(tree->left);

    if (balance > 1) {
        if (MAP_START(node) < MAP_START(tree->right)) {
            tree->right = rotate_right(tree->right);
        }
        return rotate_left(tree);
    }

    if (balance < -1) {
        if (MAP_START(node) > MAP_START(tree->left)) {
            tree->left = rotate_left(tree->left);
        }
        return rotate_right(tree);
    }

    return tree;
}

map_node_t *tree_find(map_node_t *tree, uint32_t addr)
{
    if (tree == NULL) return NULL;

    if (MAP_START(tree) > addr) {
        return tree_find(tree->left, addr);
    }
    else {
        if (MAP_END(tree) > addr) return tree;
        return tree_find(tree->right, addr);
    }
}

map_node_t *rotate_right(map_node_t *old_root)
{
    map_node_t *new_root = old_root->left;
    map_node_t *new_left = new_root->right;

    new_root->right = old_root;
    old_root->left = new_left;
    update_height(new_root);
    update_height(old_root);
    return new_root;
}

map_node_t *rotate_left(map_node_t *old_root)
{
    map_node_t *new_root = old_root->right;
    map_node_t *new_right = new_root->left;

    new_root->left = old_root;
    old_root->right = new_right;
    update_height(new_root);
    update_height(old_root);
    return new_root;
}

