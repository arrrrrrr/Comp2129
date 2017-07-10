#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>
#include <unistd.h>
#include "worm.h"
#include "datastruct.h"
#include "hashmap.h"

size_t g_nthreads = 4;
bookid_cache_t g_bookid_cache = {0};

void destroy_bookid_cache() {
    if (g_bookid_cache.map != NULL) {
        destroy_hashmap(g_bookid_cache.map);
    }
}

void create_bookid_cache(book_t *nodes, size_t count) {
    if (g_bookid_cache.map != NULL) {
        size_t checksum = (nodes[0].id >> 16) ^ (nodes[count-1].id << 16);
        if (g_bookid_cache.ref == nodes && g_bookid_cache.count == count
            && g_bookid_cache.checksum == checksum)
            return;

        destroy_hashmap(g_bookid_cache.map);
        g_bookid_cache.map = NULL;
    }

    g_bookid_cache.map = create_hashmap(count * 2);

    for (int i = 0; i < count; i++) {
        size_t bookid = nodes[i].id;
        size_t idx = i;
        hashmap_put(g_bookid_cache.map, bookid, &idx);
    }

    g_bookid_cache.count = count;
    g_bookid_cache.ref = nodes;
    g_bookid_cache.checksum = (nodes[0].id >> 16) ^ (nodes[count-1].id << 16);
}

void cqueue_enqueue(cqueue_t *q, size_t item) {
    slist_cqitem_t *head = q->head;
    head->item = item;
    q->head = head->next;
    q->size++;
}

void *cqueue_dequeue(cqueue_t *q, size_t *item) {
    if (q->size == 0)
        return NULL;

    slist_cqitem_t *tail = q->tail;
    q->tail = tail->next;
    q->size--;

    *item = tail->item;
    return item;
}

inline
size_t cqueue_size(cqueue_t *q) {
    return q->size;
}

cqueue_t *create_cqueue(int size) {
    cqueue_t *q = malloc(sizeof(*q));
    MALLOC_TEST(q);

    q->data = malloc(sizeof(*q->data) * size);

    for (int i = 0; i < size-1; i++) {
        q->data[i].item = 0L;
        q->data[i].aba = i;
        q->data[i].next = q->data + i + 1;
    }

    q->data[size-1].item = 0L;
    q->data[size-1].aba = size-1;
    q->data[size-1].next = q->data;

    q->head = q->tail = q->data;
    q->size = 0;

    return q;
}

void destroy_cqueue(cqueue_t *q) {
    if (q == NULL)
        return;

    free(q->data);
    free(q);
}

// hash the key as a crc32 value ?? is this a good idea??
uint32_t hashmap_hash_key(size_t key) {
    uint64_t hash = 0;
    hash = _mm_crc32_u64(hash, key);
    return (uint32_t)hash;
}

// hashmap hash/compression function
int hashmap_hash(map_t *map, size_t key) {
    /*  ((ai + b) % p) % n
     *  a = random value between 1 and p-1
     *  b = random value between 0 and p-1
     *  i = integer to map
     *  p = prime number > n
     *  n = length of array in hashmap
     */
    // get i via the hash function
    uint32_t i = hashmap_hash_key(key);
    return (((map->hash.a * i) + map->hash.b) % map->hash.p) % map->limit;
}

// create a hashmap of at least size entries
map_t *create_hashmap(int limit) {
    if (limit == 0)
        limit = 2;

    map_t *map = malloc(sizeof(*map));
    MALLOC_TEST(map);
    memset(map, 0, sizeof(*map));

    map->size = 0;
    // take the requested size and find the next prime number
    map->limit = next_prime(limit);

    map->entries = malloc(sizeof(map_entry_t *) * map->limit);
    MALLOC_TEST(map->entries);
    memset(map->entries, 0, sizeof(map_entry_t *) * map->limit);

    uint32_t a,b,p;
    // unprime the entries length and find the next prime after that
    p = next_prime((map->limit * 2)+1);
    // get two random numbers a and b
    get_rand_pair(&a, &b, p);

    map->hash.a = a;
    map->hash.b = b;
    map->hash.p = p;
    // init the mutex
    pthread_mutex_init(&(map->lock),NULL);

    return map;
}

// destroy the hashmap
void destroy_hashmap(map_t *map) {
    if (map == NULL)
        return;

    for (int i = 0; i < map->limit; i++) {
        if (map->entries[i] != NULL && map->entries[i] != MAP_DEFUNCT) {
            free(map->entries[i]);
        }
    }

    free(map->entries);
    pthread_mutex_destroy(&(map->lock));
    free(map);
}

// return a keyset of the hashmap
size_t *hashmap_keyset(map_t *map, int *out_length) {
    *out_length = 0;

    if (map == NULL || map->size == 0)
        return NULL;

    size_t *keyset = malloc(sizeof(*keyset) * map->size);
    MALLOC_TEST(keyset);
    int idx = 0;

    for (int i = 0; i < map->limit; i++) {
        // its a valid map entry
        if (map->entries[i] != NULL && map->entries[i] != MAP_DEFUNCT) {
            keyset[idx] = map->entries[i]->key;
            idx++;
        }
    }

    *out_length = map->size;

    return keyset;
}

// destroy a keyset created by hashmap_keyset
void destroy_hashmap_keyset(size_t *keyset) {
    if (keyset == NULL)
        return;

    free(keyset);
}

/* Get a value from a key in the hashmap
 *
 * Error or not found:
 *   returns NULL and sets the callers size to -1
 *
 * Success:
 *   returns NULL and sets the callers size to 0 if the key has no value
 *   returns a pointer to a value and sets the callers size to its size
 */
