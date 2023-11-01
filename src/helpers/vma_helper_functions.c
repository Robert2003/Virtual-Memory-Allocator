// Copyright Damian Mihai-Robert 312CAb 2022-2023
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "list.h"

// Dezaloca doar structura de miniblock_t
void dealloc_miniblock(void *node)
{
	free(node);
	node = NULL;
}

// Dezaloca un intreg nod care contine un miniblock, inclusiv bufferul
void dealloc_mini_node(void *node)
{
	miniblock_t *miniblock = NULL;

	if (node && ((node_t *)node)->data) {
		miniblock = (miniblock_t *)((node_t *)node)->data;

		if (miniblock->rw_buffer) {
			free(miniblock->rw_buffer);
			miniblock->rw_buffer = NULL;
		}
		free(miniblock);
		miniblock = NULL;
	}
	free(node);
	node = NULL;
}

// Dezaloca un intreg nod care contine un block
void dealloc_block_node(void *data)
{
	block_t *block = (block_t *)((node_t *)data)->data;

	free(block->miniblock_list);
	block->miniblock_list = NULL;
	free(block);
	block = NULL;
	free(data);
	data = NULL;
}

/*
Gaseste indexul blockului astfel incat address apartine
[block_start, next_block_start), astfel returneaza indexul blockului in care se
afla address, iar in cazul in care nu apartine, blockul nou ar trebui pus pe
indexul urmator
*/
int get_block_index(list_t *blocks, const uint64_t address)
{
	node_t *block_node = blocks->head;
	block_t *block = NULL;
	int pos = 0;

	while (block_node) {
		block = (block_t *)block_node->data;
		if (block->start_address > address) {
			pos--;
			break;
		}
		pos++;
		block_node = block_node->next;
	}

	if (!block_node)
		pos--;

	return pos;
}

/*
Gaseste indexul miniblockului astfel incat address apartine
[miniblock_start, next_miniblock_start), astfel returneaza indexul
miniblockului in care se afla address, iar in cazul in care nu apartine,
miniblockul nou ar trebui pus pe indexul urmator
*/
int get_miniblock_index(list_t *miniblocks, const uint64_t address)
{
	node_t *miniblock_node = miniblocks->head;
	miniblock_t *miniblock = NULL;
	int pos = 0;

	while (miniblock_node) {
		miniblock = (miniblock_t *)miniblock_node->data;
		if (miniblock->start_address > address) {
			pos--;
			break;
		}
		pos++;
		miniblock_node = miniblock_node->next;
	}

	if (!miniblock_node)
		pos--;

	return pos;
}

block_t *create_block(const uint64_t address, const uint64_t size)
{
	block_t *block;

	block = malloc(sizeof(block_t));
	DIE(!block, "Block malloc failed\n");
	block->size = size;
	block->start_address = address;
	block->miniblock_list = dll_create(sizeof(miniblock_t));

	return block;
}

miniblock_t *create_miniblock(const uint64_t address, const uint64_t size)
{
	miniblock_t *miniblock;

	miniblock = malloc(sizeof(miniblock_t));
	DIE(!miniblock, "Miniblock malloc failed\n");
	miniblock->size = size;
	miniblock->start_address = address;
	miniblock->perm = '6';
	miniblock->rw_buffer = NULL;

	return miniblock;
}

