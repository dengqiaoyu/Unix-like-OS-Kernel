/**
 * @file list.c
 * @brief XXX
 * @author Newton Xie(ncx), Qiaoyu Deng(qdeng)
 * @bug No known bugs
 */

#include <malloc.h>
#include <string.h>
#include <simics.h>
#include "list.h"

list_t *init_list() {
    list_t *list = calloc(1, sizeof(list_t));
    node_t *head_node = calloc(1, sizeof(node_t));
    list->head = head_node;
    node_t *tail_node = calloc(1, sizeof(node_t));
    list->tail = tail_node;

    head_node->next = tail_node;
    tail_node->prev = head_node;
    return list;
}

void add_node_to_head(list_t *list, node_t *node) {
    list->node_cnt++;
    node->next = list->head->next;
    node->next->prev = node;
    node->prev = list->head;
    list->head->next = node;
}

void add_node_to_tail(list_t *list, node_t *node) {
    list->node_cnt++;
    node->next = list->tail;
    list->tail->prev = node;
    node->prev = list->tail->prev;
    node->prev->next = node;
}

void delete_node(list_t *list, node_t *node) {
    list->node_cnt--;
    node_t *prev_node = node->prev;
    node_t *next_node = node->next;
    prev_node->next = next_node;
    next_node->prev = prev_node;
    free(node);
}

node_t *get_first_node(list_t *list) {
    if (list->node_cnt > 0)
        return list->head->next;
    else
        return NULL;
}

node_t *get_last_node(list_t *list) {
    if (list->node_cnt > 0)
        return list->tail->prev;
    else
        return NULL;
}

node_t *pop_first_node(list_t *list) {
    if (list->node_cnt == 0) return NULL;
    node_t *first_node = list->head->next;
    list->head->next = first_node->next;
    first_node->next->prev = list->head;
    first_node->prev = NULL;
    first_node->next = NULL;

    return first_node;
}
