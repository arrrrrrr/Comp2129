/*
 * Copyright (c) 2017 Michael Zammit
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

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