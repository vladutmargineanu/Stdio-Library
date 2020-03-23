#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/wait.h>

#include "so_utils.h"
#include "so_stdio.h"

int so_fgetc(SO_FILE *stream)
{
	int bytes_read = 0;
	int ret;
	/* Ultima operatie a fost de citire */
	stream->info.opRead = 1;
	stream->info.opWrite = 0;
	if (stream == NULL) {
		stream->info.flagError = 1;
		return SO_EOF;
	}
	/* Daca buffer-ul este gol, citim din fisier */
	if (stream->info.currPos == 0 &&
		stream->buffer[stream->info.currPos] == '\0') {
		bytes_read = read(stream->fd, stream->buffer, BUFSIZE);
		/* Daca am terminat de parcurs buffer-ul */
		stream->info.currPos = 0;

		/* Verificam daca am ajuns la final de fisier */
		if (bytes_read == 0) {
			stream->info.flagEof = 1;
			return SO_EOF;
		}
		/* Cati octeti am citit din fisier in buffer */
		if (bytes_read > 0)
			stream->currBuffer = bytes_read;
	}
	/* Daca am ajuns la final de buffer, citim din nou */
	if (stream->info.currPos == stream->currBuffer) {
		bytes_read = read(stream->fd, stream->buffer, BUFSIZE);
		/* Daca am terminat de parcurs buffer-ul */
		stream->info.currPos = 0;
		/* Actualizam pozitia in fisier */
		stream->info.ftellBytes += bytes_read;

		/* Verificam daca am ajuns la final de fisier */
		if (bytes_read == 0) {
			stream->info.flagEof = 1;
			return SO_EOF;
		}
		/* Cati octeti am citit din fisier in buffer */
		if (bytes_read > 0)
			stream->currBuffer = bytes_read;
	}
	/* Functia read nu a reusit sa citeasca din fisier */
	if (bytes_read < 0) {
		stream->info.flagError = 1;
		return SO_EOF;
	}
	ret = (int)stream->buffer[stream->info.currPos];
	stream->info.currPos++;
	return ret;
}

int so_fputc(int c, SO_FILE *stream)
{
	int ret_flush;

	stream->info.opWrite = 1;
	stream->info.opRead = 0;

	if (stream == NULL) {
		stream->info.flagError = 1;
		return SO_EOF;
	}
	/* Daca am parcurs tot buffer-ul, facem flush */
	if (stream->info.currPos == BUFSIZE) {
		/* Actualizam pozitia in fisier */
		stream->info.ftellBytes += BUFSIZE;
		ret_flush = so_fflush(stream);
		/* Verificam daca s-a facut flush corect */
		if (ret_flush == SO_EOF) {
			stream->info.flagError = 1;
			return SO_EOF;
		}
	}
	stream->buffer[stream->info.currPos] = c;
	stream->info.currPos++;

	return c;
}

size_t so_fread(void *ptr, size_t size, size_t nmemb, SO_FILE *stream)
{
	int count = nmemb * size;
	int bytes_read = 0;
	int bytes_read_now = 0;
	int iterator = 1;
	char *adrPtr = (char *) ptr;

	if (stream == NULL) {
		stream->info.flagError = 1;
		return 0;
	}
	while (bytes_read < count) {
		bytes_read_now = so_fgetc(stream);
		/* Daca nu s-a ajuns la sfarsit de fisier, continuam citirea */
		if (so_feof(stream) == 0) {
			*adrPtr = bytes_read_now;
			adrPtr += iterator;
		} else
			return bytes_read / size;

		bytes_read += iterator;
	}
	if (so_ferror(stream) != 0)
		return 0;

	return bytes_read / size;
}

size_t so_fwrite(const void *ptr, size_t size, size_t nmemb, SO_FILE *stream)
{
	int bytes_written = 0;
	int bytes_written_now = 0;
	int count = nmemb * size;
	int iterator = 1;
	char *adrPtr = (char *) ptr;

	if (stream == NULL) {
		stream->info.flagError = 1;
		return 0;
	}
	while (bytes_written < count) {
		bytes_written_now = so_fputc(*adrPtr, stream);

		/* Verificam daca so_fputc intoarce eroare */
		if (bytes_written_now == SO_EOF && *adrPtr != SO_EOF) {
			stream->info.flagError = 1;
			return 0;
		}
		adrPtr += iterator;
		bytes_written += iterator;
	}

	return bytes_written / size;
}


SO_FILE *so_fopen(const char *pathname, const char *mode)
{
	int fd = -1;
	/* Deschidem fisierul dat ca parametru */
	fd = open(pathname, choose_mode(mode), RIGHTS);
	/* In caz de eroare, intoarcem NULL */
	if (fd < 0)
		return NULL;

	SO_FILE *stream = allocate_memory(pathname, mode);

	initialize_file(stream);
	stream->fd = fd;

	return stream;
}

int so_fclose(SO_FILE *stream)
{
	int close_ret, flush_ret;

	/* Verificam daca ultima operatie este de scriere */
	if (stream->info.opWrite == 1) {
		flush_ret = so_fflush(stream);
		/* Nu s-a facut fflush corect */
		if (flush_ret == SO_EOF) {
			free_memory(stream);
			return SO_EOF;
		}
	}
	close_ret = close(stream->fd);
	free_memory(stream);

	if (close_ret == -1)
		return SO_EOF;

	return 0;
}

