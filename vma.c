#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "vma.h"
#include "list.h"

void dealloc_miniblock(void *node)
{
	free(node);
	node = NULL;
}

void dealloc_miniblock_node(void *node) {
	if(node && ((node_t *)node)->data) {
		miniblock_t *miniblock = (miniblock_t *)((node_t *)node)->data;

		if(miniblock->rw_buffer) {
			free(miniblock->rw_buffer);
			miniblock->rw_buffer = NULL;
		}
		free(miniblock);
		miniblock = NULL;
	}
	free(node);
	node = NULL;
}

void dealloc_block_node(void *data) {
	block_t *block = (block_t *)((node_t *)data)->data;

	free(block->miniblock_list);
	block->miniblock_list = NULL;
	free(block);
	block = NULL;
	free(data);
	data = NULL;
}

arena_t *alloc_arena(const uint64_t size)
{
	arena_t *arena = malloc(sizeof(arena_t));
	arena->arena_size = size;

	arena->block_list = dll_create(sizeof(list_t));

	return arena;
}

void dealloc_arena(arena_t *arena)
{
	block_t *block;
	miniblock_t *miniblock;
	int cnt_b = 0, cnt_m = 0;

	node_t *it = arena->block_list->head;
	while(it) {
		block = (block_t *)it->data;
		node_t *jt = block->miniblock_list->head;
		while(jt) {
			dll_remove_nth_node(block->miniblock_list, 0, dealloc_miniblock_node);
			jt = block->miniblock_list->head;
		}
		dll_remove_nth_node(arena->block_list, 0, dealloc_block_node);
		it = arena->block_list->head;
	}
	free(arena->block_list);
	free(arena);
}

int get_block_index(list_t *blocks, const uint64_t address)
{
	int pos = 0;
	node_t *it = blocks->head;

	while(it) {
		block_t *block = (block_t *)it->data;
		if(block->start_address > address) {
			pos--;
			break;
		}
		pos++;
		it = it->next;
	}

	if(!it)
		pos--;
		
	return pos;
}

block_t *create_block(const uint64_t address, const uint64_t size)
{
	block_t *block;

	block = malloc(sizeof(block_t));
	block->size = size;
	block->start_address = address;
	block->miniblock_list = dll_create(sizeof(miniblock_t));

	return block;
}

miniblock_t *create_miniblock(const uint64_t address, const uint64_t size)
{
	miniblock_t *miniblock;

	miniblock = malloc(sizeof(miniblock_t));
	miniblock->size = size;
	miniblock->start_address = address;
	miniblock->perm = 42;
	miniblock->rw_buffer = NULL;

	return miniblock;
}

int check_overlapping(arena_t *arena, const uint64_t address, const uint64_t size)
{
	block_t *block;
	int index = get_block_index(arena->block_list, address);
	node_t *node_at_index;

	if (address >= arena->arena_size) {
		printf("The allocated address is outside the size of arena\n");
		return 1;
	}

	if(address + size > arena->arena_size) {
		printf("The end address is past the size of the arena\n");
		return 1;
	}

	node_at_index = dll_get_nth_node(arena->block_list, index);

	if (node_at_index && index == -1) {
		block = (block_t *)node_at_index->data;
		if (address + size > block->start_address) {
			printf("This zone was already allocated.\n");
			return 1;
		}
	} else if (node_at_index &&  index == arena->block_list->size) {
		block = (block_t *)node_at_index->data;
		if (block->start_address + block->size > address) {
			printf("This zone was already allocated.\n");
			return 1;
		}
	} else if (node_at_index){
		block = (block_t *)node_at_index->data;
		if (block->start_address + block->size > address) {
			printf("This zone was already allocated.\n");
			return 1;
		}

		if(node_at_index->next) {
			block = (block_t *)node_at_index->next->data;
			if (address + size > block->start_address) {
				printf("This zone was already allocated.\n");
				return 1;
			}
		}
	}

	return 0;
}

