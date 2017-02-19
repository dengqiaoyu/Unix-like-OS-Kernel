/**
 * @file list.c
 * @brief XXX
 * @author Newton Xie(ncx), Qiaoyu Deng(qdeng)
 * @bug No known bugs
 */

#include <malloc.h>
#include "list.h"

struct list_t *init_list() {
    struct list_t *list = calloc(1, sizeof(struct list_t));
    struct node_t *head_node = calloc(1, sizeof(struct node_t));
    list->head = head_node;
    struct node_t *tail_node = calloc(1, sizeof(struct node_t));
    list->tail = tail_node;

    head_node->next = tail_node;
    tail_node->prev = head_node;
    return list;
}

void add_node_to_head(struct list_t *list, struct node_t *node) {
    list->node_cnt++;
    node->next = list->head->next;
    node->prev = list->head;
    list->head->next = node;
}

void add_node_to_tail(struct list_t *list, struct node_t *node) {
    list->node_cnt++;
    node->next = NULL;
    node->prev = list->tail->prev;
    list->tail->prev->next = node;
}

void delete_node(struct list_t *list, struct node_t *node) {
    list->node_cnt--;
    struct node *prev_node = node->prev;
    struct node *next_node = node->next;
    prev_node->next = next_node;
    next_node->prev = prev_node;
    free(node);
}

struct node_t *get_first_node(struct list_t *list) {
    if (list->node_cnt > 0)
        return list->head->next;
    else
        return NULL;
}

struct node_t *get_last_node(struct list_t *list) {
    if (list->node_cnt > 0)
        return list->tail->prev;
    else
        return NULL;
}