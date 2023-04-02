#include <stdio.h>
#include <stdlib.h>

#include "vma.h"
#include "list.h"

int main(void)
{
	arena_t *arena = alloc_arena(12432);
	alloc_block(arena, 100, 200);
	alloc_block(arena, 400, 200);
	alloc_block(arena, 0, 100);
	alloc_block(arena, 600, 100);
	//alloc_block(arena, 300, 100);
	//alloc_block(arena, 300, 100);
	/*alloc_block(arena, 500, 500);
	alloc_block(arena, 2000, 10000);
	alloc_block(arena, 300, 1000);
	alloc_block(arena, 12500, 1000);
	alloc_block(arena, 50, 1000);*/
	//merge_lists_at_index(arena, 0, 1);
	//merge_lists_at_index(arena, 0, 1);
	pmap(arena);
	dealloc_arena(arena);
	return 0;
}
