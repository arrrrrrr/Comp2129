#ifndef REFCOUNTING_H
#define REFCOUNTING_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

typedef struct ref_entry_t ref_entry_t;

struct ref_entry_t {
    void *alloc;
    int ctr;
    ref_entry_t *next;
    ref_entry_t *child;
};

typedef struct ref_table_t {
    ref_entry_t *head;
    int count;
} ref_table_t;

extern ref_table_t g_reftable;

ref_entry_t *find_ref(ref_entry_t **entry, ref_entry_t ***parent, void *ref);
void *new_ref(size_t size, void* dep);
void *new_ref_priv(ref_entry_t *root, size_t size);
void *assign_ref(void* ref);
void *assign_ref_priv(ref_entry_t *root);
void *del_ref(void* ref);
ref_entry_t *del_ref_priv(ref_entry_t *parent, ref_entry_t *root);
void *refcount_ref(void *ref, int *count);
void cleanup_reftable_root();
void cleanup() __attribute__((destructor));
void cleanup_priv(ref_entry_t *root);

#endif