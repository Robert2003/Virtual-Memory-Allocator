#include <stdlib.h>
#include <stdio.h>

#include "vma.h"
#include "list.h"

void dealloc_block(void *data)
{
    free(data);

    data = NULL;
}

void dealloc_miniblock(void *data)
{
    free(data);

    data = NULL;
}

void dealloc_miniblock_node(void *data) {
    miniblock_t *miniblock = (miniblock_t *)((node_t *)data)->data;

    if(miniblock->rw_buffer) {
        free(miniblock->rw_buffer);
        miniblock->rw_buffer = NULL;
    }
    free(miniblock);
    free(data);
    data = NULL;
}

void dealloc_block_node(void *data) {
    block_t *block = (block_t *)((node_t *)data)->data;

    free(block->miniblock_list);
    free(block);
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
    miniblock->perm = 0;
    miniblock->rw_buffer = NULL;

    return miniblock;
}

int check_overlapping(arena_t *arena, const uint64_t address, const uint64_t size)
{
    block_t *block;
    int index = get_block_index(arena->block_list, address);
    node_t *node_at_index;

    if (address > arena->arena_size) {
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
        dll_add_nth_node(block->miniblock_list, block->miniblock_list->size, miniblock, dealloc_miniblock);
        dll_add_nth_node(arena->block_list, index + 1, block, dealloc_block);
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
        dll_add_nth_node(block->miniblock_list, 0, miniblock, dealloc_miniblock);
        block->start_address = address;
        block->size += size;
    // Daca adaug dupa ultimul nod si trebuie combinate
    } else if (index == arena->block_list->size && block->start_address + block->size == address) {
        dll_add_nth_node(block->miniblock_list, block->miniblock_list->size, miniblock, dealloc_miniblock);
        block->size += size;
    // Daca adaug la mijloc
    } else {
        // Daca miniblockul trebuie adaugat la sfarsitul primul block gasit
        if(block->start_address + block->size == address) {
            dll_add_nth_node(block->miniblock_list, block->miniblock_list->size, miniblock, dealloc_miniblock);
            block->size += size;
            // Daca dupa adaugare am umplut zona dintre 2 blockuri si trebuie si ele legate
            if(next_node && address + size == next_block->start_address) {
                merge_lists_at_index(arena, index, index + 1);
            }
        // Daca miniblockul trebuie adaugat la inceputul blockului de dupa primul gasit
        } else if (next_node && address + size == next_block->start_address) {
            dll_add_nth_node(next_block->miniblock_list, 0, miniblock, dealloc_miniblock);
            next_block->start_address = address;
            next_block->size += size;
        // Daca trebuie doar creat un nou nod
        } else {
            block = create_block(address, size);
            dll_add_nth_node(block->miniblock_list, block->miniblock_list->size, miniblock, dealloc_miniblock);
            dll_add_nth_node(arena->block_list, index + 1, block, dealloc_block);
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
    int miniblock_index;
    int block_index = get_block_index(arena->block_list, address);
    node_t *block_node;

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
                dll_add_nth_node(left_block->miniblock_list, i, block->miniblock_list->head->data, dealloc_miniblock);
                dll_remove_nth_node(block->miniblock_list, 0, dealloc_miniblock_node);
            }
            dll_remove_nth_node(block->miniblock_list, 0, dealloc_miniblock_node);
            for (int i = 0; i < block->miniblock_list->size; i++) {
                dll_add_nth_node(right_block->miniblock_list, i, block->miniblock_list->head->data, dealloc_miniblock);
                dll_remove_nth_node(block->miniblock_list, 0, dealloc_miniblock_node);
            }
            
            dll_remove_nth_node(arena->block_list, block_index, dealloc_miniblock_node);
            dll_add_nth_node(arena->block_list, block_index, left_block, dealloc_block);
            dll_add_nth_node(arena->block_list, block_index + 1, right_block, dealloc_block);

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
        printf("%d\n", min);
        fwrite(miniblock->rw_buffer + (address - miniblock->start_address),
            sizeof(uint8_t), min, stdout);
        size -= min;

        miniblock_node = miniblock_node->next;
        while(size > 0 && miniblock_node) {
            miniblock_t *miniblock = (miniblock_t *)miniblock_node->data;
            if(miniblock->rw_buffer) {
                min = minimum(size, miniblock->size);
                printf("%d\n", min);
                fwrite(miniblock->rw_buffer, sizeof(uint8_t), min, stdout);
                size -= min;
                miniblock_node = miniblock_node->next;
            }
        }
    }
}

void write(arena_t *arena, const uint64_t address, const uint64_t size, int8_t *data)
{
    int miniblock_index;
    int block_index = get_block_index(arena->block_list, address);
    node_t *block_node;

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
        printf("%d\n", min);
        fwrite(miniblock->rw_buffer + (address - miniblock->start_address),
            sizeof(uint8_t), min, stdout);
        size -= min;

        miniblock_node = miniblock_node->next;
        while(size > 0 && miniblock_node) {
            miniblock_t *miniblock = (miniblock_t *)miniblock_node->data;
            if(miniblock->rw_buffer) {
                min = minimum(size, miniblock->size);
                printf("%d\n", min);
                fwrite(miniblock->rw_buffer, sizeof(uint8_t), min, stdout);
                size -= min;
                miniblock_node = miniblock_node->next;
            }
        }
    }
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
        cnt_m = 0;
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
