#include <stdio.h>
#include <stdlib.h>

#include "vma.h"
#include "list.h"

int main(void)
{
	arena_t *arena = alloc_arena(65536);
	alloc_block(arena, 4096, 10);
	alloc_block(arena, 12288, 10);
	alloc_block(arena, 12308, 10);
	pmap(arena);
	alloc_block(arena, 12298, 10);
	pmap(arena);
	//free_block(arena, 12308);
	pmap(arena);
	read(arena, 12289, 25);
	alloc_block(arena, 0, 1500);
	dealloc_arena(arena);
	return 0;
}
