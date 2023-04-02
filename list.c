#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "vma.h"
#include "list.h"

list_t*
dll_create(unsigned int data_size)
{
	list_t *list = malloc(sizeof(list_t));

    list->head = NULL;
    list->tail = NULL;
    list->data_size = data_size;
    list->size = 0;

    return list;
}

node_t*
dll_get_nth_node(list_t *list, unsigned int n)
{
    node_t *p = list->head;

    if (list->size == 0)
        return NULL;

    if (n > list->size)
        n = list->size;

    for (int i = 0; i < n; i++)
        p = p->next;
    
    return p;
}

void
dll_add_nth_node(list_t *list, unsigned int n, const void *new_data)
{
    node_t *current_node, *new_node;

    if (!list || !new_data) {
        return;
    }

	if (n > list->size) {
        n = list->size;
    }
    
    new_node = malloc(sizeof(node_t));
    new_node->data = malloc(list->data_size);
    memcpy(new_node->data, new_data, list->data_size);

    current_node = dll_get_nth_node(list, n);

    // Insert the new node before the current node
    if (current_node == NULL) {
        // The new node will be the new tail of the list
        new_node->prev = list->tail;
        new_node->next = NULL;
        if (list->tail) {
            list->tail->next = new_node;
            list->tail = new_node;
        } else {
            list->head = new_node;
            list->tail = new_node;
        }
    } else {
        // The new node will be inserted between two existing nodes
        new_node->prev = current_node->prev;
        new_node->next = current_node;
        if (current_node->prev) {
            current_node->prev->next = new_node;
        } else {
            list->head = new_node;
        }
        current_node->prev = new_node;
    }

    list->size++;
}

node_t*
dll_remove_nth_node(list_t *list, unsigned int n)
{
    node_t* node_to_remove;

	if (!list->head) {
        return NULL;
    }
    if (n > list->size) {
        n = list->size;
    }

    node_to_remove = dll_get_nth_node(list, n);

    if (n == 0) {
        // Remove the first node
        list->head = list->head->next;
        if (list->head != NULL) {
            list->head->prev = NULL;
        }
        if (node_to_remove == list->tail) {
            list->tail = NULL;
        }
    } else if (n == list->size - 1) {
        // Remove the last node
        list->tail = list->tail->prev;
        if (list->tail != NULL) {
            list->tail->next = NULL;
        }
        if (node_to_remove == list->head) {
            list->head = NULL;
        }
    } else {
        // Remove a node in the middle of the list
        node_to_remove->prev->next = node_to_remove->next;
        node_to_remove->next->prev = node_to_remove->prev;
    }

    list->size--;
    if(list->size == 0)
        list->head = NULL;
    node_to_remove->prev = NULL;
    node_to_remove->next = NULL;

    return node_to_remove;
}

unsigned int
dll_get_size(list_t *list)
{
	return list->size;
}

void
dll_free(list_t *list)
{
    node_t *current_node, *next_node;

	if (list == NULL) {
        return;
    }

    current_node = list->head;
    while (current_node) {
        next_node = current_node->next;
        free(current_node->data);
        free(current_node);
        current_node = next_node;
    }

    free(list);
    list = NULL;
}

void
dll_print_int_list(list_t* list)
{
	if(!list || !list->head)
        return;

    node_t* current_node = list->head;
    while (current_node != NULL) {
        printf("%d ", *(int *)current_node->data);
        current_node = current_node->next;
    }
    printf("\n");
}

void
dll_print_string_list(list_t* list)
{
	 if(!list)
        return;

    node_t* current_node = list->head->prev;
    while (current_node != NULL) {
        printf("%s ", (char *)current_node->data);
        current_node = current_node->prev;
    }
    printf("\n");
}