// Copyright Damian Mihai-Robert 312CAb 2022-2023
#pragma once
#include <inttypes.h>
#include <stddef.h>

typedef struct node_t node_t;
struct node_t {
	void *data;
	node_t *prev, *next;
};

typedef struct list_t list_t;
struct list_t {
	node_t *head, *tail;
	int data_size;
	int size;
};

typedef struct {
	uint64_t start_address;
	size_t size;
	list_t *miniblock_list;
} block_t;

typedef struct {
	uint64_t start_address;
	size_t size;
	char perm;
	void *rw_buffer;
} miniblock_t;

typedef struct {
	uint64_t arena_size;
	list_t *block_list;
} arena_t;

arena_t *alloc_arena(const uint64_t size);
void dealloc_arena(arena_t *arena);

void alloc_block(arena_t *arena, const uint64_t address, const uint64_t size);
void free_block(arena_t *arena, const uint64_t address);

void read(arena_t *arena, uint64_t address, uint64_t size);
void write(arena_t *arena, const uint64_t address, const uint64_t size,
		   int8_t *data);
void pmap(const arena_t *arena);
void mprotect(arena_t *arena, uint64_t address, char *permission);

void merge_lists_at_index(arena_t *arena, int dest, int src);
