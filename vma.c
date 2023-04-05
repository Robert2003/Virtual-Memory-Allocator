// Copyright Damian Mihai-Robert 312CAb 2022-2023
#include "vma.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "list.h"
#include "vma_helper_functions.h"

arena_t *alloc_arena(const uint64_t size)
{
	arena_t *arena = malloc(sizeof(arena_t));
	DIE(!arena, "Arena malloc failed\n");
	arena->arena_size = size;
	arena->block_list = dll_create(sizeof(list_t));

	return arena;
}

void dealloc_arena(arena_t *arena)
{
	block_t *block;
	node_t *block_node = NULL, *miniblock_node = NULL;

	block_node = arena->block_list->head;
	while (block_node) {
		block = (block_t *)block_node->data;
		miniblock_node = block->miniblock_list->head;
		while (miniblock_node) {
			dll_remove_node(block->miniblock_list, 0, dealloc_mini_node);
			miniblock_node = block->miniblock_list->head;
		}
		dll_remove_node(arena->block_list, 0, dealloc_block_node);
		block_node = arena->block_list->head;
	}
	free(arena->block_list);
	free(arena);
}

void alloc_block(arena_t *arena, const uint64_t address, const uint64_t size)
{
	node_t *curr_node = NULL, *next_node = NULL;
	block_t *block, *next_block;
	miniblock_t *miniblock;
	int index = get_block_index(arena->block_list, address), dll_size;

	if (check_overlapping(arena, address, size))
		return;

	curr_node = dll_get_node(arena->block_list, index);

	if (!curr_node) {
		miniblock = create_miniblock(address, size);
		block = create_block(address, size);
		dll_size = block->miniblock_list->size;
		dll_add_node(block->miniblock_list, dll_size, miniblock);
		dll_add_node(arena->block_list, index + 1, block);
		return;
	}

	block = (block_t *)curr_node->data;
	if (curr_node->next) {
		next_node = curr_node->next;
		next_block = (block_t *)next_node->data;
	}

	miniblock = create_miniblock(address, size);
	// Daca adaug inainte de primul nod si trebuie combinate
	if (index == -1 && address + size == block->start_address) {
		dll_add_node(block->miniblock_list, 0, miniblock);
		block->start_address = address;
		block->size += size;
		// Daca adaug dupa ultimul nod si trebuie combinate
	} else if (index == arena->block_list->size &&
			   block->start_address + block->size == address) {
		dll_size = block->miniblock_list->size;
		dll_add_node(block->miniblock_list, dll_size, miniblock);
		block->size += size;
		// Daca adaug la mijloc
	} else {
		// Daca miniblockul trebuie adaugat la sfarsitul primul block gasit
		if (block->start_address + block->size == address) {
			dll_size = block->miniblock_list->size;
			dll_add_node(block->miniblock_list, dll_size, miniblock);
			block->size += size;
			// Daca dupa adaugare am umplut zona dintre 2 blockuri si trebuie si
			// ele legate
			if (next_node && address + size == next_block->start_address)
				merge_lists_at_index(arena, index, index + 1);
			// Daca miniblockul trebuie adaugat la inceputul blockului de dupa
			// primul gasit
		} else if (next_node && address + size == next_block->start_address) {
			dll_add_node(next_block->miniblock_list, 0, miniblock);
			next_block->start_address = address;
			next_block->size += size;
			// Daca trebuie doar creat un nou nod
		} else {
			block = create_block(address, size);
			dll_size = block->miniblock_list->size;
			dll_add_node(block->miniblock_list, dll_size, miniblock);
			dll_add_node(arena->block_list, index + 1, block);
		}
	}
}

