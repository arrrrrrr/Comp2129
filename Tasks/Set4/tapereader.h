#ifndef TAPEREADER_H
#define TAPEREADER_H

#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <pthread.h>

typedef struct {
    int fd;
    size_t size;
    char *map;
} fmap_t;

typedef struct {
    int id;
    int length;
    char *readptr;
    char filename[16];
} thr_data_t;

char *get_read_ptr(int offset);
char *read_from_read_ptr(char **read_ptr, int length, int id);
void* tape_reader(void* args);

#endif