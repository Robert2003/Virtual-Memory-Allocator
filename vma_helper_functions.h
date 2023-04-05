// Copyright Damian Mihai-Robert 312CAb 2022-2023
#pragma once
#include "vma.h"

void dealloc_miniblock(void *node);
void dealloc_mini_node(void *node);
void dealloc_block_node(void *data);
int get_block_index(list_t *blocks, const uint64_t address);
int get_miniblock_index(list_t *miniblocks, const uint64_t address);
block_t *create_block(const uint64_t address, const uint64_t size);
miniblock_t *create_miniblock(const uint64_t address, const uint64_t size);
int check_overlapping(arena_t *arena, const uint64_t address,
					  const uint64_t size);
void merge_lists_at_index(arena_t *arena, int dest, int src);
int minimum(int a, int b);
char *octal_to_permissions(int8_t permission);
int correct_permissions(node_t *start, int size, char perm);
char permissions(char *permission);