void merge_lists_at_index(arena_t *arena, int dest, int src)
{
	node_t *dest_node = dll_get_nth_node(arena->block_list, dest);
	node_t *src_node = dll_get_nth_node(arena->block_list, src);
	block_t *dest_block = (block_t *)dest_node->data;
	block_t *src_block = (block_t *)src_node->data;

	dest_block->size += src_block->size;
	dest_block->miniblock_list->tail->next = src_block->miniblock_list->head;
	src_block->miniblock_list->head->prev = dest_block->miniblock_list->tail;
	dest_block->miniblock_list->tail = src_block->miniblock_list->tail;
	dest_block->miniblock_list->size += src_block->miniblock_list->size;
	src_block->miniblock_list->head = NULL;

	dll_remove_nth_node(arena->block_list, src, dealloc_block_node);
}

void alloc_block(arena_t *arena, const uint64_t address, const uint64_t size)
{
	node_t *curr_node = NULL, *next_node = NULL;
	block_t *block, *next_block;
	miniblock_t *miniblock;
	int index = get_block_index(arena->block_list, address);

	if (check_overlapping(arena, address, size))
		return;

	curr_node = dll_get_nth_node(arena->block_list, index);
	
	if (!curr_node) {
		miniblock = create_miniblock(address, size);
		block = create_block(address, size);
		dll_add_nth_node(block->miniblock_list, block->miniblock_list->size, miniblock);
		dll_add_nth_node(arena->block_list, index + 1, block);
		return;
	}

	block = (block_t *)curr_node->data;
	if(curr_node->next) {
		next_node = curr_node->next;
		next_block = (block_t *)next_node->data;
	}

	miniblock = create_miniblock(address, size);
	// Daca adaug inainte de primul nod si trebuie combinate
	if(index == -1 && address + size == block->start_address) {
		dll_add_nth_node(block->miniblock_list, 0, miniblock);
		block->start_address = address;
		block->size += size;
	// Daca adaug dupa ultimul nod si trebuie combinate
	} else if (index == arena->block_list->size && block->start_address + block->size == address) {
		dll_add_nth_node(block->miniblock_list, block->miniblock_list->size, miniblock);
		block->size += size;
	// Daca adaug la mijloc
	} else {
		// Daca miniblockul trebuie adaugat la sfarsitul primul block gasit
		if(block->start_address + block->size == address) {
			dll_add_nth_node(block->miniblock_list, block->miniblock_list->size, miniblock);
			block->size += size;
			// Daca dupa adaugare am umplut zona dintre 2 blockuri si trebuie si ele legate
			if(next_node && address + size == next_block->start_address) {
				merge_lists_at_index(arena, index, index + 1);
			}
		// Daca miniblockul trebuie adaugat la inceputul blockului de dupa primul gasit
		} else if (next_node && address + size == next_block->start_address) {
			dll_add_nth_node(next_block->miniblock_list, 0, miniblock);
			next_block->start_address = address;
			next_block->size += size;
		// Daca trebuie doar creat un nou nod
		} else {
			block = create_block(address, size);
			dll_add_nth_node(block->miniblock_list, block->miniblock_list->size, miniblock);
			dll_add_nth_node(arena->block_list, index + 1, block);
		}
	}
}

int get_miniblock_index(list_t *miniblocks, const uint64_t address)
{
	int pos = 0;
	node_t *it = miniblocks->head;

	while(it) {
		miniblock_t *block = (miniblock_t *)it->data;
		if(block->start_address > address) {
			pos--;
			break;
		}
		pos++;
		it = it->next;
	}

	if(!it)
		pos--;
		
	return pos;
}