void free_block(arena_t *arena, const uint64_t address)
{
	node_t *block_node, *miniblock_node = NULL;
	block_t *block = NULL, *left_block = NULL, *right_block = NULL;
	miniblock_t *miniblock = NULL;
	int miniblock_index, size, start_address;
	int block_index = get_block_index(arena->block_list, address);

	// Daca adresa gasita este inainte de primul block
	if (block_index < 0) {
		printf("Invalid address for free.\n");
		return;
	}

	block_node = dll_get_node(arena->block_list, block_index);
	block = (block_t *)block_node->data;

	// Daca adresa nu se afla in interiorul blockului gasit
	if (!(block->start_address <= address &&
		  block->start_address + block->size >= address)) {
		printf("Invalid address for free.\n");
		return;
	}

	miniblock_index = get_miniblock_index(block->miniblock_list, address);
	miniblock_node = dll_get_node(block->miniblock_list, miniblock_index);
	miniblock = (miniblock_t *)miniblock_node->data;

	// Daca adresa nu este o adresa de inceput al unui miniblock
	if (miniblock->start_address != address) {
		printf("Invalid address for free.\n");
		return;
	}

	// Daca este unicul miniblock sterg
	if (block->miniblock_list->size == 1) {
		dll_remove_node(block->miniblock_list, miniblock_index,
						dealloc_mini_node);
		dll_remove_node(arena->block_list, block_index, dealloc_block_node);
	// Daca trebuie sa sterg un miniblock din mijloc
	} else if (miniblock_index > 0 &&
			   miniblock_index < block->miniblock_list->size - 1) {
		size = miniblock->start_address - block->start_address;
		left_block = create_block(block->start_address, size);

		start_address = miniblock->start_address + miniblock->size;
		size = block->start_address + block->size - start_address;
		right_block = create_block(start_address, size);

		// Creez blockul cu nodurile din stanga celui care va fi sters
		for (int i = 0; i < miniblock_index; i++) {
			dll_add_node(left_block->miniblock_list, i,
						 block->miniblock_list->head->data);
			dll_remove_node(block->miniblock_list, 0, dealloc_miniblock);
		}
		// Sterg miniblockul gasit
		dll_remove_node(block->miniblock_list, 0, dealloc_mini_node);
		// Creez blockul cu nodurile din dreapta celui care va fi sters
		for (int i = 0; block->miniblock_list->size; i++) {
			dll_add_node(right_block->miniblock_list, i,
						 block->miniblock_list->head->data);
			dll_remove_node(block->miniblock_list, 0, dealloc_miniblock);
		}
		// Sterg blockul vechi si adaug cele doua blockuri noi
		dll_remove_node(arena->block_list, block_index, dealloc_block_node);
		dll_add_node(arena->block_list, block_index, left_block);
		dll_add_node(arena->block_list, block_index + 1, right_block);
	// Daca sterg de la inceputul/sfarsitul blockului
	} else {
		block->size -= miniblock->size;
		dll_remove_node(block->miniblock_list, miniblock_index,
						dealloc_mini_node);
		block->start_address =
			((miniblock_t *)block->miniblock_list->head->data)->start_address;
	}
}

void read(arena_t *arena, uint64_t address, uint64_t size)
{
	node_t *block_node = NULL, *miniblock_node = NULL;
	block_t *block = NULL;
	miniblock_t *miniblock = NULL;
	int block_index = get_block_index(arena->block_list, address);
	int miniblock_index;

	// Daca adresa nu este o adresa de inceput al unui miniblock
	if (block_index < 0) {
		printf("Invalid address for read.\n");
		return;
	}

	block_node = dll_get_node(arena->block_list, block_index);
	block = (block_t *)block_node->data;

	// Daca adresa nu se afla in interiorul blockului gasit
	if (!(block->start_address <= address &&
		  block->start_address + block->size >= address)) {
		printf("Invalid address for read.\n");
		return;
	}

	// Daca ies din dimensiunile blockului
	if (block->start_address + block->size < address + size) {
		size = block->start_address + block->size - address;
		printf("Warning: size was bigger than the block size. "
			  "Reading %lu characters.\n", size);
	}

	miniblock_index = get_miniblock_index(block->miniblock_list, address);
	miniblock_node = dll_get_node(block->miniblock_list, miniblock_index);

	if (!correct_permissions(miniblock_node, size, 'R')) {
		printf("Invalid permissions for read.\n");
		return;
	}

	miniblock = (miniblock_t *)miniblock_node->data;

	if (miniblock->rw_buffer) {
		// min este folosit pentru a printa fie size-ul ramas caractere, fie
		// dimensiunea ramasa din block
		int min =
			minimum(size, miniblock->start_address + miniblock->size - address);
		fwrite(miniblock->rw_buffer + (address - miniblock->start_address),
			   sizeof(int8_t), min, stdout);
		size -= min;

		miniblock_node = miniblock_node->next;
		while (size > 0 && miniblock_node) {
			miniblock = (miniblock_t *)miniblock_node->data;
			if (miniblock->rw_buffer) {
				min = minimum(size, miniblock->size);
				fwrite(miniblock->rw_buffer, sizeof(int8_t), min, stdout);
				size -= min;
				miniblock_node = miniblock_node->next;
			} else {
				break;
			}
		}
		printf("\n");
	}
}

