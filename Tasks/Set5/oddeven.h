#ifndef ODDEVEN_H
#define ODDEVEN_H
#include <stdatomic.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define THR_COUNT 2

void* thread_even(void* arg);

void* thread_odd(void* arg);

typedef struct thrdata_t {
    int tid;
    int value;
    atomic_int *actr;
} thrdata_t;

#endif