void *hashmap_get(map_t *map, size_t key, size_t *out_value) {
    if (map == NULL || map->size == 0)
        return NULL;

    // set it to a sentinel value
    *out_value = MAP_ENTRY_VALUE_EMPTY;

    // calculate the hash for the key
    int h = hashmap_hash(map, key);
    int scanned = 0, found = -1, limit = map->limit, idx = h;

    // scan until we reach entries[h-1]
    while (scanned < limit) {
        // reached an empty index cant be past here
        if (map->entries[idx] == NULL)
            break;
        // if not defunct and key sizes match
        if (map->entries[idx] != MAP_DEFUNCT && map->entries[idx]->key == key) {
            // compare and set found if a match then end the loop
            found = idx;
            break;
        }
        if (++idx >= limit)
            idx = 0;

        scanned++;
    }

    // not found in the hashmap
    if (found == -1)
        return NULL;
    // key has no value but is valid
    if (map->entries[found]->has_value == 0)
        return out_value;

    *out_value = map->entries[found]->value;

    return out_value;
}

void *hashmap_get_l(map_t *map, size_t key, size_t *out_value) {
    if (map == NULL || map->size == 0)
        return NULL;

    // set it to a sentinel value
    *out_value = MAP_ENTRY_VALUE_EMPTY;

    // calculate the hash for the key
    int h = hashmap_hash(map, key);
    int scanned = 0, found = 0, limit = map->limit, idx = h;

    map_entry_t entry = {0};
    pthread_mutex_lock(&(map->lock));

    // scan until we reach entries[h-1]
    while (scanned < limit) {
        // reached an empty index cant be past here
        if (map->entries[idx] == NULL)
            break;
        // if not defunct and key sizes match
        if (map->entries[idx] != MAP_DEFUNCT && map->entries[idx]->key == key) {
            // compare and set found if a match then end the loop
            memcpy(&entry, map->entries[idx], sizeof(map_entry_t));
            found++;
            break;
        }
        if (++idx >= limit)
            idx = 0;

        scanned++;
    }

    pthread_mutex_unlock(&(map->lock));

    // not found in the hashmap
    if (!found)
        return NULL;
    // key has no value but is valid
    if (entry.has_value == 0)
        return out_value;

    *out_value = entry.value;

    return out_value;
}

// does a key exist in the hashmap
int hashmap_exists(map_t *map, size_t key) {
    if (map == NULL || map->size == 0)
        return 0;
    // calculate the hash for the key
    int h = hashmap_hash(map, key);
    int scanned = 0, found = -1, limit = map->limit, idx = h;
    // scan until we reach entries[h-1]
    while (scanned < limit) {
        // reached an empty index cant be past here
        if (map->entries[idx] == NULL)
            break;
        // if not defunct and key sizes match
        if (map->entries[idx] != MAP_DEFUNCT && map->entries[idx]->key == key) {
            // compare and set found if a match then end the loop
            found = idx;
            break;
        }

        if (++idx >= limit)
            idx = 0;

        scanned++;
    }
    // not found in the hashmap
    if (found == -1)
        return 0;

    return 1;
}

// does a key exist in the hashmap
int hashmap_exists_l(map_t *map, size_t key) {
    if (map == NULL || map->size == 0)
        return 0;
    // calculate the hash for the key
    int h = hashmap_hash(map, key);
    int scanned = 0, found = 0, limit = map->limit, idx = h;
    map_entry_t entry = {0};

    pthread_mutex_lock(&(map->lock));
    // scan until we reach entries[h-1]
    while (scanned < limit) {
        // reached an empty index cant be past here
        if (map->entries[idx] == NULL)
            break;
        // if not defunct and key sizes match
        if (map->entries[idx] != MAP_DEFUNCT && map->entries[idx]->key == key) {
            // compare and set found if a match then end the loop
            memcpy(&entry, map->entries[idx], sizeof(map_entry_t));
            found++;
            break;
        }

        if (++idx >= limit)
            idx = 0;

        scanned++;
    }

    pthread_mutex_unlock(&(map->lock));

    return found;
}

// synchronized put
int hashmap_put_l(map_t *map, size_t key, size_t *value) {
    pthread_mutex_lock(&(map->lock));
    int result = hashmap_put(map, key, value);
    pthread_mutex_unlock(&(map->lock));
    return result;
}

/* Put/update a key/value pair in the hashmap
 *
 * Error:
 *   returns 0
 *
 * Success:
 *   returns 1
 */
int hashmap_put(map_t *map, size_t key, size_t *value) {
    // bad parameters
    if (map == NULL)
        return 0;

    // resize if we've reached the load factor
    if (((double)map->size / (double)map->limit) >= MAP_LOAD_FACTOR)
        hashmap_resize(map);

    // calculate the hash for the key
    int h = hashmap_hash(map, key);
    int scanned = 0, idx = h, limit = map->limit;

    // scan until we reach entries[h-1]
    while (scanned < limit) {
        // reached an empty or defunct index, lets place out entry here
        if (map->entries[idx] == NULL || map->entries[idx] == MAP_DEFUNCT) {
            map_entry_t *entry = malloc(sizeof(*entry));
            memset(entry, 0, sizeof(*entry));
            // store the key
            entry->key = key;
            // setup for no value
            entry->has_value = 0;
            entry->value = MAP_ENTRY_VALUE_EMPTY;
            // store the value if its size > 0
            if (value != NULL) {
                entry->value = *value;
                entry->has_value = 1;
            }

            map->entries[idx] = entry;
            // increment the current size counter
            atomic_fetch_add(&map->size, 1);
            //map->size++;
            break;
        }

        // compare any key of the same size and if matching update it
        if (map->entries[idx]->key == key) {
            map_entry_t *entry = map->entries[idx];

            // free the value first
            if (entry->has_value) {
                entry->value = MAP_ENTRY_VALUE_EMPTY;
                entry->has_value = 0;
            }

            // store the new value if its size > 0
            if (value != NULL) {
                entry->value = *value;
                entry->has_value = 1;
            }

            break;
        }

        if (++idx >= limit)
            idx = 0;

        scanned++;
    }

    return 1;
}

