#include <stdlib.h>
#include <stdio.h>

#include "vma.h"
#include "list.h"

arena_t *alloc_arena(const uint64_t size)
{
    arena_t *arena = malloc(sizeof(arena_t));
    arena->arena_size = size;

    arena->alloc_list = dll_create(sizeof(list_t));

    return arena;
}

void dealloc_arena(arena_t *arena)
{
    node_t *it = NULL;
    list_t *current_list;

    it = arena->alloc_list->head;
    for(it; it != NULL; it = it->next) {
        current_list = ((block_t *)it->data)->miniblock_list;
        dll_free(current_list);
    }
    dll_free(arena->alloc_list);
    free(arena);
    arena = NULL;
}

void alloc_block(arena_t *arena, const uint64_t address, const uint64_t size)
{

}

void free_block(arena_t *arena, const uint64_t address)
{

}

void read(arena_t *arena, uint64_t address, uint64_t size)
{

}

void write(arena_t *arena, const uint64_t address, const uint64_t size, int8_t *data)
{

}

void pmap(const arena_t *arena)
{

}

void mprotect(arena_t *arena, uint64_t address, int8_t *permission)
{

}
