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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "nonogram.h"

#define BUF_SIZE 4096

void print_solution(nonogram_t *png) {
    printf("X:\n");

    for (int i = 0; i < png->x_runs_length; i++) {
        run_t *this_run = &png->x_runs[i];

        for (int j = 0; j < this_run->length; j++) {
            printf("%s%d",(j == 0) ? "" : " ",this_run->run[j]);
        }

        printf("\n");
    }

    printf("\nY:\n");

    for (int i = 0; i < png->y_runs_length; i++) {
        run_t *this_run = &png->y_runs[i];

        for (int j = 0; j < this_run->length; j++) {
            printf("%s%d",(j == 0) ? "" : " ",this_run->run[j]);
        }

        printf("\n");
    }
}

void solve_nonogram(nonogram_t *png, char *data) {
    for (int h = 0; h < 2; h++) {
        int i_limit = (h==0) ? png->x_runs_length : png->y_runs_length;
        int j_limit = (h==0) ? png->y_runs_length : png->x_runs_length;

        for (int i = 0; i < i_limit; i++) {
            int count = 0, current = 0, *row;

            row = malloc(sizeof(*row)*j_limit);
            memset(row, 0, j_limit);

            for (int j = j_limit-1; j >= 0; j--) {
                int idx = (h==0) ? i*png->y_runs_length+j
                                 : j*png->y_runs_length+i;

                if (data[idx] == '1') {
                    current++;

                    if (j != 0)
                        continue;
                }

                if (current > 0) {
                    row[count++] = current;
                    current = 0;
                }
            }

            if (count == 0) {
                count++;
                row[0] = 0;
            }

            run_t *this_run = (h==0) ? &png->x_runs[i] : &png->y_runs[i];

            this_run->length = count;
            this_run->run = malloc(sizeof(int)*count);
            memset(this_run->run, 0, count);

            for (int k = 0; k < count; k++)
                this_run->run[k] = row[k];

            free(row);
        }
    }
}

nonogram_t *create_nonogram(char *data, int width, int height) {
    nonogram_t *png = NULL;
    png = malloc(sizeof(*png));

    if (png == NULL) {
        printf("Malloc failed\n");
        exit(1);
    }

    png->x_runs_length = height;
    png->y_runs_length = width;

    png->x_runs = malloc(sizeof(run_t)*png->x_runs_length);

    if (png->x_runs == NULL) {
        printf("Malloc failed\n");
        exit(1);
    }

    png->y_runs = malloc(sizeof(run_t)*png->y_runs_length);

    if (png == NULL) {
        printf("Malloc failed\n");
        exit(1);
    }

    for (int i = 0; i < png->x_runs_length; i++) {
        run_t *this_run = &png->x_runs[i];
        this_run->length = 0;
        this_run->run = NULL;
    }

    for (int i = 0; i < png->y_runs_length; i++) {
        run_t *this_run = &png->y_runs[i];
        this_run->length = 0;
        this_run->run = NULL;
    }

    return png;
}

void free_nonogram(nonogram_t *png) {
    if (png == NULL)
        return;

    for (int i = 0; i < png->x_runs_length; i++) {
        if (png->x_runs[i].run != NULL)
            free(png->x_runs[i].run);
    }

    free(png->x_runs);

    for (int i = 0; i < png->y_runs_length; i++) {
        if (png->y_runs[i].run != NULL)
            free(png->y_runs[i].run);
    }

    free(png->y_runs);

    free(png);
}

int main(void) {
    const char delim[2] = " ";
    int dim[2] = { 0 };
    int read = 0, err = 0;

    char c, *buf = NULL, *pbuf, *token;

    buf = malloc(sizeof(*buf)*BUF_SIZE);

    if (buf == NULL) {
        printf("Malloc failed\n");
        return 1;
    }

    pbuf = buf;

    while((c = getchar()) != '\n' && (pbuf-buf) < BUF_SIZE) {
        if (c == EOF) {
            printf("Cannot decode\n");
            return 1;
        }

        *pbuf++ = c;
    }

    *pbuf = '\0';
    token = strtok(buf, delim);

    while (token != NULL) {
        char *pEnd;
        long tmp = strtol(token, &pEnd, 10);

        if (read == 2) {
            err = 1;
            break;
        }

        if (*pEnd != '\0') {
            err = 1;
            break;
        }

        dim[read++] = (int)tmp;
        token = strtok(NULL, delim);
    }

    if (err || read < 2 || dim[0] < 0 || dim[1] < 0) {
        printf("Cannot decode\n");
        return 1;
    }

    // zero the buffer
    memset(buf, '\0', BUF_SIZE*sizeof(*buf));
    // move pbuf back to the start of the buffer
    pbuf = buf;
    read = 0;

    int readw = 0;

    while((c = getchar()) != EOF && (pbuf-buf) < BUF_SIZE) {
        if (c == '\n') {
            if (readw != dim[0]) {
                printf("Invalid image data\n");
                return 1;
            }

            read++;
            readw = 0;
            continue;
        }

        readw++;
        *pbuf++ = c;
    }

    // more or less chars than expected or reached the end of the buffer
    if ((pbuf-buf) != dim[0]*dim[1] || read != dim[1] || pbuf-buf == BUF_SIZE) {
        printf("Invalid image data\n");
        return 1;
    }

    for (int i = 0; i < dim[1]; i++) {
        for (int j = 0; j < dim[0]; j++) {
            int idx = i*dim[0]+j;
            int value = buf[idx]-'0';

            if (value < 0 || value > 1) {
                printf("Invalid image data\n");
                return 1;
            }
        }
    }

    nonogram_t *png = create_nonogram(buf, dim[0], dim[1]);

    solve_nonogram(png, buf);
    print_solution(png);

    free_nonogram(png);
    free(buf);

    return 0;
}