// remove an entry from the hashmap
void hashmap_remove(map_t *map, size_t key) {
    if (map == NULL || map->size == 0)
        return;

    // calculate the hash for the key
    int h = hashmap_hash(map, key);
    int scanned = 0, limit = map->limit, idx = h;

    // scan until we reach entries[h-1]
    while (scanned < map->limit) {
        // reached an empty index cant be past here
        if (map->entries[idx] == NULL)
            break;
        // if not defunct and key sizes match
        if (map->entries[idx] != MAP_DEFUNCT &&
            map->entries[idx]->key == key) {
            // free the entry
            free(map->entries[idx]);
            // set the entry to defunct
            map->entries[idx] = MAP_DEFUNCT;
            // fix the size up
            atomic_fetch_sub(&map->size, 1);
            //map->size--;
            break;
        }

        if (++idx >= limit)
            idx = 0;

        scanned++;
    }

    return;
}

// synchronized remove
void hashmap_remove_l(map_t *map, size_t key) {
    pthread_mutex_lock(&(map->lock));
    hashmap_remove(map, key);
    pthread_mutex_unlock(&(map->lock));
}

// internal helper to resize a hashmap to the next prime twice its current limit
void hashmap_resize(map_t *map) {
    // double the size of the hashmap
    int newlimit = next_prime(map->limit*2);
    int oldlimit = map->limit;

    // make a copy of the old entries
    map_entry_t **old_entries = malloc(sizeof(map_entry_t *) * oldlimit);
    memcpy(old_entries, map->entries, sizeof(map_entry_t *) * oldlimit);

    // free the old entries
    free(map->entries);
    // allocate new entries
    map->entries = malloc(sizeof(map_entry_t *) * newlimit);
    memset(map->entries, 0, sizeof(map_entry_t *) * newlimit);

    uint32_t a,b,p;
    // double the size
    p = next_prime((newlimit * 2) + 1);
    // get two random numbers a and b
    get_rand_pair(&a, &b, p);

    // fixup the internal hash struct
    map->hash.a = a;
    map->hash.b = b;
    map->hash.p = p;

    // fixup the size and limit
    map->size = 0;
    map->limit = newlimit;

    // add all the old entries back into the hashmap
    for (int i = 0; i < oldlimit; i++) {
        // valid entry
        if (old_entries[i] != NULL && old_entries[i] != MAP_DEFUNCT) {
            // no value we just pass null
            size_t *old_value = (old_entries[i]->has_value) ? &(old_entries[i]->value) : NULL;
            // put the old entry back into the hashmap
            hashmap_put(map, old_entries[i]->key, old_value);
            free(old_entries[i]);
            old_entries[i] = NULL;
        }
    }

    free(old_entries);
}

// helpers for hashmap
// thanks stackoverflow
int next_prime(int num) {
    int c;

    if(num < 2)
        c = 2;
    else if (num == 2)
        c = 3;
    else {
        if (!(num & 1))
            num++;
        for (c = num; !is_prime(num); num += 2);
    }

    return c;
}

int is_prime(int num) {
    if ((num & 1) == 0)
        return num == 2;
    else {
        int i, limit = sqrt(num);

        for (i = 3; i <= limit; i+=2){
            if (num % i == 0)
                return 0;
        }
    }

    return 1;
}

// get a pair of random 32-bit ints using rdrand
void get_rand_pair(uint32_t *a, uint32_t *b, int32_t rmax) {
    uint32_t r1 = 0, r2 = 0;
    // setup the seed
    srand(time(NULL));

    // generate two random numbers using the hw random generator
    //make sure that r1 is not 0
    while (r1 == 0)
        r1 = rand() % rmax;

    r2 = rand() % rmax;

    // limit the value between 0 and rmax
    *a = r1;
    *b = r2;
}

/*
 * queue data structure built using a singly linked list
 * create_queue - creates a queue
 * destroy_queue - cleans it up
 * enqueue_item - append an item to the end of the queue
 * dequeue_item - remove an item from the start of the queue
 */
void enqueue_item(queue_t *q, size_t item) {
    slist_qitem_t *li = malloc(sizeof(*li));
    li->item = item;
    li->next = NULL;

    // append it to the end of the queue
    if (atomic_load(&(q->size)) == 0)
        q->head = li;
    else
        q->tail->next = li;

    q->tail = li;
    // atomically update the size
    atomic_fetch_add(&(q->size), 1);
}

void *dequeue_item(queue_t *q, size_t *item) {
    if (atomic_load(&(q->size)) == 0)
        return NULL;

    // atomically update the size
    atomic_fetch_sub(&(q->size), 1);

    slist_qitem_t *li = q->head;
    q->head = li->next;
    *item = li->item;
    free(li);

    // queue is now empty
    if (atomic_load(&(q->size)) == 0)
        q->head = q->tail = NULL;

    return item;
}

queue_t *create_queue() {
    queue_t *q = malloc(sizeof(*q));
    MALLOC_TEST(q);

    q->head = q->tail = NULL;
    q->size = 0;

    return q;
}

void destroy_queue(queue_t *q) {
    if (q == NULL)
        return;

    slist_qitem_t *curr = q->head;

    while (curr != NULL) {
        slist_qitem_t *tmp = curr->next;
        free(curr);
        curr = tmp;
    }

    free(q);
}

// mostly lock-free queue!
// done my best to deal with preemption
aqueue_t *create_aqueue() {
    aqueue_t *q = malloc(sizeof(*q));
    MALLOC_TEST(q);

    // dummy node
    slist_node_t *li = malloc(sizeof(*li));
    li->item = NULL;
    li->next = NULL;
    q->head = q->tail = li;
    atomic_store(&q->size, 0);
    q->enqueue_lock = false;
    q->dequeue_lock = false;

    return q;
}

