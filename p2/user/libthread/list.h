#ifndef __LIST_H__
#define __LIST_H__

typedef struct node node_t;
struct node {
    node_t *prev;
    node_t *next;
    char data[0];
};

typedef struct list list_t;
struct list {
    int node_cnt;
    node_t *head;
    node_t *tail;
}; 

list_t *init_list();
void add_node_to_head(list_t *list, node_t *node);
void add_node_to_tail(list_t *list, node_t *node);
void delete_node(list_t *list, node_t *node);
node_t *get_first_node(list_t *list);
node_t *get_last_node(list_t *list);

#endif
