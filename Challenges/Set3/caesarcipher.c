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
#include <ctype.h>

#define BUF_SIZE 1024

int main(void) {
    printf("Enter key: ");

    char buf[BUF_SIZE], c;
    int key = -1, read = 0;

    while ((c = getchar()) != EOF) {
        if (c == '\n')
            break;

        buf[read++] = c;
    }

    if (c == EOF)
        printf("\n");

    buf[read] = '\0';

    if (sscanf(buf, "%d", &key) == 0 || key < 0 || key > 26) {
        printf("\nInvalid key!\n");
        return 1;
    }

    read = 0;

    printf("Enter line: ");

    while ((c = getchar()) != EOF) {
        if (c == '\n')
            break;

        buf[read++] = c;
    }

    buf[read] = '\0';

    char *pbuf = buf;

    while (*pbuf) {
        if (isalpha(*pbuf)) {
            if (islower(*pbuf))
                c = ((*pbuf - 'a' + key) % 26) + 'a';
            else
                c = ((*pbuf - 'A' + key) % 26) + 'A';

            *pbuf = c;
        }
        pbuf++;
    }

    printf("\n%s\n",buf);
}