int so_fflush(SO_FILE *stream)
{
	int bytesWritten = 0;
	int bytesWritten_now = 0;
	int count = 0;

	count = stream->info.currPos;

	if (stream == NULL) {
		stream->info.flagError = 1;
		return SO_EOF;
	}
	/* Daca buffer-ul este gol, nu facem nimic */
	if (stream->info.currPos == 0)
		return 0;
	/* Daca ultima operatie a fost de scriere, scriem in fisier */
	if (stream->info.opWrite == 1) {
		while (bytesWritten < stream->info.currPos) {
			bytesWritten_now = write(stream->fd,
				stream->buffer + bytesWritten,
				count);
			if (bytesWritten_now == -1) {
				stream->info.flagError = 1;
				return SO_EOF;
			}
			bytesWritten += bytesWritten_now;
			count -= bytesWritten_now;
		}

		if (bytesWritten != -1) {
			memset(stream->buffer, 0, BUFSIZE);
			stream->info.currPos = 0;
			stream->currBuffer = 0;
		}

		return 0;
	}
	stream->info.flagError = 1;

	return SO_EOF;
}

int so_fileno(SO_FILE *stream)
{
	return stream->fd;
}

int so_feof(SO_FILE *stream)
{
	if (stream->info.flagEof == 1)
		return SO_EOF;

	return 0;
}

int so_ferror(SO_FILE *stream)
{
	if (stream->info.flagError == 1)
		return SO_EOF;

	return 0;
}

int so_fseek(SO_FILE *stream, long offset, int whence)
{
	int ret_fflush = 0;
	int ret_seek = 0;

	if (stream == NULL) {
		stream->info.flagError = 1;
		return SO_EOF;
	}
	/* Daca ultima operatie a fost de citire, invalidam buffer-ul */
	if (stream->info.opRead == 1) {
		memset(stream->buffer, 0, BUFSIZE);
		stream->info.currPos = 0;
		stream->currBuffer = 0;
	}
	/* Daca ultima operatie a fost de scriere, facem fflush */
	if (stream->info.opWrite == 1) {
		ret_fflush = so_fflush(stream);

		if (ret_fflush != 0) {
			stream->info.flagError = 1;
			return SO_EOF;
		}
	}
	/* Intoarce numarul de bytes de la inceput pana la cursor */
	ret_seek = lseek(stream->fd, offset, whence);

	if (ret_seek < 0) {
		stream->info.flagError = 1;
		return SO_EOF;
	}
	/* Actualizam pozitia cursorului in structura */
	stream->info.ftellBytes = ret_seek;

	return 0;
}

long so_ftell(SO_FILE *stream)
{
	if (stream == NULL) {
		stream->info.flagError = 1;
		return SO_EOF;
	}
	if (stream->info.ftellBytes == 0)
		return stream->info.currPos;
	else
		return stream->info.ftellBytes + stream->info.currPos;
}

SO_FILE *so_popen(const char *command, const char *type)
{
	int pfd[2];
	int fdescriptor = -1;
	int pid = -1;
	int ret_pipe = -1;
	char *pathname = NULL;
	char *mode = NULL;
	char *command_cpy = NULL;

	command_cpy = malloc(strlen(command) + 1);

	if (command_cpy == NULL)
		return NULL;

	strcpy(command_cpy, command);

	char *arg_exe[NO_ARG] = {"sh", "-c", command_cpy, NULL};

	if ((*type != 'r' && *type != 'w') || type[1] != '\0')
		return NULL;
	/* Alocam memorie pentru structura SO_FILE */
	SO_FILE *stream = allocate_memory(pathname, mode);

	ret_pipe = pipe(pfd);

	if (ret_pipe < 0) {
		free_memory(stream);
		free(command_cpy);
		return NULL;
	}

	pid = fork();
	/* fork intoarceeroare */
	if (pid == -1) {
		close(pfd[0]);
		close(pfd[1]);
		free_memory(stream);
		free(command_cpy);
		return NULL;
	}
	/* Procesul copil */
	if (pid == 0) {
		if (*type == 'r') {
			close(pfd[0]);
			if (pfd[1] != STDOUT_FILENO) {
				dup2(pfd[1], STDOUT_FILENO);
				close(pfd[1]);
			}
		}

		if (*type == 'w') {
			close(pfd[1]);
			if (pfd[0] != STDIN_FILENO) {
				dup2(pfd[0], STDIN_FILENO);
				close(pfd[0]);
			}
		}

		execv(PATH_SHELL, arg_exe);
		return NULL;
	}
	/* Parintele - procesul parinte */
	if (*type == 'r') {
		fdescriptor = pfd[0];
		close(pfd[1]);
	}

	if (*type == 'w') {
		fdescriptor = pfd[1];
		close(pfd[0]);
	}
	/* Initializam file descriptorul din structrura */
	initialize_file(stream);
	stream->fd = fdescriptor;
	stream->info.pidFile = pid;
	free(command_cpy);
	return stream;
}

int so_pclose(SO_FILE *stream)
{
	int pid = -1;
	int wait_pid = -1;
	int pstat = -1;
	/* Daca deja s-a facut 'so_pclosed' sau waitpid return error */
	wait_pid = stream->info.pidFile;
	so_fclose(stream);

	pid = waitpid(wait_pid, &pstat, 0);

	return (pid == -1 ? -1 : pstat);
}
