// Copyright Damian Mihai-Robert 312CAb 2022-2023
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "vma.h"
#include "list.h"

list_t *dll_create(int64_t data_size)
{
	list_t *list = malloc(sizeof(list_t));
	DIE(!list, "List malloc failed\n");

	list->head = NULL;
	list->tail = NULL;
	list->data_size = data_size;
	list->size = 0;

	return list;
}

node_t *dll_get_node(list_t *list, int64_t n)
{
	node_t *p = list->head;

	if (list->size == 0)
		return NULL;

	if (n < 0)
		n = 0;

	if (n > list->size)
		n = list->size - 1;

	for (int i = 0; i < n; i++)
		p = p->next;

	return p;
}

void dll_add_node(list_t *list, int64_t n, void *new_data)
{
	node_t *current_node, *new_node;

	if (!list || !new_data)
		return;

	new_node = malloc(sizeof(node_t));
	DIE(!new_node, "Node malloc failed\n");
	new_node->data = malloc(list->data_size);
	DIE(!new_node->data, "Data malloc failed\n");
	memcpy(new_node->data, new_data, list->data_size);

	free(new_data);
	new_data = NULL;

	current_node = dll_get_node(list, n);

	// Insert the new node before the current node
	if (!current_node) {
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
		if (current_node->prev)
			current_node->prev->next = new_node;
		else
			list->head = new_node;
		current_node->prev = new_node;
	}

	list->size++;
}

void dll_remove_node(list_t *list, int64_t n, void (*f)(void *))
{
	node_t *node_to_remove;

	if (!list->head)
		return;

	if (n >= list->size)
		n = list->size - 1;

	node_to_remove = dll_get_node(list, n);

	if (n == 0) {
		// Sterge primul nod
		list->head = list->head->next;
		if (list->head)
			list->head->prev = NULL;
		if (node_to_remove == list->tail)
			list->tail = NULL;
	} else if (n == list->size - 1) {
		// Sterge ultimul nod
		list->tail = list->tail->prev;
		if (list->tail)
			list->tail->next = NULL;
		if (node_to_remove == list->head)
			list->head = NULL;
	} else {
		// Sterge un nod in mijlocul listei
		node_to_remove->prev->next = node_to_remove->next;
		node_to_remove->next->prev = node_to_remove->prev;
	}

	list->size--;
	if (list->size == 0)
		list->head = NULL;
	node_to_remove->prev = NULL;
	node_to_remove->next = NULL;

	if (f)
		f(node_to_remove);
}

int64_t dll_get_size(list_t *list) { return list->size; }

void dll_free(list_t *list)
{
	node_t *current_node, *next_node;

	if (!list)
		return;

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

void dll_print_int_list(list_t *list)
{
	if (!list || !list->head)
		return;

	node_t *current_node = list->head;
	while (current_node) {
		printf("%d ", *(int *)current_node->data);
		current_node = current_node->next;
	}
	printf("\n");
}

void dll_print_string_list(list_t *list)
{
	if (!list)
		return;

	node_t *current_node = list->head->prev;
	while (current_node) {
		printf("%s ", (char *)current_node->data);
		current_node = current_node->prev;
	}
	printf("\n");
}
