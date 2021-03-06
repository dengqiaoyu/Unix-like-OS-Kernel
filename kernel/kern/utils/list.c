/**
 * @file list.c
 * @brief this file contains list helper function used to create a queue linked
 *        list.
 * @author Newton Xie(ncx), Qiaoyu Deng(qdeng)
 * @bug No known bugs
 */

#include <malloc.h>     /* calloc */
#include <string.h>     /* memset */
#include <stdio.h>      /* printf() */
#include <syscall.h>    /* gettid() */
#include <simics.h>
#include "scheduler.h"
#include "task.h"
#include "utils/list.h"       /* header file of this code */

/**
 * Initailize the list.
 * @return pointer to the list on success and NULL on failure
 */
list_t *list_init() {
    list_t *list = malloc(sizeof(list_t));
    if (list == NULL) return NULL;
    list->node_cnt = 0;

    node_t *head_node = calloc(1, sizeof(node_t));
    if (head_node == NULL) {
        free(list);
        return NULL;
    }
    list->head = head_node;

    node_t *tail_node = calloc(1, sizeof(node_t));
    if (tail_node == NULL) {
        free(head_node);
        free(list);
        return NULL;
    }
    list->tail = tail_node;

    head_node->next = tail_node;
    tail_node->prev = head_node;

    return list;
}

void list_destroy(list_t *list) {
    clear_list(list);
    free(list);
}

int get_list_size(list_t *list) {
    return list->node_cnt;
}

/**
 * Add new node after the dummy head node.
 * @param list the pointer to the list
 * @param node the pointer to the node
 */
void add_node_to_head(list_t *list, node_t *node) {
    list->node_cnt++;
    node->next = list->head->next;
    node->next->prev = node;
    node->prev = list->head;
    list->head->next = node;
}

/**
 * Add new node after the dummy tail node.
 * @param list the pointer to the list
 * @param node the pointer to the node
 */
void add_node_to_tail(list_t *list, node_t *node) {
    list->node_cnt++;
    node->prev = list->tail->prev;
    node->prev->next = node;
    node->next = list->tail;
    list->tail->prev = node;
}

/**
 * Removes a node from a list.
 * @param list the pointer to the list
 * @param node the pointer to the node
 */
void remove_node(list_t *list, node_t *node) {
    list->node_cnt--;
    node_t *prev_node = node->prev;
    node_t *next_node = node->next;
    prev_node->next = next_node;
    next_node->prev = prev_node;
}

/**
 * Delete node list.
 * @param list the pointer to the list
 * @param node the pointer to the node
 */
void delete_node(list_t *list, node_t *node) {
    list->node_cnt--;
    node_t *prev_node = node->prev;
    node_t *next_node = node->next;
    prev_node->next = next_node;
    next_node->prev = prev_node;
    free(node);
}

/**
 * Get the first node's in the list(not the head node)
 * @param  list the pointer to the list
 * @return      the pointer to the node for success, NULL for fail
 */
node_t *get_first_node(list_t *list) {
    if (list->node_cnt > 0) {
        return list->head->next;
    } else {
        return NULL;
    }
}

/**
 * Get the last node's in the list(not the tail node)
 * @param  list the pointer to the list
 * @return      the pointer to the node for success, NULL for fail
 */
node_t *get_last_node(list_t *list) {
    if (list->node_cnt > 0)
        return list->tail->prev;
    else
        return NULL;
}

/**
 * Gets the next node in a list.
 * @param   list        the pointer to the list
 *          node    a node present in the list
 * @return A pointer to the next node, or NULL if none exists.
 */
node_t *get_next_node(list_t *list, node_t *node) {
    if (node->next == list->tail) return NULL;
    else return node->next;
}

/**
 * Inserts a node before another node in a list.
 * @param   list        the pointer to the list
 *          cur_node    a node present in the list
 *          new_node    the node to be inserted
 */
void insert_before(list_t *list, node_t *cur_node, node_t *new_node) {
    list->node_cnt++;
    new_node->prev = cur_node->prev;
    cur_node->prev->next = new_node;
    new_node->next = cur_node;
    cur_node->prev = new_node;
}

/**
 * Get the first node in the list and unlinked it in the list.
 * @param  list the pointer to the list
 * @return      the pointer to the node for success, NULL for fail
 */
node_t *pop_first_node(list_t *list) {
    if (list->node_cnt == 0) return NULL;
    list->node_cnt--;
    node_t *first_node = list->head->next;
    list->head->next = first_node->next;
    first_node->next->prev = list->head;
    first_node->prev = NULL;
    first_node->next = NULL;

    return first_node;
}

/**
 * Clear all the nodes in the list.
 * @param list the pointer to the list
 */
void clear_list(list_t *list) {
    if (list->node_cnt == 0) return;
    node_t *node_rover = get_first_node(list);
    node_t *next = NULL;
    while (node_rover != NULL) {
        next = node_rover->next;
        free(node_rover);
        node_rover = next;
    }

    free(list->head);
    memset(list, 0, sizeof(list_t));
}
