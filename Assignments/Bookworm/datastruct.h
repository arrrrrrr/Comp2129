#ifndef DATASTRUCT_H
#define DATASTRUCT_H

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/types.h>
#include <pthread.h>
#include <string.h>
#include <math.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <unistd.h>

#define MALLOC_TEST(ptr) if ((ptr) == NULL) {          \
                            perror("Malloc Failed\n"); \
                            exit(1);                   \
                         }

#define CACHE_LINE_SIZE 64

// singly linked list item def
typedef struct slist_cqitem_t slist_cqitem_t;

struct slist_cqitem_t {
    size_t item;
    size_t aba;
    slist_cqitem_t *next;
};

// queue def
typedef struct {
    slist_cqitem_t *data;
    slist_cqitem_t *head;
    slist_cqitem_t *tail;
    // atomic because it might be shared
    atomic_size_t size;
} cqueue_t;

// singly linked list item def
typedef struct slist_qitem_t slist_qitem_t;

struct slist_qitem_t {
    size_t item;
    slist_qitem_t *next;
};

// singly linked list item def
typedef struct slist_item_t slist_item_t;

struct slist_item_t {
    void *item;
    slist_item_t *next;
};

// queue def
typedef struct {
    slist_qitem_t *head;
    slist_qitem_t *tail;
    // atomic because it might be shared
    atomic_int size;
} queue_t;

// concept of concurrent queue based on
// http://www.drdobbs.com/parallel/writing-a-generalized-concurrent-queue/211601363?pgno=1
typedef struct slist_node_t slist_node_t;

struct slist_node_t {
    void *item;
    // next pointer
    slist_node_t *next;
    char pad[CACHE_LINE_SIZE-sizeof(void *)
             -sizeof(slist_node_t *)];
};

typedef struct {
    char pad0[CACHE_LINE_SIZE];
    // head
    slist_node_t *head;
    char pad1[CACHE_LINE_SIZE-sizeof(slist_node_t *)];
    // dequeueing lock (consumer)
    atomic_bool dequeue_lock;
    char pad2[CACHE_LINE_SIZE-sizeof(atomic_bool)];
    // tail
    slist_node_t *tail;
    char pad3[CACHE_LINE_SIZE-sizeof(slist_node_t *)];
    // enqueueing lock (producer)
    atomic_bool enqueue_lock;
    char pad4[CACHE_LINE_SIZE-sizeof(atomic_bool)];
    // size
    atomic_int size;
    char pad5[CACHE_LINE_SIZE-sizeof(atomic_int)];
} aqueue_t;

// stack def
typedef struct {
    slist_item_t *top;
    size_t size;
} stack_t;

queue_t *create_queue(void);
void     destroy_queue(queue_t *q);
void     enqueue_item(queue_t *q, size_t item);
void    *dequeue_item(queue_t *q, size_t *item);

aqueue_t *create_aqueue();
void      destroy_aqueue(aqueue_t *q);
int       aqueue_size(aqueue_t *q);
void      aqueue_enqueue(aqueue_t *q, void *item, size_t size);
void     *aqueue_dequeue(aqueue_t *q);

stack_t *create_stack(void);
void     destroy_stack(stack_t *s);
void     push_item(stack_t *s, void *item, size_t size);
void    *pop_item(stack_t *s, void *item, size_t size);
void    *peek_item(stack_t *s, void *item, size_t size);

void cqueue_enqueue(cqueue_t *q, size_t item);
void *cqueue_dequeue(cqueue_t *q, size_t *item);
cqueue_t *create_cqueue(int size);
void destroy_cqueue(cqueue_t *q);
size_t cqueue_size(cqueue_t *q);

#endif