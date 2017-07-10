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

int main(int argc, char **argv)
{
    printf("Enter strand: ");

    char buf[BUF_SIZE] = {0}, c;
    int read = 0;

    while ((c = getchar()) != EOF) {
        if (c == '\n')
            break;

        if (read < BUF_SIZE)
            buf[read++] = c;
    }

    if (c == EOF)
        printf("\n");

    printf("\n");

    if (strlen(buf) == 0) {
        printf("No strand provided.\n");
        return 1;
    }

    char *pbuf = buf;

    while (*pbuf) {
        switch (toupper(*pbuf)) {
            case 'A': *pbuf = *pbuf + ('T'-'A'); break;
            case 'T': *pbuf = *pbuf - ('T'-'A'); break;
            case 'G': *pbuf = *pbuf - ('G'-'C'); break;
            case 'C': *pbuf = *pbuf + ('G'-'C'); break;
            default:  *pbuf = 'x'; break;
        }
        pbuf++;
    }

    printf("Complementary strand is %s\n", buf);

    return 0;
}