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
#include <string.h>

#define BUF_SIZE 1024

int main(int argc, char **argv) {
    switch (argc) {
        case 1: printf("No arguments\n"); return 1;
        case 2: printf("Not enough arguments\n"); return 1;
        case 3: break;
        default: printf("Too many arguments\n"); return 1;
    }

    if (strlen(argv[1]) != strlen(argv[2])) {
        printf("Invalid arguments\n");
        return 1;
    }

    char buf[BUF_SIZE] = { 0 };
    char c;
    int read = 0;

    while ((c = getchar()) != EOF)
        buf[read++] = c;

    for (int i = 0; i < read; i++)
        for (int j = 0; j < strlen(argv[1]); j++)
            if (buf[i] == argv[1][j])
                buf[i] = argv[2][j];

    printf("%s", buf);
}