void destroy_aqueue(aqueue_t *q) {
    if (q == NULL)
        return;

    for (slist_node_t *curr = q->head, *tmp; curr; curr = tmp) {
        tmp = curr->next;
        if (curr->item != NULL)
            free(curr->item);
        free(curr);
    }

    free(q);
}

int aqueue_size(aqueue_t *q) {
    return atomic_load(&q->size);
}

void aqueue_enqueue(aqueue_t *q, void *item, size_t size) {
    bool expected = false;

    slist_node_t *li = malloc(sizeof(*li));
    li->item = malloc(size);
    // copy the item
    memcpy(li->item, item, size);
    li->next = NULL;
    // acquire lock
    while (!atomic_compare_exchange_weak(&q->enqueue_lock, &expected, true))
    {
        //usleep(5); // sleep a little so we dont hammer the queue
    }

    // head is null, set the new item to the head
    __sync_swap(&q->tail->next, li);
    // atomically set the tail to the new item
    __sync_swap(&q->tail,li);
    // atomically update the size
    // release the lock
    atomic_fetch_add(&(q->size), 1);
    atomic_store(&q->enqueue_lock, expected);
    //printf("q->head: %p, q->tail: %p\n",q->head,q->tail);
}

void *aqueue_dequeue(aqueue_t *q) {
    bool expected = false;
    slist_node_t *li = NULL;
    //int last_sleep = 10;
    // acquire lock
    while (!atomic_compare_exchange_weak(&q->dequeue_lock, &expected, true))
    {
        //usleep(5); // sleep a little so we dont hammer the queue
    }
    // nothing on the queue
    if (atomic_load(&(q->size)) == 0) {
        //printf("queue size is 0\n");
        atomic_store(&q->dequeue_lock, expected);
        return NULL;
    }

    // atomically copy the head pointer
    __sync_swap(&li,q->head->next);
    //printf("dequeue: li->next = %p, li->item = %p\n",li->next, li->item);
    // we were probably preempted, release the lock and return
    if (li->item == NULL) {
        atomic_store(&q->dequeue_lock, expected);
        return NULL;
    }
    void *tmp = li->item;
    // null it out
    __sync_swap(&li->item, NULL);
    // atomically set the head pointer to its next
    __sync_swap(&q->head->next, li->next);

    if (aqueue_size(q) == 1)
        __sync_swap(&q->tail, q->head);

    // we removed something off the queue, release the lock
    atomic_fetch_sub(&(q->size), 1);
    atomic_store(&q->dequeue_lock, expected);
    // do the expensive part at the end
    free(li);

    //return item;
    return tmp;
}

/*
 * stack data structure built using a singly linked list
 * create_stack - creates a stack
 * destroy_stack - cleans it up
 * push_item - push an item onto the top of the stack
 * pop_item - pop an item off the top of the stack
 * peek_item - peek the item on the top of the stack
 */
void push_item(stack_t *s, void *item, size_t size) {
    slist_item_t *li = malloc(sizeof(*li));
    MALLOC_TEST(li);

    li->item = malloc(size);
    MALLOC_TEST(li->item);

    // copy the item
    memcpy(li->item, item, size);
    li->next = NULL;

    if (s->top != NULL)
        li->next = s->top;

    s->top = li;
    s->size++;
}

void *pop_item(stack_t *s, void *item, size_t size) {
    if (s->top == NULL)
        return NULL;

    slist_item_t *li = s->top;
    // update the top of the stack
    s->top = li->next;
    s->size--;

    // copy the item to the address passed in
    memcpy(item, li->item, size);
    // free the list item
    free(li->item);
    free(li);

    return item;
}

void *peek_item(stack_t *s, void *item, size_t size) {
    if (s->top == NULL)
        return NULL;
    // copy the item on the top to the address passed in
    memcpy(item, s->top->item, size);
    return item;
}

stack_t *create_stack() {
    stack_t *s = malloc(sizeof(*s));
    MALLOC_TEST(s);

    s->top = NULL;
    s->size = 0;

    return s;
}

void destroy_stack(stack_t *s) {
    if (s == NULL)
        return;

    slist_item_t *curr = s->top;

    while (curr != NULL) {
        slist_item_t *tmp = curr->next;
        free(curr->item);
        free(curr);
        curr = tmp;
    }

    free(s);
}

void print_book_node(book_t *graph, book_t *bn) {
    printf("---------- Book Node ----------\n");
    printf("book_id:      %zu\n",bn->id);
    printf("author_id:    %zu\n",bn->author_id);
    printf("publisher_id: %zu\n",bn->publisher_id);
    printf("n_author:     %zu\n",bn->n_author_edges);
    printf("n_publisher:  %zu\n",bn->n_publisher_edges);
    printf("n_cited:      %zu\n",bn->n_citation_edges);

    for (int i = 0; i < bn->n_author_edges; i++)
        printf("b_author_edges[%d]: %zu (index: %zu)\n", i, graph[bn->b_author_edges[i]].id, bn->b_author_edges[i]);
    for (int i = 0; i < bn->n_citation_edges; i++)
        printf("b_citation_edges[%d]: %zu (index: %zu)\n", i, graph[bn->b_citation_edges[i]].id, bn->b_citation_edges[i]);
    for (int i = 0; i < bn->n_publisher_edges; i++)
        printf("b_publisher_edges[%d]: %zu (index: %zu)\n", i, graph[bn->b_publisher_edges[i]].id, bn->b_publisher_edges[i]);
    printf("-------------------------------\n\n");
}

void copy_book_node(book_t *dst, book_t *src) {
    memcpy(dst, src, sizeof(book_t));
}

