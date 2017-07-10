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
#include <ctype.h>
#include <string.h>

#define BUF_SIZE 256

int main(void) {
    char linebuf[BUF_SIZE] = {0};
    char grambuf[BUF_SIZE] = {0};

    int lchars = 0, gchars = 0;
    char c;

    printf("Enter line: ");

    while ((c = getchar()) != '\n')
        if (isalnum(c))
            linebuf[lchars++] = tolower(c);

    printf("Enter anagram: ");

    while ((c = getchar()) != '\n')
        if (isalnum(c))
            grambuf[gchars++] = tolower(c);

    printf("\n");

    if (strlen(linebuf) != strlen(grambuf)) {
        printf("Not an anagram.\n");
        return 1;
    }

    char *ptrl, *ptrg;

    for ( ptrg = grambuf ; *ptrg; ptrg++ ) {
        ptrl = linebuf;

        while (*ptrl) {
            if (*ptrg == *ptrl) {
                *ptrl = '\x20';
                break;
            }
            ptrl++;
        }
    }

    int match = 0;

    for (int i = 0; i < lchars; i++)
        if (linebuf[i] == '\x20')
            match++;

    if (match == strlen(grambuf)) {
        printf("Anagram!\n");
    } else {
        printf("Not an anagram.\n");
    }
}