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
#include "list.h"       /* header file of this code */

/**
 * Initailize the list.
 * @param  list the pointer to the list
 * @return      SUCCESS(0) for success, ERROR_INIT_LIST_CALLOC_FAILED(-1) for
 *              fail
 */
int init_list(list_t *list) {
    node_t *head_node = calloc(1, sizeof(node_t));
    if (head_node == NULL) {
        int tid = gettid();
        printf("Cannot allocate more memory for thread %d\n", tid);
        return ERROR_INIT_LIST_CALLOC_FAILED;
    }
    /* this is the dummy head for easy iterate */
    list->head = head_node;

    node_t *tail_node = calloc(1, sizeof(node_t));
    if (tail_node == NULL) {
        free(head_node);
        list->head = NULL;
        int tid = gettid();
        printf("Cannot allocate more memory for thread %d\n", tid);
        return ERROR_INIT_LIST_CALLOC_FAILED;
    }
    /* this is the dummy head for easy iterate */
    list->tail = tail_node;

    head_node->next = tail_node;
    tail_node->prev = head_node;

    return SUCCESS;
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
    if (list->node_cnt > 0)
        return list->head->next;
    else
        return NULL;
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
    free(list->tail);
    memset(list, 0, sizeof(list_t));
}
