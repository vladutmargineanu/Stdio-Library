#include <stdlib.h>
#include <fcntl.h>
#include <string.h>

#include "so_utils.h"

int choose_mode(const char *mode)
{
	if (strcmp(mode, "r") == 0)
		return O_RDONLY;

	if (strcmp(mode, "r+") == 0)
		return O_RDWR;

	if (strcmp(mode, "w") == 0)
		return O_WRONLY | O_CREAT | O_TRUNC;

	if (strcmp(mode, "w+") == 0)
		return O_RDWR | O_CREAT | O_TRUNC;

	if (strcmp(mode, "a") == 0)
		return O_WRONLY | O_CREAT | O_APPEND;

	if (strcmp(mode, "a+") == 0)
		return O_RDWR | O_CREAT | O_APPEND;

	return -1;
}

SO_FILE *allocate_memory(const char *pathname, const char *mode)
{
	SO_FILE *stream = malloc(sizeof(struct _so_file));

	if (pathname == NULL)
		stream->pathname = NULL;
	else {
		stream->pathname = malloc(strlen(pathname) + 1);

		if (stream->pathname == NULL)
			return NULL;

		strcpy(stream->pathname, pathname);
	}

	if (mode == NULL)
		stream->mode = NULL;
	else {
		stream->mode = malloc(strlen(mode) + 1);

		if (stream->mode == NULL)
			return NULL;

		strcpy(stream->mode, mode);
	}

	stream->buffer = malloc(sizeof(char) * BUFSIZE);

	if (stream->buffer == NULL)
		return NULL;

	memset(stream->buffer, 0, BUFSIZE);

	return stream;
}

void free_memory(SO_FILE *stream)
{
	/* eliberam memoria alocata dinamic */
	free(stream->buffer);
	stream->buffer = NULL;

	free(stream->pathname);
	stream->pathname = NULL;

	free(stream->mode);
	stream->mode = NULL;

	free(stream);
	stream = NULL;
}

void initialize_file(SO_FILE *stream)
{
	stream->fd = 0;
	stream->offset = 0;
	stream->currBuffer = 0;
	/* Initializam structura interioara INFO_FILE */
	stream->info.flagError = 0;
	stream->info.flagEof = 0;
	stream->info.opRead = 0;
	stream->info.opWrite = 0;
	stream->info.pidFile = 0;
	stream->info.currPos = 0;
	stream->info.ftellBytes = 0;
}
