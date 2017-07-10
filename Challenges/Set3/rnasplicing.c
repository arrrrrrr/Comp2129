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
#include <stdlib.h>

#define BUF_SIZE 1024

int main(void) {
    printf("Input strand: ");

    char buf[BUF_SIZE], c;
    int read = 0;

    while ((c = getchar()) != EOF) {
        if (c == '\n')
            break;

        if (read < BUF_SIZE)
            buf[read++] = c;
    }

    if (c == EOF)
        printf("\n");

    buf[read] = '\0';

    char dstbuf[BUF_SIZE];
    char *pbuf = buf, *pdstbuf = dstbuf, *pintron = NULL;

    while (*pbuf) {
        if (!pintron) {
            char *found = strstr(pbuf, "GUGU");

            if (found != NULL) {
                pintron = found;
                continue;
            } else {
                // copy the rest of the string
                memcpy(pdstbuf, pbuf, strlen(pbuf));
                pdstbuf += strlen(pbuf);
                break;
            }
        } else {
            char *found = strstr(pintron, "AGAG");

            if (found != NULL) {
                // copy the characters before the sequence
                memcpy(pdstbuf, pbuf, pintron-pbuf);
                // update the dstbuf
                pdstbuf += (pintron-pbuf);
                // update pbuf
                pbuf = found+4;
                // null the pointer
                pintron = NULL;
            } else {
                // copy the rest of the string
                memcpy(pdstbuf, pintron, strlen(pintron));
                pdstbuf += strlen(pintron);
                break;
            }
        }
    }

    *pdstbuf = '\0';

    printf("\nOutput is %s\n", dstbuf);

    return 0;
}