#pragma once

#include <stdint-gcc.h>

// Intrusive doubly-linked list node
// Embed this at the start of your struct to make it listable
struct list_node_t {
    list_node_t* next;
    list_node_t* prev;
};

using list_node = list_node_t;

// Initialize a list head
inline void list_init(list_node* head) {
    head->next = head;
    head->prev = head;
}

// Check if list is empty
inline bool list_empty(list_node* head) {
    return head->next == head;
}

// Insert new between prev and next
inline void list_insert(list_node* prev, list_node* new_node, list_node* next) {
    new_node->prev = prev;
    new_node->next = next;
    prev->next = new_node;
    next->prev = new_node;
}

// Add node to tail of list
inline void list_push_back(list_node* head, list_node* node) {
    list_insert(head->prev, node, head);
}

// Add node to head of list
inline void list_push_front(list_node* head, list_node* node) {
    list_insert(head, node, head->next);
}

// Remove node from list
inline void list_remove(list_node* node) {
    node->prev->next = node->next;
    node->next->prev = node->prev;
    node->next = nullptr;
    node->prev = nullptr;
}

// Get first node (not head)
inline list_node* list_front(list_node* head) {
    return head->next == head ? nullptr : head->next;
}

// Get last node (not head)
inline list_node* list_back(list_node* head) {
    return head->prev == head ? nullptr : head->prev;
}

// Iterate over list
#define list_for_each(head, var) \
    for ((var) = (head)->next; (var) != (head); (var) = (var)->next)

// Iterate safely (can remove during iteration)
#define list_for_each_safe(head, var, tmp) \
    for ((var) = (head)->next, (tmp) = (var)->next; \
         (var) != (head); \
         (var) = (tmp), (tmp) = (var)->next)