void free_block(arena_t *arena, const uint64_t address)
{
	int miniblock_index, size = 0;;
	int block_index = get_block_index(arena->block_list, address);
	node_t *block_node;

	if (block_index < 0) {
		printf("Invalid address for free.\n");
		return;
	}

	block_node = dll_get_nth_node(arena->block_list, block_index);

	block_t *block = (block_t *)block_node->data;

	if (!(block->start_address <= address && block->start_address + block->size >= address)) {
		printf("Invalid address for free.\n");
		return;
	}

	miniblock_index = get_miniblock_index(block->miniblock_list, address);
	node_t *miniblock_node = dll_get_nth_node(block->miniblock_list, miniblock_index);

	miniblock_t *miniblock = (miniblock_t *)miniblock_node->data;

	if (miniblock->start_address != address) {
		printf("Invalid address for free.\n");
		return;
	}

	if (block->miniblock_list->size == 1) {
		dll_remove_nth_node(block->miniblock_list, miniblock_index, dealloc_miniblock_node);
		dll_remove_nth_node(arena->block_list, block_index, dealloc_block_node);
	} else {
		if (miniblock_index > 0 && miniblock_index < block->miniblock_list->size - 1) {
			block_t *left_block = create_block(block->start_address, miniblock->start_address - block->start_address);
			block_t *right_block = create_block(miniblock->start_address + miniblock->size, block->start_address + block->size - (miniblock->start_address + miniblock->size));
			
			for (int i = 0; i < miniblock_index; i++) {
				dll_add_nth_node(left_block->miniblock_list, i, block->miniblock_list->head->data);
				dll_remove_nth_node(block->miniblock_list, 0, dealloc_miniblock);
			}
			dll_remove_nth_node(block->miniblock_list, 0, dealloc_miniblock_node);
			for (int i = 0; block->miniblock_list->size; i++) {
				dll_add_nth_node(right_block->miniblock_list, i, block->miniblock_list->head->data);
				dll_remove_nth_node(block->miniblock_list, 0, dealloc_miniblock);
			}
			
			dll_remove_nth_node(arena->block_list, block_index, dealloc_block_node);
			dll_add_nth_node(arena->block_list, block_index, left_block);
			dll_add_nth_node(arena->block_list, block_index + 1, right_block);

		} else {
			block->size -= miniblock->size;
			dll_remove_nth_node(block->miniblock_list, miniblock_index, dealloc_miniblock_node);
			block->start_address = ((miniblock_t *)block->miniblock_list->head->data)->start_address;
		}
	}
}

int minimum(int a, int b)
{
	return a < b ? a : b;
}

void read(arena_t *arena, uint64_t address, uint64_t size)
{
	int miniblock_index;
	int block_index = get_block_index(arena->block_list, address);
	node_t *block_node;

	if (block_index < 0) {
		printf("Invalid address for read.\n");
		return;
	}

	block_node = dll_get_nth_node(arena->block_list, block_index);

	block_t *block = (block_t *)block_node->data;

	if (!(block->start_address <= address && block->start_address + block->size >= address)) {
		printf("Invalid address for read.\n");
		return;
	}

	if(block->start_address + block->size < address + size) {
		size = block->start_address + block->size - address;
		printf("Warning: size was bigger than the block size. Reading %lu characters.\n", size);
	}

	miniblock_index = get_miniblock_index(block->miniblock_list, address);
	node_t *miniblock_node = dll_get_nth_node(block->miniblock_list, miniblock_index);

	miniblock_t *miniblock = (miniblock_t *)miniblock_node->data;

	if(miniblock->rw_buffer) {
		int min = minimum(size, miniblock->start_address + miniblock->size - address);
		fwrite(miniblock->rw_buffer + (address - miniblock->start_address),
			sizeof(uint8_t), min, stdout);
		size -= min;

		miniblock_node = miniblock_node->next;
		while(size > 0 && miniblock_node) {
			miniblock_t *miniblock = (miniblock_t *)miniblock_node->data;
			if(miniblock->rw_buffer) {
				min = minimum(size, miniblock->size);
				fwrite(miniblock->rw_buffer, sizeof(uint8_t), min, stdout);
				size -= min;
				miniblock_node = miniblock_node->next;
			} else 
				break;
		}
		printf("\n");
	}
}