// assemble the result data
void build_result_data(book_t **nodes, size_t node_count,
                       result_t *result, int *result_from) {
    int curr = *result_from;
    // copy node_count pointers from the thread data to the result elements
    memcpy(&result->elements[curr], nodes, sizeof(book_t *) * node_count);
    result->n_elements += node_count;

    *result_from += node_count;
}

// helper to partition an array
int partition_array(int count, int id, int n_threads, int *start, int *end) {
    int slice   = count / n_threads;
    int remains = count % n_threads;
    int starts  = slice * id;

    if (remains > 0) {
        if (id < remains)
            slice++;

        starts = slice*id;

        if (id >= remains)
            starts += remains;
    }

    int ends = (starts+slice) < count ? starts+slice : count;

    if (start != NULL)
        *start = starts;
    if (end != NULL)
        *end = ends;

    return ends - starts;
}

// perform a bfs to find the shortest path between set a and set b using all edges
size_t *bfs_shortest_path(book_t *graph, int count, size_t *a_index, int a_index_count,
                          size_t *b_index, int b_index_count, int *walk_len, int threads) {
    size_t *walk = NULL;
    int v_walked = 0;
    // initialize the length of the walk
    *walk_len = 0;
    // whether we've visited particular nodes
    map_t *visited = create_hashmap(count/2);
    map_t *v_from = create_hashmap(count/2);
    map_t *b_idx_map = create_hashmap(b_index_count *2);
    cqueue_t *q = create_cqueue(count);
    // of course we've visited the start node
    for (int i = 0; i < a_index_count; i++)
        hashmap_put(visited, a_index[i], NULL);
    for (int i = 0; i < b_index_count; i++)
        hashmap_put(b_idx_map, b_index[i], NULL);
    for (int i = 0; i < a_index_count; i++)
        hashmap_put(v_from, a_index[i], NULL);
    for (int i = 0; i < a_index_count; i++)
        cqueue_enqueue(q, a_index[i]);

    // for storing vertex v
    size_t v, b_found = 0;
    int b_found_flag = 0;

    while (cqueue_dequeue(q, &v) != NULL) {
        //printf("queue size: %zu\n",q->size);
        int n_publisher_edges = graph[v].n_publisher_edges;
        int n_author_edges = graph[v].n_author_edges;
        int n_citation_edges = graph[v].n_citation_edges;
        // degree of v
        int degv = n_publisher_edges + n_author_edges + n_citation_edges;
        // all the vertices adjacent to vertex v
        size_t *adjacent = malloc(sizeof(*adjacent) * degv);
        int adj_ctr = 0;

        // build the adjacent vertex list
        for (int i = 0; i < n_author_edges; i++)
            adjacent[adj_ctr++] = graph[v].b_author_edges[i];
        for (int i = 0; i < n_citation_edges; i++)
            adjacent[adj_ctr++] = graph[v].b_citation_edges[i];
        for (int i = 0; i < n_publisher_edges; i++)
            adjacent[adj_ctr++] = graph[v].b_publisher_edges[i];

        for (int i = 0; i < adj_ctr; i++) {
            size_t w = adjacent[i];
            // have we visited w yet
            if (!hashmap_exists(visited, w)) {
                hashmap_put(visited, w, NULL);
                // add w visted from v to the visited from hashmap
                hashmap_put(v_from, w, &v);
                // found book b, flag that we don't need to go any deeper into the bfs tree
                if (hashmap_exists(b_idx_map, w)) {
                    b_found = w;
                    b_found_flag = 1;
                    break;
                }
                // enqueue w
                cqueue_enqueue(q, w);
            }
        }

        free(adjacent);
        // terminate early
        if (b_found_flag)
            break;
    }

    // did we find a path from v_a to v_b
    if (b_found_flag && hashmap_exists(v_from, b_found)) {
        // create a stack to reverse the path from v_b to v_a
        stack_t *s = create_stack();

        size_t  w = b_found;
        int a_found = 0;
        // push w onto the stack
        push_item(s, (void *)&w, sizeof(w));
        // loop until we reach the start vertex
        while (!a_found) {
            size_t value;
            // get the value for key[w]
            hashmap_get(v_from, w, &value);
            w = value;
            // push v onto the stack
            push_item(s, (void *)&w, sizeof(w));

            for (int i = 0; i < a_index_count; i++) {
                if (w == a_index[i]) {
                    a_found = 1;
                    break;
                }
            }
        }

        // the walk length is
        v_walked = s->size;
        // allocate a walk of stack size
        walk = malloc(sizeof(*walk) * v_walked);
        int wctr = 0;
        // reverse the path
        while (pop_item(s, (void *)&w, sizeof(w)))
            walk[wctr++] = w;
        // cleanup
        destroy_stack(s);
    }

    // destroy the visit hashmaps
    destroy_hashmap(visited);
    destroy_hashmap(v_from);
    destroy_hashmap(b_idx_map);
    // destroy the queue
    destroy_cqueue(q);

    // store the length in the callers length parameter
    *walk_len = v_walked;
    // return the walk
    return walk;
}

