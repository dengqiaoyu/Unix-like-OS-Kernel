#ifndef __LIST_H__
#define __LIST_H__

struct node_t {
    struct node_t *prev;
    struct node_t *next;
    unsigned int padding[2];
    char data[0];
} node_t;

struct list_t {
    int node_cnt;
    struct node_t *head;
    struct node_t *tail;
} list_t;

struct list_t *init_list();
void add_node_to_head(list_t *list, node_t *node);
void add_node_to_tail(list_t *list, node_t *node);
void delete_node(list_t *list, node_t *node);
node_t *get_first_node(list_t *list);
node_t *get_last_node(list_t *list);

#endif