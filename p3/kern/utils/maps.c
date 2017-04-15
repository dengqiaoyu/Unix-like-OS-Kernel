/** @file
 *
 */

#include <stdlib.h>
#include <simics.h>
#include <assert.h>
#include "utils/maps.h"
#include "utils/maps_internal.h"

#define MAP_LOW(node) (node->map.low)
#define MAP_HIGH(node) (node->map.high)
#define MAP_PERMS(node) (node->map.perms)

map_list_t *maps_init() {
    map_list_t *maps = malloc(sizeof(map_list_t));
    if (maps != NULL) maps->root = NULL;
    return maps;
}

void maps_destroy(map_list_t *maps) {
    tree_destroy(maps->root);
    free(maps);
}

void maps_clear(map_list_t *maps) {
    tree_destroy(maps->root);
    maps->root = NULL;
}

int maps_copy(map_list_t *from, map_list_t *to) {
    map_node_t *copy = tree_copy(from->root);
    if (copy == NULL && from->root != NULL) return -1;
    to->root = copy;
    return 0;
}

void undo_maps_copy(map_list_t *maps) {
    maps_clear(maps);
}

void maps_print(map_list_t *maps) {
    tree_print(maps->root);
}

// assumes no overlaps
void maps_insert(map_list_t *maps, uint32_t low, uint32_t high, int perms) {
    map_node_t *node = make_node(low, high, perms);
    maps->root = tree_insert(maps->root, node);
}

map_t *maps_find(map_list_t *maps, uint32_t low, uint32_t high) {
    map_node_t *node = tree_find(maps->root, low, high);
    return &(node->map);
}

void maps_delete(map_list_t *maps, uint32_t low) {
    tree_delete(maps->root, low);
}

int max(int a, int b) {
    return (a > b) ? a : b;
}

int get_height(map_node_t *node) {
    if (node == NULL) return 0;
    return node->height;
}

// assumes children all have proper heights
void update_height(map_node_t *node) {
    int tallest_child = max(get_height(node->left), get_height(node->right));
    node->height = tallest_child + 1;
}

map_node_t *make_node(uint32_t low, uint32_t high, int perms) {
    map_node_t *node = malloc(sizeof(map_node_t));
    MAP_LOW(node) = low;
    MAP_HIGH(node) = high;
    MAP_PERMS(node) = perms;

    node->left = node->right = NULL;
    node->height = 1;
    return node;
}

map_node_t *smallest_node(map_node_t *tree) {
    if (tree == NULL) return NULL;
    map_node_t *node = tree;
    while (node->left != NULL) node = node->left;
    return node;
}

int get_balance(map_node_t *tree) {
    if (tree == NULL) return 0;
    return get_height(tree->right) - get_height(tree->left);
}

void copy_node(map_node_t *from, map_node_t *to) {
    MAP_LOW(to) = MAP_LOW(from);
    MAP_HIGH(to) = MAP_HIGH(from);
    MAP_PERMS(to) = MAP_PERMS(from);
}

map_node_t *tree_find(map_node_t *tree, uint32_t low, uint32_t high) {
    if (tree == NULL) return NULL;

    if (high < MAP_LOW(tree)) {
        return tree_find(tree->left, low, high);
    } else if (MAP_HIGH(tree) < low) {
        return tree_find(tree->right, low, high);
    }

    return tree;
}

// assumes no overlaps
map_node_t *tree_insert(map_node_t *tree, map_node_t *node) {
    if (tree == NULL) return node;

    if (MAP_LOW(node) < MAP_LOW(tree)) {
        assert(MAP_HIGH(node) < MAP_LOW(tree));
        tree->left = tree_insert(tree->left, node);
    } else {
        assert(MAP_HIGH(tree) < MAP_LOW(node));
        tree->right = tree_insert(tree->right, node);
    }

    update_height(tree);
    int balance = get_balance(tree);

    if (balance > 1) {
        // if (MAP_LOW(node) < MAP_LOW(tree->right)) {
        if (get_balance(tree->right) < 0) {
            tree->right = rotate_right(tree->right);
        }
        return rotate_left(tree);
    }

    if (balance < -1) {
        // if (MAP_LOW(tree->left) < MAP_LOW(node)) {
        if (get_balance(tree->left) > 0) {
            tree->left = rotate_left(tree->left);
        }
        return rotate_right(tree);
    }

    return tree;
}

// assumes low is low
map_node_t *tree_delete(map_node_t *tree, uint32_t low) {
    if (MAP_LOW(tree) < low) {
        tree->right = tree_delete(tree->right, low);
    } else if (low < MAP_LOW(tree)) {
        tree->left = tree_delete(tree->left, low);
    } else {
        map_node_t *temp;
        if (tree->left == NULL && tree->right == NULL) {
            free(tree);
            return NULL;
        } else if (tree->left == NULL) {
            temp = tree->right;
            free(tree);
            return temp;
        } else if (tree->right == NULL) {
            temp = tree->left;
            free(tree);
            return temp;
        } else {
            copy_node(smallest_node(tree->right), tree);
            tree->right = tree_delete(tree->right, MAP_LOW(tree));
        }
    }

    update_height(tree);
    int balance = get_balance(tree);

    if (balance > 1) {
        if (get_balance(tree->right) < 0) {
            tree->right = rotate_right(tree->right);
        }
        return rotate_left(tree);
    }

    if (balance < -1) {
        if (get_balance(tree->left) > 0) {
            tree->left = rotate_left(tree->left);
        }
        return rotate_right(tree);
    }

    return tree;
}

void tree_destroy(map_node_t *tree) {
    if (tree == NULL) return;
    tree_destroy(tree->left);
    tree_destroy(tree->right);
    free(tree);
}

map_node_t *tree_copy(map_node_t *tree) {
    if (tree == NULL) return NULL;

    map_node_t *copy = malloc(sizeof(map_node_t));
    if (copy == NULL) return NULL;
    copy_node(tree, copy);
    copy->height = tree->height;

    copy->left = tree_copy(tree->left);
    if (copy->left == NULL && tree->left != NULL) {
        free(copy);
        return NULL;
    }

    copy->right = tree_copy(tree->right);
    if (copy->right == NULL && tree->right != NULL) {
        tree_destroy(copy->left);
        free(copy);
        return NULL;
    }

    return copy;
}

void tree_print(map_node_t *node) {
    if (node == NULL) return;
    tree_print(node->left);
    lprintf("low: %x high: %x perms: %x height: %d",
            (unsigned int)MAP_LOW(node),
            (unsigned int)MAP_HIGH(node),
            (unsigned int)MAP_PERMS(node),
            node->height);
    tree_print(node->right);
}

map_node_t *rotate_right(map_node_t *old_root) {
    map_node_t *new_root = old_root->left;
    map_node_t *new_left = new_root->right;

    new_root->right = old_root;
    old_root->left = new_left;
    update_height(old_root);
    update_height(new_root);
    return new_root;
}

map_node_t *rotate_left(map_node_t *old_root) {
    map_node_t *new_root = old_root->right;
    map_node_t *new_right = new_root->left;

    new_root->left = old_root;
    old_root->right = new_right;
    update_height(old_root);
    update_height(new_root);
    return new_root;
}

