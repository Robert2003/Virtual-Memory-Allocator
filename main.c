#include <stdio.h>
#include <stdlib.h>

#include "vma.h"
#include "list.h"

int main(void)
{
	list_t *list = dll_create(sizeof(int));
	int v[] = {0,1,2,3,4,5,6,7,9,9};
	dll_add_nth_node(list, 0, &v[0]);
	dll_add_nth_node(list, 1, &v[1]);
	dll_add_nth_node(list, 2, &v[2]);
	dll_add_nth_node(list, 1, &v[3]);
	dll_add_nth_node(list, 0, &v[4]);
	dll_remove_nth_node(list, 0);
	dll_remove_nth_node(list, 0);
	dll_remove_nth_node(list, 0);
	dll_remove_nth_node(list, 0);
	dll_remove_nth_node(list, 0);
	dll_remove_nth_node(list, 0);
	dll_print_int_list(list);
	arena_t *arena = alloc_arena(12432);
	dealloc_arena(arena);
	return 0;
}
