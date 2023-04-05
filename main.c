#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "vma.h"
#include "list.h"

int main(void)
{
	char command[100];
	uint64_t address, size;
	arena_t *arena = NULL;
	int8_t *data = NULL, ch;
	char perm[100];

	scanf("%s", command);

	while (strcmp(command, "DEALLOC_ARENA")) {
		if (!strcmp(command, "ALLOC_ARENA")) {
			scanf("%ld", &size);
			arena = alloc_arena(size);
		} else if (!strcmp(command, "ALLOC_BLOCK")) {
			scanf("%ld %ld", &address, &size);
			alloc_block(arena, address, size);
		} else if (!strcmp(command, "WRITE")) {
			scanf("%ld %ld", &address, &size);
			scanf("%c", &ch);
			data = malloc((size + 1) * sizeof(char));
			fread(data, size, sizeof(char), stdin);
			write(arena, address, size, data);
			free(data);
			data = NULL;
		} else if (!strcmp(command, "READ")) {
			scanf("%ld %ld", &address, &size);
			read(arena, address, size);
		} else if (!strcmp(command, "PMAP")) {
			pmap(arena);
		} else if (!strcmp(command, "FREE_BLOCK")) {
			scanf("%ld", &address);
			free_block(arena, address);
		} else if (!strcmp(command, "MPROTECT")) {
			scanf("%ld", &address);
			fgets(perm, 100, stdin);
			mprotect(arena, address, perm);
		} else {
			printf("Invalid command. Please try again.\n");
		}
		scanf("%s", command);
	}
	dealloc_arena(arena);
	return 0;
}