void write(arena_t *arena, const uint64_t address, const uint64_t size, int8_t *data)
{
	int miniblock_index, cursor_index = 0;
	int block_index = get_block_index(arena->block_list, address);
	node_t *block_node;
	uint64_t new_size = size;

	if (block_index < 0) {
		printf("Invalid address for write.\n");
		return;
	}

	block_node = dll_get_nth_node(arena->block_list, block_index);

	block_t *block = (block_t *)block_node->data;

	if (!(block->start_address <= address && block->start_address + block->size >= address)) {
		printf("Invalid address for write.\n");
		return;
	}

	if(block->start_address + block->size < address + size) {
		new_size = block->start_address + block->size - address;
		printf("Warning: size was bigger than the block size. Writing %lu characters.\n", new_size);
	}

	miniblock_index = get_miniblock_index(block->miniblock_list, address);
	node_t *miniblock_node = dll_get_nth_node(block->miniblock_list, miniblock_index);

	miniblock_t *miniblock = (miniblock_t *)miniblock_node->data;

	int min = minimum(new_size, miniblock->start_address + miniblock->size - address);
	if(!miniblock->rw_buffer)
		miniblock->rw_buffer = calloc(miniblock->size, sizeof(uint8_t));
	memcpy(miniblock->rw_buffer + (address - miniblock->start_address),
			data + cursor_index, min);
	cursor_index += min;
	new_size -= min;

	miniblock_node = miniblock_node->next;
	while(new_size > 0 && miniblock_node) {
		miniblock_t *miniblock = (miniblock_t *)miniblock_node->data;
		min = minimum(new_size, miniblock->size);
		if(!miniblock->rw_buffer)
			miniblock->rw_buffer = calloc(miniblock->size, sizeof(uint8_t));
		memcpy(miniblock->rw_buffer, data + cursor_index, min);
		cursor_index += min;
		new_size -= min;
		miniblock_node = miniblock_node->next;
	}
}

void pmap(const arena_t *arena)
{
	block_t *block;
	miniblock_t *miniblock;
	int cnt_b = 1, cnt_m = 1;
	uint64_t free_memory = arena->arena_size, total_miniblocks = 0;

	node_t *it = arena->block_list->head;
	while(it) {
		block = (block_t *)it->data;
		total_miniblocks += block->miniblock_list->size;
		free_memory -= block->size;
		it = it->next;
	}

	printf("Total memory: 0x%lX bytes\n", arena->arena_size);
	printf("Free memory: 0x%lX bytes\n", free_memory);
	printf("Number of allocated blocks: %d\n", arena->block_list->size);
	printf("Number of allocated miniblocks: %ld\n", total_miniblocks);

	it = arena->block_list->head;
	while(it) {
		block = (block_t *)it->data;
		printf("\nBlock %d begin\n", cnt_b);
		printf("Zone: 0x%lX - 0x%lX\n", block->start_address, block->start_address + block->size);
		//printf("Block %d:\n\tstart_address: %lu\n\tsize: %lu\n\tend_address: %lu\n\n", cnt_b, block->start_address, block->size, block->size+block->start_address);
		node_t *jt = block->miniblock_list->head;
		cnt_m = 1;
		while(jt) {
			miniblock = (miniblock_t *)jt->data;
			printf("Miniblock %d:\t\t0x%lX\t\t-\t\t0x%lX\t\t| ", cnt_m, miniblock->start_address, miniblock->start_address + miniblock->size);
			if(miniblock->perm == 42)
				printf("RW-");
			printf("\n");
			//printf("\tMiniblock %d:\n\t\tstart_address: %lu\n\t\tsize: %lu\tend_address: %lu\n", cnt_m, miniblock->start_address, miniblock->size, miniblock->size+miniblock->start_address);
			cnt_m++;
			jt = jt->next;
		}
		printf("Block %d end\n", cnt_b);
		it = it->next;
		cnt_b++;
	}
}

void mprotect(arena_t *arena, uint64_t address, int8_t *permission)
{

}
