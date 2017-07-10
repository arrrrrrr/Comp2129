#ifndef HASHMAP_H
#define HASHMAP_H

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <pthread.h>
#include <string.h>
#include <math.h>
#include <x86intrin.h>
#include <stdatomic.h>

// hashmap sentinel
#define MAP_DEFUNCT ((void *)1)
#define MAP_ENTRY_VALUE_EMPTY ((size_t)-1)

// hashmap load factor
#define MAP_LOAD_FACTOR 0.5

#define MALLOC_TEST(ptr) if ((ptr) == NULL) {          \
                            perror("Malloc Failed\n"); \
                            exit(1);                   \
                         }
// hashmap entry def
typedef struct {
    size_t  key;
    size_t  value;
    int     has_value;
} map_entry_t;

// hash function data
typedef struct map_hash_t {
    uint32_t a;
    uint32_t b;
    uint32_t p;
} map_hash_t;

typedef struct {
    map_entry_t **entries;
    map_hash_t hash;

    atomic_size_t size; // size of current entries
    size_t limit; // max number of entries in the hashmap

    pthread_mutex_t lock; // mutex
} map_t;

map_t   *create_hashmap(int limit);
void     destroy_hashmap(map_t *map);
void    *hashmap_get(map_t *map, size_t key, size_t *out_value);
int      hashmap_exists(map_t *map, size_t key);

int      hashmap_put(map_t *map, size_t key, size_t *value);
void     hashmap_remove(map_t *map, size_t key);

// synchronized map functions
int      hashmap_put_l(map_t *map, size_t key, size_t *value);
void    *hashmap_get_l(map_t *map, size_t key, size_t *out_value);
int      hashmap_exists_l(map_t *map, size_t key);
void     hashmap_remove_l(map_t *map, size_t key);

size_t  *hashmap_keyset(map_t *map, int *out_length);
void     destroy_hashmap_keyset(size_t *keyset);
int      hashmap_hash(map_t *map, size_t key);
uint32_t hashmap_hash_key(size_t key);
void     hashmap_resize(map_t *map);

// helpers
int next_prime(int num);
int is_prime(int num);
void get_rand_pair(uint32_t *a, uint32_t *b, int32_t rmax);

#endif