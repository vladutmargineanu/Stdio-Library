CC=gcc
CFLAGS=-Wall -g -fPIC
EXE=libso_stdio.so

build: $(EXE)

$(EXE): so_stdio.o so_utils.o
	$(CC) -shared so_stdio.o so_utils.o -o $(EXE)

so_stdio.o: so_stdio.c so_utils.h
	$(CC) $(CFLAGS) -c so_stdio.c  -o so_stdio.o 

so_utils.o: so_utils.c so_utils.h
	$(CC) $(CFLAGS) -c so_utils.c -o so_utils.o

clean:
	rm -rf so_stdio.o so_utils.o libso_stdio.so
