/** @file utils/maps_internal.h
 *  @brief Contains declarations of internal maps.c structures and functions.
 *  @author Newton Xie (ncx)
 *  @author Qiaoyu Deng (qdeng)
 *  @bug No known bugs.
 */

#ifndef _MAPS_INTERNAL_H_
#define _MAPS_INTERNAL_H_

#include "utils/kern_mutex.h"

/** @brief Wraps a map_t struct with a binary tree node.
 *
 *  The map_t struct is defined in maps.h.
 *
 *  The map_node_t struct is used when creating a new tree node in order
 *  to avoid multiple calls to malloc(). It is not exposed to users of the
 *  maps interface.
 */
typedef struct map_node {
    struct map map;
    int height;

    struct map_node *left;
    struct map_node *right;
} map_node_t;

/** @brief Represents a list of memory maps.
 *
 *  This struct is exposed to users as an abstract map_list_t type. A typedef
 *  is given in maps.h. It contains a pointer to the map_node_t at the root
 *  of the maps tree.
 */
struct map_list {
    map_node_t *root;
    kern_mutex_t mutex;
};

/** @brief Returns the max of two integers.
 */
int max(int a, int b);

/** @brief Returns the height of a tree node.
 */
int get_height(map_node_t *node);

/** @brief  Updates the height of a tree node.
 *
 *  Assumes that the heights of its children are correct.
 */
void update_height(map_node_t *node);

/** @brief  Finds the smallest node in a tree.
 *  @param  tree     A pointer to the tree root.
 *  @return A pointer to the smallest node in the tree (NULL if tree is NULL).
 */
map_node_t *smallest_node(map_node_t *tree);

/** @brief  Determines which subtree has greater height.
 *  @param  tree    A pointer to the tree root.
 *  @return The difference between the heights of the right subtree and the
 *          left subtree (may be negative). 0 if tree is NULL.
 */
int get_balance(map_node_t *tree);

/** @brief  Copies the map_t entries from one map node to another.
 *  @param  from    A pointer to a map node containing the map_t to copy from.
 *          to      A pointer to the destination map node.
 */
void copy_map(map_node_t *from, map_node_t *to);

/** @brief  Performs an AVL right rotation on a maps tree.
 *  @param  old_root  A pointer to the tree root.
 *  @return A pointer to the root of the rotated tree.
 */
map_node_t *rotate_right(map_node_t *old_root);

/** @brief  Performs an AVL left rotation on a maps tree.
 *  @param  old_root  A pointer to the tree root.
 *  @return A pointer to the root of the rotated tree.
 */
map_node_t *rotate_left(map_node_t *old_root);

/** @brief  Makes a map node.
 *  @param  low     Map start.
 *          high    Map end (inclusive).
 *          perms   Map perms (an OR of flags defined in maps.h).
 *  @return A pointer to a height 1 map node, or NULL on failure.
 */
map_node_t *tree_node(uint32_t low, uint32_t high, int perms);

/** @brief  Deletes a maps tree.
 *
 *  Recursively calls tree_destroy on left and right subtrees, then frees
 *  the map node memory.
 *
 *  @param  tree    A pointer to the tree root.
 */
void tree_destroy(map_node_t *tree);

/** @brief  Inserts a map node into a maps tree.
 *
 *  Assumes that the low and high values of the node's map_t do not overlap
 *  with those of any entries in the tree.
 *
 *  If the tree is empty (tree == NULL), returns the node.
 *
 *  Otherwise, recursively calls tree_insert to insert the node into the
 *  appropriate subtree. After insertion, the subtrees will be well-formed
 *  and balanced, with correct heights. Subtrees heights are compared, and
 *  if it is determined that the current tree is imbalanced, then necessary
 *  rotations are applied, producing a new tree satisfying AVL invariants.
 *
 *  @param  tree    A pointer to the tree root.
 *          node    A pointer to the map node to be inserted.
 *  @return A pointer to the root of the new tree.
 */
map_node_t *tree_insert(map_node_t *tree, map_node_t *node);

/** @brief  Finds a map node which overlaps with a given region.
 *
 *  If the tree is empty (tree == NULL), returns NULL.
 *
 *  If the tree's root map node intersects with the search region, then a
 *  pointer to the root is returned. Otherwise, recursively calls tree_find
 *  on the appropriate subtree.
 *
 *  @param  tree    A pointer to the tree root.
 *          low     Search region start value.
 *          high    Search region end value (inclusive).
 *  @return A pointer to an overlapping map node, or NULL if none exist.
 */
map_node_t *tree_find(map_node_t *tree, uint32_t low, uint32_t high);

/** @brief  Deletes a map node from a tree.
 *
 *  The low parameter must be the start value of a map present in the tree.
 *
 *  CASE 1: The tree is a singleton. Then the root is freed, returning NULL.
 *  CASE 2: The tree only has one subtree. In this case the root is freed, and
 *          a pointer to the non-empty subtree is returned.
 *  CASE 3: The tree has two subtrees. Then the map_t attibutes of the smallest
 *          node in the right subtree are copied to the root of the tree, and
 *          that node is removed by a recursive call to tree_delete.
 *
 *          Resulting subtree heights are compared. If it is determined that
 *          the current tree is imbalanced, then necessary rotations are
 *          applied, producing a new tree satisfying AVL invariants.
 *
 *  @param  tree    A pointer to the tree root.
 *          low     The start value in the map_t of the map node to be deleted.
 *  @return A pointer to the root of the new tree.
 */
map_node_t *tree_delete(map_node_t *tree, uint32_t low);

/** @brief  Makes a copy of a maps tree.
 *
 *  If the tree is empty, then NULL is returned. Otherwise, a new node is
 *  created with the same map_t attributes as the tree's root. The left and
 *  right subtree copies are then filled in by recursive calls.
 *
 *  @param  tree    A pointer to the root of the tree to be copied.
 *  @return A pointer to the root of the copy, or NULL on failure.
 */
map_node_t *tree_copy(map_node_t *tree);

/** @brief  Prints the contents of a maps tree to Simics.
 *
 *  To be used as a debugging tool only. Makes a recursive call to the left
 *  subtree, prints the map node contents of the root, and then makes another
 *  recursive call on the right subtree.
 *
 *  @param  tree    A pointer to the tree root.
 */
void tree_print(map_node_t *tree);

#endif
