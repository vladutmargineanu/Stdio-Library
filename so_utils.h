#ifndef SO_UTILS_H
#define SO_UTILS_H

#include <stdlib.h>

#define BUFSIZE 4096
#define RIGHTS  0644
#define PATH_SHELL "/bin/sh"
#define NO_ARG 4

typedef struct _info_file {
	int flagError;      /* eroare */
	int flagEof;		/* final de fisier */
	int opRead;         /* ultima operatie pe stream de citire */
	int opWrite;        /* ultima operatie pe stream de scriere */
	int pidFile;        /* id-ul procesului pentru pipe */
	int currPos;        /* ietrator prin octetii cititi */
	int ftellBytes;		/* retinem pozitia cursorului */
} INFO_FILE;

typedef struct _so_file {
	int fd;             /* file descriptor */
	char *pathname;     /* calea catre fisier */
	char *mode;         /* modul de acces */
	char *buffer;       /* buffer-ul asociat*/
	int offset;         /* numar bytes fata de pozitie */
	int currBuffer;     /* cati octeti s-au citit in buffer */
	INFO_FILE info;     /* info fisier */
} SO_FILE;


int choose_mode(const char *mode);
SO_FILE *allocate_memory(const char *pathname, const char *mode);
void free_memory(SO_FILE *stream);
void initialize_file(SO_FILE *stream);

#endif /* SO_UTILS_H */
