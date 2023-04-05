// Copyright Damian Mihai-Robert 312CAb 2022-2023
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "vma.h"
#include "list.h"

#define MAX_SIZE 100

int main(void)
{
	arena_t *arena = NULL;
	uint64_t address, size;
	int8_t *data = NULL, ch;
	char command[MAX_SIZE], perm[MAX_SIZE];

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
			DIE(!data, "Data malloc failed\n");
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
			fgets(perm, MAX_SIZE, stdin);
			mprotect(arena, address, perm);
		} else {
			printf("Invalid command. Please try again.\n");
		}
		scanf("%s", command);
	}

	dealloc_arena(arena);
	return 0;
}
