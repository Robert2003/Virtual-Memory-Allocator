#pragma once

list_t*
dll_create(unsigned int data_size);

node_t*
dll_get_nth_node(list_t* list, int n);

void
dll_add_nth_node(list_t *list, unsigned int n, void *new_data);

node_t*
dll_remove_nth_node(list_t *list, unsigned int n, void (*f)(void *));

void
dll_free(list_t *list);

unsigned int
dll_get_size(list_t* list);

void
dll_print_int_list(list_t* list);

void
dll_print_string_list(list_t* list);