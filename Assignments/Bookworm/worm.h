#ifndef WORM_H
#define WORM_H

#include <stdint.h>
#include <stdatomic.h>
#include <math.h>
#include "datastruct.h"
#include "hashmap.h"

extern size_t g_nthreads;

typedef struct book_t {
    size_t id;
    size_t author_id;
    size_t publisher_id;
    size_t* b_author_edges;
    size_t* b_citation_edges;
    size_t* b_publisher_edges;
    size_t n_author_edges;
    size_t n_citation_edges;
    size_t n_publisher_edges;
} book_t;

typedef struct result_t {
    book_t **elements;
    size_t   n_elements;
} result_t;

typedef struct thr_findbook_t {
    int         tid;
    size_t      param1;
    size_t      param2;
    book_t     *nodes;
    size_t      count;
    uint16_t    n_threads;
    book_t    **nodes_found;
    size_t      nodes_found_count;
    size_t     *a_index;
    int         a_index_count;
    size_t     *b_index;
    int         b_index_count;
    size_t     *idlist;
    size_t      idlist_count;
    aqueue_t    *q;
    atomic_int *actr;
    atomic_int *actr2;
    atomic_int *visitctr;
    _Atomic ssize_t *a_visited;
    map_t      *sharedmap;
} thr_findbook_t;

typedef struct thr_kdist_t {
    int         tid;
    size_t      k;
    book_t     *nodes;
    size_t      count;
    size_t      nthreads;
    aqueue_t   *sharedq;
    int         sharedq_count;
    map_t      *vshared;
    size_t *vfound;
} thr_kdist_t;

typedef struct bookid_cache_t {
    book_t *ref;
    size_t count;
    size_t checksum;
    map_t  *map;
} bookid_cache_t;

result_t* find_book(book_t* nodes, size_t count, size_t book_id);
result_t* find_books_by_author(book_t* nodes, size_t count, size_t author_id);
result_t* find_books_reprinted(book_t* nodes, size_t count, size_t publisher_id);
result_t* find_books_k_distance(book_t* nodes, size_t count, size_t book_id, uint16_t k);
result_t* find_shortest_distance(book_t* nodes, size_t count, size_t b1_id, size_t b2_id);
result_t* find_shortest_edge_type(book_t* nodes, size_t count, size_t a1_id, size_t a2_id);
result_t* find_books_k_distance_st(book_t *nodes, size_t count, size_t book_id, uint16_t k);

result_t* find_books_k_distance_mt2(book_t *nodes, size_t count, size_t book_id, uint16_t k);
void *find_books_k_distance_mt2_worker(void *arg);
// helpers
void copy_book_node(book_t *dst, book_t *src);
int partition_array(int count, int id, int n_threads, int *start, int *end);

void build_result_data(book_t **nodes, size_t node_count, result_t *result, int *result_from);

size_t *bfs_shortest_path(book_t *graph, int count, size_t *a_index, int a_index_count,
                          size_t *b_index, int b_index_count, int *walk_len, int threads);

book_t **bfs_distance(book_t *graph, int count, size_t a_index, size_t k, int *found_len);
void print_book_node(book_t *graph, book_t *bn);
// worker functions
void *find_book_worker(void *arg);
void *find_books_by_author_worker(void *arg);
void *find_books_k_distance_st_worker(void *arg);
void *find_shortest_distance_worker(void *arg);
result_t *find_book_cached(book_t *nodes, size_t count, size_t id);
result_t* find_book_uncached(book_t* nodes, size_t count, size_t book_id);

void destroy_bookid_cache() __attribute__((destructor));
void create_bookid_cache(book_t *nodes, size_t count);

#endif