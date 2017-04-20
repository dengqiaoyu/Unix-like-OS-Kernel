/**
 * @file list.c
 * @brief this file contains list function declaretion and structure definition.
 * @author Newton Xie(ncx), Qiaoyu Deng(qdeng)
 * @bug No known bugs
 */

#ifndef __LIST_H__
#define __LIST_H__

typedef struct node {
    struct node *prev;
    struct node *next;
    char data[0];
} node_t;

typedef struct list {
    int node_cnt;
    node_t *head;
    node_t *tail;
} list_t;


list_t *list_init();

void list_destroy(list_t *list);

int get_list_size(list_t *list);

/**
 * Add new node after the dummy head node.
 * @param list the pointer to the list
 * @param node the pointer to the node
 */
void add_node_to_head(list_t *list, node_t *node);

/**
 * Add new node after the dummy tail node.
 * @param list the pointer to the list
 * @param node the pointer to the node
 */
void add_node_to_tail(list_t *list, node_t *node);

/**
 * Removes a node from a list.
 * @param list the pointer to the list
 * @param node the pointer to the node
 */
void remove_node(list_t *list, node_t *node);

/**
 * Delete node list.
 * @param list the pointer to the list
 * @param node the pointer to the node
 */
void delete_node(list_t *list, node_t *node);

/**
 * Get the first node's in the list(not the head node)
 * @param  list the pointer to the list
 * @return      the pointer to the node for success, NULL for fail
 */
node_t *get_first_node(list_t *list);

/**
 * Get the last node's in the list(not the tail node)
 * @param  list the pointer to the list
 * @return      the pointer to the node for success, NULL for fail
 */
node_t *get_last_node(list_t *list);

/**
 * Gets the next node in a list.
 * @param   list        the pointer to the list
 *          node    a node present in the list
 * @return A pointer to the next node, or NULL if none exists.
 */
node_t *get_next_node(list_t *list, node_t *node);

/**
 * Inserts a node before another node in a list.
 * @param   list        the pointer to the list
 *          cur_node    a node present in the list
 *          new_node    the node to be inserted
 */
void insert_before(list_t *list, node_t *cur_node, node_t *new_node);

/**
 * Get the first node in the list and unlinked it in the list.
 * @param  list the pointer to the list
 * @return      the pointer to the node for success, NULL for fail
 */
node_t *pop_first_node(list_t *list);

/**
 * Clear all the nodes in the list.
 * @param list the pointer to the list
 */
void clear_list(list_t *list);

#endif /* __LIST_H__ */