book_t **bfs_distance(book_t *graph, int count, size_t a_index,
                       size_t k, int *found_len) {
    book_t **found = NULL;
    int v_found = 0;
    size_t v_level = 0;
    // initialize the length of the walk
    *found_len = 0;
    map_t *visited = create_hashmap(count);
    // insert the root node on the current level
    hashmap_put(visited, a_index, &v_level);
   // create the queue for bfs
    queue_t *q = create_queue();
    enqueue_item(q, a_index);
    // for storing vertex v
    size_t v;

    //while (cqueue_dequeue(q,&v) != NULL) {
    while (dequeue_item(q,&v) != NULL) {
        hashmap_get(visited, v, &v_level);
        // reached k
        if (++v_level == k+1) {
            break;
        }
        // degree of v
        int degv = graph[v].n_citation_edges;

        for (int i = 0; i < degv; i++) {
        // for each adjacent vertex w in related publisher books
            size_t w = graph[v].b_citation_edges[i];
            // have we visited w yet
            if (!hashmap_exists(visited,w)) {
                hashmap_put(visited, w, &v_level);
                enqueue_item(q,w);
            }
        }
    }

    size_t *ks = hashmap_keyset(visited,&v_found);
    found = malloc(sizeof(book_t *)*v_found);
    for (int i = 0; i < v_found; i++)
        found[i] = &graph[ks[i]];

    destroy_hashmap_keyset(ks);
    // destroy the queue
    destroy_queue(q);
    destroy_hashmap(visited);
    // store the length in the callers length parameter
    *found_len = v_found;
    return found;
}

result_t *find_book(book_t *nodes, size_t count, size_t id) {
    if (count >= 100000) {
        return find_book_cached(nodes, count, id);
    } else {
        return find_book_uncached(nodes, count, id);
    }
}

result_t *find_book_cached(book_t *nodes, size_t count, size_t id) {
    // result
    result_t *result = malloc(sizeof(*result));
    memset(result, 0, sizeof(*result));

    create_bookid_cache(nodes,count);

    if (!hashmap_exists(g_bookid_cache.map,id))
        return result;

    size_t idx;
    hashmap_get(g_bookid_cache.map,id,&idx);

    result->elements = malloc(sizeof(book_t *));
    result->n_elements = 1;
    result->elements[0] = &nodes[idx];

    return result;
}

result_t *find_book_uncached(book_t *nodes, size_t count, size_t id) {
    // result
    result_t *result = malloc(sizeof(*result));
    memset(result, 0, sizeof(*result));

    for (int i = 0; i < count; i++) {
        if (nodes[i].id == id) {
            result->elements = malloc(sizeof(book_t *));
            result->n_elements = 1;
            result->elements[0] = &nodes[i];
            break;
        }
    }

    return result;
}

result_t *find_books_by_author(book_t *nodes, size_t count, size_t author) {
    // allocate the result struct to return
    result_t *result = malloc(sizeof(*result));
    memset(result, 0, sizeof(*result));

    for (int i = 0; i < count; i++) {
        // found a book with a matching author id
        if (nodes[i].author_id == author) {
            size_t n_author_edges = nodes[i].n_author_edges;
            // allocate enough for n_author_edges+1 to account for the current book node
            result->elements = malloc(sizeof(book_t *) * (n_author_edges+1));
            result->elements[0] = &nodes[i];
            // copy all the books found under b_author_edges
            for (int j = 0; j < n_author_edges; j++)
                result->elements[j+1] = &nodes[ nodes[i].b_author_edges[j] ];

            result->n_elements = n_author_edges+1;

            break;
        }
    }

    return result;
}

result_t* find_books_reprinted(book_t *nodes, size_t count, size_t publisher_id) {
    // allocate the result struct to return
    result_t *result = malloc(sizeof(*result));
    memset(result, 0, sizeof(*result));

    size_t *idxlist = NULL;
    size_t  n_idxlist = 0;

    // prepare the list of publisher book ids for finding reprints
    for (int i = 0; i < count; i++) {
        if (nodes[i].publisher_id == publisher_id) {
            n_idxlist  = nodes[i].n_publisher_edges+1;
            idxlist    = malloc(sizeof(*idxlist)*n_idxlist);
            idxlist[0] = i;

            for (int j = 0, k = 1; j < nodes[i].n_publisher_edges; j++, k++) {
                idxlist[k] = nodes[i].b_publisher_edges[j];
            }
            break;
        }
    }

    if (n_idxlist == 0)
        return result;

    map_t *reprints = create_hashmap(20);

    for (int i = 0; i < n_idxlist; i++) {
        // book id we're searching for
        size_t bookid = nodes[idxlist[i]].id;
        size_t n_author_edges = nodes[idxlist[i]].n_author_edges;
        // search through the nodes for each bookid and check whether its been reprinted
        for (int j = 0; j < n_author_edges; j++) {
            size_t tmp_idx = nodes[idxlist[i]].b_author_edges[j];
            // if same book id and different publisher id
            if (nodes[tmp_idx].id == bookid && nodes[idxlist[i]].publisher_id != nodes[tmp_idx].publisher_id) {
                hashmap_put(reprints, tmp_idx, NULL);
            }
        }
    }

    int result_count, result_idx = 0;
    size_t *ks = hashmap_keyset(reprints, &result_count);

    if (result_count > 0) {
        result->elements = malloc(sizeof(book_t *) * result_count);

        for (int i = 0; i < result_count; i++)
            result->elements[result_idx++] = &nodes[ks[i]];
    }

    result->n_elements = result_count;
    destroy_hashmap_keyset(ks);
    destroy_hashmap(reprints);
    free(idxlist);

    return result;
}


// wrapper for calling either single or multithreaded version of the function
result_t* find_books_k_distance(book_t *nodes, size_t count,
                                size_t book_id, uint16_t k) {
    return find_books_k_distance_st(nodes, count, book_id, k);
}

/* single threaded k distance version
 */
result_t* find_books_k_distance_st(book_t *nodes, size_t count,
                                   size_t book_id, uint16_t k) {
    // allocate the result struct to return
    result_t *result = malloc(sizeof(*result));
    memset(result, 0, sizeof(*result));
    ssize_t book_idx = -1;

    // find the index for the bookid
    for (int i = 0; i < count; i++) {
        if (nodes[i].id == book_id) {
            book_idx = i;
            break;
        }
    }
    // book not found
    if (book_idx == -1)
        return result;

    int n_found = 0;

    result->elements = bfs_distance(nodes, count, (size_t)book_idx, k, &n_found);
    result->n_elements = n_found;

    return result;
}

