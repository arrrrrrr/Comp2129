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

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "worm.h"

#define MAX_BUFFER 65535

// Reads given graph file and returns a book graph.
book_t* graph_loader(size_t* count, char* filename) {

    char buffer[MAX_BUFFER];
    size_t n_books = 0;

    // Open graph file
    FILE* f = fopen(filename, "r");
    if (f == NULL) {
        perror("Fatal error! Unable to open graph file");
        return NULL;
    }

    // Read book count
    if (fgets(buffer, MAX_BUFFER, f) == NULL || sscanf(buffer, "%zu", &n_books) == 0) {
        fprintf(stderr, "Fatal error! Unable to read book count.\n");
        return NULL;
    }

    book_t* graph = malloc(sizeof(book_t) * n_books);

    // Read books
    for (size_t i = 0; i < n_books; i++) {

        // Read book ID
        if (fgets(buffer, MAX_BUFFER, f) == NULL || sscanf(buffer, "%zu", &graph[i].id) == 0) {
            fprintf(stderr, "Fatal error! Unable to read book ID for book %zu.\n", i);
            return NULL;
        }

        // Read publisher ID
        if (fgets(buffer, MAX_BUFFER, f) == NULL || sscanf(buffer, "%zu", &graph[i].publisher_id) == 0) {
            fprintf(stderr, "Fatal error! Unable to read publisher ID for book %zu.\n", i);
            return NULL;
        }

        // Read author ID
        if (fgets(buffer, MAX_BUFFER, f) == NULL || sscanf(buffer, "%zu", &graph[i].author_id) == 0) {
            fprintf(stderr, "Fatal error! Unable to read author ID for book %zu.\n", i);
            return NULL;
        }

        size_t cap = 10;
        size_t size = 0;

        buffer[MAX_BUFFER - 1] = '\0';
        size_t length = MAX_BUFFER - 1;
        graph[i].b_publisher_edges = malloc(sizeof(size_t) * cap);

        // Read publisher edges
        while (length == MAX_BUFFER - 1 && buffer[length] != '\n') {
            if (fgets(buffer, MAX_BUFFER, f) == NULL) {
                fprintf(stderr, "Fatal error! Unable to read publishers for book %zu.\n", i);
            }

            length = strlen(buffer);
            for (char* s = strtok(buffer, " "); s != NULL; s = strtok(NULL, " ")) {
                if (size == cap) {
                    cap = cap * 2;
                    graph[i].b_publisher_edges = realloc(graph[i].b_publisher_edges, sizeof(size_t) * cap);
                }
                if (strcmp("\n", s) != 0) {
                    size_t k = strtol(s, NULL, 10);
                    graph[i].b_publisher_edges[size] = k;
                    size++;
                }
            }
        }
        graph[i].n_publisher_edges = size;

        cap = 10;
        size = 0;

        buffer[MAX_BUFFER - 1] = '\0';
        length = MAX_BUFFER - 1;
        graph[i].b_author_edges = malloc(sizeof(size_t) * cap);

        // Read author edges
        while (length == MAX_BUFFER - 1 && buffer[length] != '\n') {
            if (fgets(buffer, MAX_BUFFER, f) == NULL) {
                fprintf(stderr, "Fatal error! Unable to read publishers for book %zu.\n", i);
            }

            length = strlen(buffer);
            for (char* s = strtok(buffer, " "); s != NULL; s = strtok(NULL, " ")) {
                if (size == cap) {
                    cap = cap * 2;
                    graph[i].b_author_edges = realloc(graph[i].b_author_edges, sizeof(size_t) * cap);
                }
                if (strcmp("\n", s) != 0) {
                    size_t k = strtol(s, NULL, 10);
                    graph[i].b_author_edges[size] = k;
                    size++;
                }
            }
        }
        graph[i].n_author_edges = size;

        cap = 10;
        size = 0;

        buffer[MAX_BUFFER - 1] = '\0';
        length = MAX_BUFFER - 1;
        graph[i].b_citation_edges = malloc(sizeof(size_t) * cap);

        // Read citation edges
        while (length == MAX_BUFFER - 1 && buffer[length] != '\n') {
            if (fgets(buffer, MAX_BUFFER, f) == NULL) {
                fprintf(stderr, "Fatal error! Unable to read citations for book %zu.\n", i);
            }

            length = strlen(buffer);
            for (char* s = strtok(buffer, " "); s != NULL; s = strtok(NULL, " ")) {
                if (size == cap) {
                    cap *= 2;
                    graph[i].b_citation_edges = realloc(graph[i].b_citation_edges, sizeof(size_t) * cap);
                }
                if (strcmp("\n", s) != 0) {
                    size_t k = strtol(s, NULL, 10);
                    graph[i].b_citation_edges[size] = k;
                    size++;
                }
            }
        }
        graph[i].n_citation_edges = size;
    }

    *count = n_books;
    return graph;
}

void test_sample(book_t* graph, size_t count) {

    result_t* r1 = find_book(graph, count, 9);
    if (r1 == NULL) {
        fprintf(stderr, "Fail! find_book() => result set is NULL.\n");
    } else if (r1->n_elements != 1) {
        fprintf(stderr, "Fail! find_book() => result set contains %zu elements.\n", r1->n_elements);
    } else if (r1->elements[0]->id != 9) {
        fprintf(stderr, "Fail! find_book() => result set contains book with ID %zu.\n", r1->n_elements);
    }

    result_t* r2 = find_books_by_author(graph, count, 1);
    if (r2 == NULL) {
        fprintf(stderr, "Fail! find_books_by_author() => result set is NULL.\n");
    } else if (r2->n_elements != 5) {
        fprintf(stderr, "Fail! find_books_by_author() => result set contains %zu elements.\n", r2->n_elements);
    } else {
        size_t find_count = 0;
        size_t book_ids[] = {0, 4, 6, 11, 15};
        for (size_t i = 0; i < 5; i++) {
            for (size_t j = 0; j < 5; j++) {
                if (r2->elements[i]->id == book_ids[j]) {
                    find_count += 1;
                    book_ids[j] = -1;
                    break;
                }
            }
        }
        if (find_count != 5) {
            fprintf(stderr, "Fail! find_books_by_author() => result set contains %zu incorrect elements.\n", 5 - find_count);
        }
    }

    if (r1 != NULL) {
        if (r1->elements != NULL) {
            free(r1->elements);
        }
        free(r1);
    }

    if (r2 != NULL) {
        if (r2->elements != NULL) {
            free(r2->elements);
        }
        free(r2);
    }
}

int main(int argc, char** argv) {

    // Example usage

    size_t count = 0;
    book_t* graph = graph_loader(&count, "sample.graph");
    if (graph == NULL) {
        return 1;
    }

    test_sample(graph, count);

    for (size_t i = 0; i < count; i++) {
        free(graph[i].b_author_edges);
        free(graph[i].b_publisher_edges);
        free(graph[i].b_citation_edges);
    }

    free(graph);

    return 0;
}