/*
Functie care verifica daca adresa noua se afla in afara arenei, sau daca se
gaseste pe o zona de memorie deja alocata
*/
int check_overlapping(arena_t *arena, const uint64_t address,
					  const uint64_t size)
{
	block_t *block = NULL;
	node_t *node_at_index;
	int index = get_block_index(arena->block_list, address);

	if (address >= arena->arena_size) {
		printf("The allocated address is outside the size of arena\n");
		return 1;
	}

	if (address + size > arena->arena_size) {
		printf("The end address is past the size of the arena\n");
		return 1;
	}

	node_at_index = dll_get_node(arena->block_list, index);

	if (node_at_index && index == -1) {
		block = (block_t *)node_at_index->data;
		if (address + size > block->start_address) {
			printf("This zone was already allocated.\n");
			return 1;
		}
	} else if (node_at_index && index == arena->block_list->size) {
		block = (block_t *)node_at_index->data;
		if (block->start_address + block->size > address) {
			printf("This zone was already allocated.\n");
			return 1;
		}
	} else if (node_at_index) {
		block = (block_t *)node_at_index->data;
		if (block->start_address + block->size > address) {
			printf("This zone was already allocated.\n");
			return 1;
		}

		if (node_at_index->next) {
			block = (block_t *)node_at_index->next->data;
			if (address + size > block->start_address) {
				printf("This zone was already allocated.\n");
				return 1;
			}
		}
	}

	return 0;
}

// Functie care uneste doua blockuri
void merge_lists_at_index(arena_t *arena, int dest, int src)
{
	node_t *dest_node = dll_get_node(arena->block_list, dest);
	node_t *src_node = dll_get_node(arena->block_list, src);
	block_t *dest_block = (block_t *)dest_node->data;
	block_t *src_block = (block_t *)src_node->data;

	// Creste dimensiunea blockului
	dest_block->size += src_block->size;
	// Conexiunile pentru a pune o lista la finalul alteia
	dest_block->miniblock_list->tail->next = src_block->miniblock_list->head;
	src_block->miniblock_list->head->prev = dest_block->miniblock_list->tail;
	dest_block->miniblock_list->tail = src_block->miniblock_list->tail;
	// Creste dimensiunea listei blockului de la indexul dest
	dest_block->miniblock_list->size += src_block->miniblock_list->size;
	/*
	Head-ul listei sursa se face null, deoarece daca ar fi aplicat free pe
	acesta, s-ar sterge si in lista dest, deoarece sunt 2 conexiuni spre
	acelasi nod
	*/
	src_block->miniblock_list->head = NULL;

	dll_remove_node(arena->block_list, src, dealloc_block_node);
}

int minimum(int a, int b) { return a < b ? a : b; }

char *octal_to_permissions(int8_t permission)
{
	char *perm = malloc(4);
	DIE(!perm, "Perm malloc failed\n");
	int mask = 4;
	int i;
	int decimal_perm = permission - '0';

	for (i = 0; i < 3; i++) {
		if (decimal_perm & mask)
			// Se seteaza fiecare permisiune in parte la R, W, X
			perm[i] = (i % 3 == 0) ? 'R' : ((i % 3 == 1) ? 'W' : 'X');
		else
			// Daca bitul este 0, atunci permisiunea lipseste
			perm[i] = '-';
		// Deplasez stanga pentru a verifica urmatoarea permisiune
		mask >>= 1;
	}
	perm[3] = '\0';
	return perm;
}

/*
Functie care verifica daca toate miniblockurile asupra carora se face read
sau write au aceste permisiuni

parametrul perm va fi de tipul 'R' sau 'W', in functie de cazul verificarii
*/
int correct_permissions(node_t *start, int size, char perm)
{
	node_t *node = start;
	miniblock_t *miniblock = (miniblock_t *)node->data;
	int min;
	char *p;

	p = octal_to_permissions(miniblock->perm);
	if (!strchr(p, perm)) {
		free(p);
		return 0;
	}
	free(p);

	node = node->next;
	while (size > 0 && node) {
		miniblock = (miniblock_t *)node->data;
		p = octal_to_permissions(miniblock->perm);
		if (!strchr(p, perm)) {
			free(p);
			return 0;
		}
		free(p);
		min = minimum(size, miniblock->size);
		size -= min;
		node = node->next;
	}
	return 1;
}

// Functie care prelucreaza parametrul cu permisiuni al operatiei MPROTECT
char permissions(char *permission)
{
	char perm = '0';
	if (strstr(permission, "PROT_READ"))
		perm += 4;
	if (strstr(permission, "PROT_WRITE"))
		perm += 2;
	if (strstr(permission, "PROT_EXEC"))
		perm += 1;
	return perm;
}