/*
 * TODO write ze code
 */
void *find_shortest_distance_worker(void *arg) {
    thr_findbook_t *tb = (thr_findbook_t *)arg;

    book_t  *nodes      = tb->nodes;
    book_t **found      = NULL;

    int    n_threads = tb->n_threads;
    int         dist = 0;

    size_t *walk = bfs_shortest_path(
        tb->nodes, tb->count, tb->a_index, tb->a_index_count,
        tb->b_index, tb->b_index_count, &dist, n_threads);

    if (dist == 0) {
        tb->nodes_found = found;
        tb->nodes_found_count = dist;
        return NULL;
    }

    found = malloc(sizeof(book_t *) * dist);

    for (int i = 0; i < dist; i++)
        found[i] = &nodes[walk[i]];

    tb->nodes_found = found;
    tb->nodes_found_count = dist;

    // free the vertex list of the walk
    free(walk);

    return NULL;
}

/*
 * TODO Make work??
 */
result_t* find_shortest_distance(book_t *nodes, size_t count,
                                 size_t b1_id, size_t b2_id) {

    // allocate the result struct to return
    result_t *result = malloc(sizeof(*result));
    memset(result, 0, sizeof(*result));
    result->n_elements = 0;
    size_t l_nthreads = g_nthreads;
    // will fix later
    g_nthreads = 1;
    // a walk of length 0 exists from a_id to itself
    if (b1_id == b2_id) {
        ssize_t a_idx = -1;
        ssize_t b_idx = -1;
        int a_idx_count = 0;

        for (int i = 0; i < count; i++) {
            if (nodes[i].id == b1_id) {
                    a_idx = i;
                a_idx_count++;
                for (int j = 0; j < nodes[i].n_author_edges; j++)
                    if (nodes[ nodes[i].b_author_edges[j] ].id == b1_id) {
                        a_idx_count++;
                        b_idx = nodes[i].b_author_edges[j];
                        break;
                    }

                break;
            }
        }

        if (a_idx_count > 0) {
            result->elements = malloc(sizeof(book_t *) * a_idx_count);
            result->elements[0] = &nodes[a_idx];
            if (a_idx_count == 2)
                result->elements[1] = &nodes[b_idx];
            result->n_elements = a_idx_count;
        }

        return result;
    }

    int a_idx_found = 0, b_idx_found = 0;
    int a_cap = 5, b_cap = 5;

    size_t *a_idx = malloc(sizeof(size_t) * a_cap);
    size_t *b_idx = malloc(sizeof(size_t) * b_cap);

    // first check to see if book node a_id exists, else we cant continue
    for (int i = 0; i < count; i++) {
        if (nodes[i].id == b1_id) {
            a_idx[a_idx_found++] = i;

            if (a_idx_found == a_cap) {
                a_cap *= 2;
                a_idx = realloc(a_idx, sizeof(size_t) * a_cap);
            }
        }
        if (nodes[i].id == b2_id) {
            b_idx[b_idx_found++] = i;

            if (b_idx_found == b_cap) {
                b_cap *= 2;
                b_idx = realloc(b_idx, sizeof(size_t) * b_cap);
            }
        }
    }

    // either a_id or b_id doesnt exist so neither can a minimal walk
    if (a_idx_found == 0 || b_idx_found == 0) {
        free(a_idx);
        free(b_idx);
        return result;
    }

    // allocate threads
    pthread_t      *threads = malloc(sizeof(*threads) * g_nthreads);
    thr_findbook_t *thrdata = malloc(sizeof(*thrdata) * g_nthreads);

    // initialize the "threads"
    for (int i = 0; i < g_nthreads; i++) {
        thrdata[i].tid       = i;
        thrdata[i].param1    = b1_id;
        thrdata[i].param2    = b2_id;
        thrdata[i].nodes     = nodes;
        thrdata[i].count     = count;
        // reduce the thread count by 1 since
        thrdata[i].n_threads = g_nthreads;
        thrdata[i].actr      = NULL;
        // only needs to store 1 result
        thrdata[i].nodes_found = NULL;
        thrdata[i].nodes_found_count = 0;
        // pass in the source and dest node pointers
        thrdata[i].a_index = a_idx;
        thrdata[i].a_index_count = a_idx_found;
        thrdata[i].b_index = b_idx;
        thrdata[i].b_index_count = b_idx_found;

        if (g_nthreads > 1) {
            // create the threads
            pthread_create(&threads[i], NULL,
                find_shortest_distance_worker,(void *)&thrdata[i]);
        }
    }

    // noop
    usleep(0);

    // if thread count > 1 join threads else run single threaded
    if (g_nthreads > 1) {
        for (int i = 0; i < g_nthreads; i++)
            pthread_join(threads[i], NULL);
    } else {
        find_shortest_distance_worker((void *)&thrdata[0]);
    }

    int result_idx = 0, result_count = 0;

    // sum up the result count
    for (int i = 0; i < g_nthreads; i++)
        result_count += thrdata[i].nodes_found_count;

    // combine the result data if our result count > 0
    if (result_count > 0) {
        // allocate enough to store the combined result
        result->elements = malloc(sizeof(book_t *) * result_count);
        // build the result data
        for (int i = 0; i < g_nthreads; i++) {
            if (thrdata[i].nodes_found_count > 0) {
                // assemble the result data
                build_result_data(
                    thrdata[i].nodes_found, thrdata[i].nodes_found_count,
                    result, &result_idx);
            }
        }
    }

    // free the pointer array
    for (int i = 0; i < g_nthreads; i++)
        free(thrdata[i].nodes_found);

    free(threads);
    free(thrdata);
    free(a_idx);
    free(b_idx);

    g_nthreads = l_nthreads;

    return result;
}
result_t* find_shortest_edge_type(book_t* nodes, size_t count, size_t a1_id, size_t a2_id) {
    return NULL;
}

