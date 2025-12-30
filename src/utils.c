#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include "common.h"

/**
 * The pointer returned by this function must be freed by the caller.
 */
char *readFile(
    char *file,
    int64_t *read_size
)
{
	FILE *fptr = fopen(file, "rb");
	fseek(fptr, 0, SEEK_END);
	*read_size = ftell(fptr);

	char *content = malloc(sizeof(char) * *read_size);
	rewind(fptr);
	int64_t actual_read_size =
	    fread(content, sizeof(char), *read_size, fptr);

	fclose(fptr);
	assert(actual_read_size == *read_size);

	return content;
}
