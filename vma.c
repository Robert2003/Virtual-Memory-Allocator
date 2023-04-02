#include <stdlib.h>
#include <stdio.h>

#include "vma.h"
#include "list.h"

arena_t *alloc_arena(const uint64_t size)
{
    arena_t *arena = malloc(sizeof(arena_t));
    arena->arena_size = size;

    arena->block_list = dll_create(sizeof(list_t));

    return arena;
}

void dealloc_arena(arena_t *arena)
{
    node_t *it = NULL;
    list_t *miniblock_list;

    it = arena->block_list->head;
    for(it; it != NULL; it = it->next) {
        miniblock_list = ((block_t *)it->data)->miniblock_list;
        dll_free(miniblock_list);
        //free(miniblock_list);
    }
    dll_free(arena->block_list);
    //free(arena->block_list);
    free(arena);
    arena = NULL;
}

int get_block_index(list_t *blocks, const uint64_t address)
{
    int pos = 0;
    node_t *it = blocks->head;

    while(it) {
        block_t *block = (block_t *)it->data;
        if(block->start_address >= address) {
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
    miniblock->perm = 0;

    return miniblock;
}

int check_overlapping(arena_t *arena, const uint64_t address, const uint64_t size)
{
    block_t *block;
    int index = get_block_index(arena->block_list, address);
    node_t *node_at_index;

    if (index == -1)
        node_at_index = dll_get_nth_node(arena->block_list, index + 1);
    else if (index == arena->block_list->size)
        node_at_index = dll_get_nth_node(arena->block_list, index - 1);
    else
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

    src_node = dll_remove_nth_node(arena->block_list, src);
}

void alloc_block(arena_t *arena, const uint64_t address, const uint64_t size)
{
    node_t *curr_node = NULL, *next_node = NULL;
    block_t *block = NULL, *next_block = NULL;
    miniblock_t *miniblock;
    int index = get_block_index(arena->block_list, address);

    if (check_overlapping(arena, address, size))
        return;
    
    if (index == -1)
        curr_node = dll_get_nth_node(arena->block_list, index + 1);
    else
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
        //printf("Blockul asta se termina la: %ld\n", block->start_address + block->size);
        //printf("Blockul nou incepe la: %ld\n", address);
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
    block_t *block;
    miniblock_t *miniblock;
    int cnt_b = 0, cnt_m = 0;

    node_t *it = arena->block_list->head;
    while(it) {
        block = (block_t *)it->data;
        printf("Block %d:\n\tstart_address: %lu\n\tsize: %lu\n\tend_address: %lu\n\n", cnt_b, block->start_address, block->size, block->size+block->start_address);
        cnt_b++;
        node_t *jt = block->miniblock_list->head;
        while(jt) {
            miniblock = (miniblock_t *)jt->data;
            printf("\tMiniblock %d:\n\t\tstart_address: %lu\n\t\tsize: %lu\tend_address: %lu\n", cnt_m, miniblock->start_address, miniblock->size, miniblock->size+miniblock->start_address);
            cnt_m++;
            jt = jt->next;
        }
        it = it->next;
    }
}

void mprotect(arena_t *arena, uint64_t address, int8_t *permission)
{

}