result_t* find_books_k_distance_mt2(book_t *nodes, size_t count, size_t book_id, uint16_t k) {
    // allocate the result struct to return
    result_t *result = malloc(sizeof(*result));
    memset(result, 0, sizeof(*result));

    size_t nthreads = g_nthreads, level = 0;
    ssize_t book_idx = -1;
    // find the index for the bookid
    for (int i = 0; i < count; i++) {
        if (nodes[i].id == book_id) {
            book_idx = i;
            break;
        }
    }
    // book not found
    if (book_idx == -1)
        return result;

    // create a queue and enqueue the first vertex
    aqueue_t *sharedq = create_aqueue();
    aqueue_enqueue(sharedq, (void *)&book_idx, sizeof(book_idx));
    // visited the start vertex
    map_t *vshared = create_hashmap(100);
    hashmap_put(vshared, (size_t)book_idx, &level);

    size_t *a_found = malloc(sizeof(size_t)*count);
    memset(a_found, 0, sizeof(size_t)*count);

    a_found[book_idx] = 1;

    size_t *pv = NULL;

    // prepare queue for workers
    while ((pv = (size_t *)aqueue_dequeue(sharedq)) != NULL) {
        size_t v = *pv;
        free(pv);
        hashmap_get(vshared, v, &level);
        level++;
        int degv = nodes[v].n_citation_edges;

        for (int i = 0; i < degv; i++) {
            size_t w = nodes[v].b_citation_edges[i];
            // have we visited w yet
            if (!hashmap_exists(vshared, w)) {
                //atomic_store(&a_found[w],1);
                hashmap_put(vshared, w, &level);
                a_found[w] = 1;
                aqueue_enqueue(sharedq, &w, sizeof(w));
            }
        }

        // we have enough to partition
        if (aqueue_size(sharedq) >= 4)
            break;
    }

    thr_kdist_t *thrdata = malloc(sizeof(*thrdata) * nthreads);
    pthread_t   *threads = malloc(sizeof(*threads) * nthreads);
    int qcount = aqueue_size(sharedq);

    for (int i = 0; i < nthreads; i++) {
        // create the thread data
        thrdata[i] = (thr_kdist_t){
            .tid = i, .k = k, .nodes = nodes, .count = count,
            .nthreads = nthreads, .sharedq = sharedq,
            .vshared = vshared, .sharedq_count = qcount,
            .vfound = a_found
        };

        pthread_create(&threads[i], NULL, find_books_k_distance_mt2_worker, &thrdata[i]);
    }

    // join threads
    for (int i = 0; i < nthreads; i++)
        pthread_join(threads[i],NULL);

    result->elements = malloc(sizeof(book_t *) * count);

    //int ks_size = 0, ctr = 0;
    int ctr = 0;
    //size_t *ks = hashmap_keyset(vshared, &ks_size);

    for (int i = 0; i < count; i++)
        if (a_found[i])
            result->elements[ctr++] = &nodes[a_found[i]];

    // for (int i = 0; i < ks_size; i++)
    //     result->elements[ctr++] = &nodes[ks[i]];

    result->n_elements = ctr;
    //result->elements = realloc(result->elements, sizeof(book_t) * ctr);

    destroy_aqueue(sharedq);
    destroy_hashmap(vshared);
    free(a_found);
    free(thrdata);
    free(threads);

    return result;
}

void *find_books_k_distance_mt2_worker(void *arg) {
    thr_kdist_t *data = (thr_kdist_t *)arg;

    //map_t *vpriv = create_hashmap(data->count);
    map_t *vshared = data->vshared;
    aqueue_t *qshared = data->sharedq;
    book_t *nodes = data->nodes;
    size_t *vfound = data->vfound;

    int tid = data->tid;
    size_t k = data->k;
    size_t nthreads = data->nthreads;
    ssize_t *vlevel = malloc(sizeof(ssize_t)*data->count);

    for (int i = 0; i < data->count; i++)
        vlevel[i] = -1L;

    // private queue
    queue_t *qpriv = create_queue();
    // get a count of how many we need to dequeue
    int count = partition_array(data->sharedq_count, tid, nthreads, NULL, NULL);
    //printf("thread %d -> count: %d\n",tid, count);
    int ctr = 0;
    // dequeue stuff from shared queue
    while (ctr < count) {
        size_t *pv = NULL;
        while ((pv = (size_t *)aqueue_dequeue(qshared)) == NULL);
        size_t v = *pv, level = 0;
        free(pv);
        // get from the shared hashmap
        hashmap_get(vshared, v, &level);
        vlevel[v] = level;
        // put it into our private hashmap
        //hashmap_put(vpriv, v, &level);
        // re-enqueue it into our private queue
        enqueue_item(qpriv, v);
        ctr++;
    }

    size_t v;
    // prepare queue for workers
    while (dequeue_item(qpriv,&v) != NULL) {
        size_t level = 0;

        int degv = nodes[v].n_citation_edges;
        level = vlevel[v];
        //hashmap_get(vpriv, v, &level);
        // reached k
        if (level++ == k)
            break;

        for (int i = 0; i < degv; i++) {
            size_t w = nodes[v].b_citation_edges[i];
            // have we visited w yet
            // if (!hashmap_exists(vpriv, w)) {
            if (vlevel[w] < 0) {
                vfound[w] = 1;
                //hashmap_put(vpriv, w, &level);
                vlevel[w] = level;
                enqueue_item(qpriv, w);
                //atomic_store(&vfound[w],1);
            }
        }
    }

    //destroy_hashmap(vpriv);
    free(vlevel);
    destroy_queue(qpriv);
    return NULL;
}