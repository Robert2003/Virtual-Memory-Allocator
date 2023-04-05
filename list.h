// Copyright Damian Mihai-Robert 312CAb 2022-2023
#pragma once
#include <inttypes.h>
#include <stddef.h>
#include <assert.h>
#include <errno.h>

#define DIE(assertion, call_description)				\
	do {								\
		if (assertion) {					\
			fprintf(stderr, "(%s, %d): ",			\
					__FILE__, __LINE__);		\
			perror(call_description);			\
			exit(errno);				        \
		}							\
	} while (0)

#include "list.h"
#include "vma.h"

list_t *dll_create(int64_t data_size);

node_t *dll_get_node(list_t *list, int64_t n);

void dll_add_node(list_t *list, int64_t n, void *new_data);

void dll_remove_node(list_t *list, int64_t n, void (*f)(void *));

void dll_free(list_t *list);

int64_t dll_get_size(list_t *list);

void dll_print_int_list(list_t *list);

void dll_print_string_list(list_t *list);