void write(arena_t *arena, const uint64_t address, const uint64_t size,
		   int8_t *data)
{
	node_t *block_node = NULL, *miniblock_node = NULL;
	block_t *block = NULL;
	miniblock_t *miniblock = NULL;
	uint64_t new_size = size;
	int block_index = get_block_index(arena->block_list, address);
	int miniblock_index, cursor_index = 0, min;

	// Daca adresa nu este o adresa de inceput al unui miniblock
	if (block_index < 0) {
		printf("Invalid address for write.\n");
		return;
	}

	block_node = dll_get_node(arena->block_list, block_index);
	block = (block_t *)block_node->data;

	// Daca adresa nu se afla in interiorul blockului gasit
	if (!(block->start_address <= address &&
		  block->start_address + block->size >= address)) {
		printf("Invalid address for write.\n");
		return;
	}

	// Daca ies din dimensiunile blockului
	if (block->start_address + block->size < address + size) {
		new_size = block->start_address + block->size - address;
		printf("Warning: size was bigger than the block size. "
			  "Writing %lu characters.\n", new_size);
	}

	miniblock_index = get_miniblock_index(block->miniblock_list, address);
	miniblock_node = dll_get_node(block->miniblock_list, miniblock_index);

	if (!correct_permissions(miniblock_node, size, 'W')) {
		printf("Invalid permissions for write.\n");
		return;
	}

	miniblock = (miniblock_t *)miniblock_node->data;

	min = minimum(new_size,
				  miniblock->start_address + miniblock->size - address);
	if (!miniblock->rw_buffer) {
		// Aloc memorie doar pentru bufferele utilizate
		miniblock->rw_buffer = calloc(miniblock->size, sizeof(int8_t));
		DIE(!miniblock->rw_buffer, "Buffer calloc failed\n");
	}
	memcpy(miniblock->rw_buffer + (address - miniblock->start_address),
		   data + cursor_index, min);
	// cursor_index este pozitia din bufferul din care copiez
	cursor_index += min;
	new_size -= min;

	miniblock_node = miniblock_node->next;
	while (new_size > 0 && miniblock_node) {
		miniblock = (miniblock_t *)miniblock_node->data;
		min = minimum(new_size, miniblock->size);
		if (!miniblock->rw_buffer) {
			miniblock->rw_buffer = calloc(miniblock->size, sizeof(int8_t));
			DIE(!miniblock->rw_buffer, "Buffer calloc failed\n");
		}
		memcpy(miniblock->rw_buffer, data + cursor_index, min);
		cursor_index += min;
		new_size -= min;
		miniblock_node = miniblock_node->next;
	}
}

void pmap(const arena_t *arena)
{
	node_t *block_node = NULL, *miniblock_node = NULL;
	block_t *block = NULL;
	miniblock_t *miniblock = NULL;
	uint64_t free_memory = arena->arena_size, total_miniblocks = 0;
	int cnt_b = 1, cnt_m = 1;

	block_node = arena->block_list->head;
	while (block_node) {
		block = (block_t *)block_node->data;
		total_miniblocks += block->miniblock_list->size;
		free_memory -= block->size;
		block_node = block_node->next;
	}

	printf("Total memory: 0x%lX bytes\n", arena->arena_size);
	printf("Free memory: 0x%lX bytes\n", free_memory);
	printf("Number of allocated blocks: %d\n", arena->block_list->size);
	printf("Number of allocated miniblocks: %ld\n", total_miniblocks);

	block_node = arena->block_list->head;
	while (block_node) {
		block = (block_t *)block_node->data;
		printf("\nBlock %d begin\n", cnt_b);
		printf("Zone: 0x%lX - 0x%lX\n", block->start_address,
			   block->start_address + block->size);
		miniblock_node = block->miniblock_list->head;
		cnt_m = 1;
		while (miniblock_node) {
			miniblock = (miniblock_t *)miniblock_node->data;
			printf("Miniblock %d:\t\t0x%lX\t\t-\t\t0x%lX\t\t| ", cnt_m,
				   miniblock->start_address,
				   miniblock->start_address + miniblock->size);
			// Stringul de permisiuni obtinut din cifra in octal
			char *p = octal_to_permissions(miniblock->perm);
			printf("%s\n", p);
			free(p);
			cnt_m++;
			miniblock_node = miniblock_node->next;
		}
		printf("Block %d end\n", cnt_b);
		block_node = block_node->next;
		cnt_b++;
	}
}

void mprotect(arena_t *arena, uint64_t address, char *permission)
{
	node_t *block_node = NULL, *miniblock_node = NULL;
	block_t *block = NULL;
	miniblock_t *miniblock = NULL;
	int block_index = get_block_index(arena->block_list, address);
	int miniblock_index;
	// calculez in octal ce fel de permisiune are
	char perm = permissions(permission);

	// Daca adresa nu este o adresa de inceput al unui miniblock
	if (block_index < 0) {
		printf("Invalid address for mprotect.\n");
		return;
	}

	block_node = dll_get_node(arena->block_list, block_index);
	block = (block_t *)block_node->data;

	// Daca adresa nu se afla in interiorul blockului gasit
	if (!(block->start_address <= address &&
		  block->start_address + block->size >= address)) {
		printf("Invalid address for mprotect.\n");
		return;
	}

	miniblock_index = get_miniblock_index(block->miniblock_list, address);
	miniblock_node = dll_get_node(block->miniblock_list, miniblock_index);
	miniblock = (miniblock_t *)miniblock_node->data;

	// Daca adresa nu este o adresa de inceput al unui miniblock
	if (miniblock->start_address != address) {
		printf("Invalid address for mprotect.\n");
		return;
	}

	if (miniblock)
		miniblock->perm = perm;
}
