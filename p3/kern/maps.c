/** @file
 *
 */

#include <stdlib.h>
#include <simics.h>
#include <assert.h>
#include "maps.h"
#include "maps_internal.h"

#define MAP_START(node) (node->map.start)
#define MAP_SIZE(node) (node->map.size)
#define MAP_PERMS(node) (node->map.perms)

map_list_t *maps_init() {
    map_list_t *list = malloc(sizeof(map_list_t));
    list->root = NULL;
    return list;
}

void maps_destroy(map_list_t *list) {
    tree_destroy(list->root);
    free(list);
}

map_list_t *maps_copy(map_list_t *list) {
    map_list_t *copy = malloc(sizeof(map_list_t));
    copy->root = tree_copy(list->root);
    return copy;
}

//TODO end me
void maps_print(map_list_t *list) {
    tree_print(list->root);
}

// assumes no overlaps
void maps_insert(map_list_t *list, uint32_t addr, uint32_t size, int perms) {
    map_node_t *node = make_node(addr, size, perms);
    list->root = tree_insert(list->root, node);
}

map_t *maps_find(map_list_t *list, uint32_t addr, uint32_t size) {
    map_node_t *node = tree_find(list->root, addr, size);
    return &(node->map);
}

void maps_delete(map_list_t *list, uint32_t addr) {
    tree_delete(list->root, addr);
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

map_node_t *make_node(uint32_t addr, uint32_t size, int perms) {
    map_node_t *node = malloc(sizeof(map_node_t));
    MAP_START(node) = addr;
    MAP_SIZE(node) = size;
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
    MAP_START(to) = MAP_START(from);
    MAP_SIZE(to) = MAP_SIZE(from);
    MAP_PERMS(to) = MAP_PERMS(from);
}

map_node_t *tree_find(map_node_t *tree, uint32_t addr, uint32_t size) {
    if (tree == NULL) return NULL;

    if (addr < MAP_START(tree)) {
        if (MAP_START(tree) - addr >= size) {
            return tree_find(tree->left, addr, size);
        }
    }
    else if (addr - MAP_START(tree) >= MAP_SIZE(tree)) {
        return tree_find(tree->right, addr, size);
    }

    return tree;
}

// assumes no overlaps
map_node_t *tree_insert(map_node_t *tree, map_node_t *node) {
    if (tree == NULL) return node;
    
    if (MAP_START(node) < MAP_START(tree)) {
        assert(MAP_START(tree) - MAP_START(node) >= MAP_SIZE(node));
        tree->left = tree_insert(tree->left, node);
    }
    else {
        assert(MAP_START(node) - MAP_START(tree) >= MAP_SIZE(tree));
        tree->right = tree_insert(tree->right, node);
    }

    update_height(tree);
    int balance = get_balance(tree);

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

map_node_t *tree_delete(map_node_t *tree, uint32_t addr) {
    if (MAP_START(tree) < addr) {
        tree->right = tree_delete(tree->right, addr);
    }
    else if (addr < MAP_START(tree)) {
        tree->left = tree_delete(tree->left, addr);
    }
    else {
        if (tree->left == NULL || tree->right == NULL) {
            map_node_t *temp;
            if (tree->left == NULL) temp = tree->right;
            else temp = tree->left;

            if (temp == NULL) {
                temp = tree;
                tree = NULL;
            }
            else {
                copy_node(temp, tree);
                tree->left = NULL;
                tree->right = NULL;
            }
            free(temp);
        }
        else {
            copy_node(smallest_node(tree->right), tree);
            tree->right = tree_delete(tree->right, MAP_START(tree));
        }
    }
    if (tree == NULL) return tree;

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
    map_node_t *left = tree->left;
    map_node_t *right = tree->right;
    free(tree);
    tree_destroy(left);
    tree_destroy(right);
}

map_node_t *tree_copy(map_node_t *tree) {
    if (tree == NULL) return NULL;

    map_node_t *copy = malloc(sizeof(map_node_t));
    MAP_START(copy) = MAP_START(tree);
    MAP_SIZE(copy) = MAP_SIZE(tree);
    MAP_PERMS(copy) = MAP_PERMS(tree);
    copy->height = tree->height;

    copy->left = tree_copy(tree->left);
    copy->right = tree_copy(tree->right);

    return copy;
}

void tree_print(map_node_t *node) {
    if (node == NULL) return;
    tree_print(node->left);
    lprintf("start: %x size: %x perms: %x height: %d", (unsigned int)MAP_START(node), 
            (unsigned int)MAP_SIZE(node), (unsigned int)MAP_PERMS(node),
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

