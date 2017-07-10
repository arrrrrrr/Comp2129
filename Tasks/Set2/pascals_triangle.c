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
#include <stdint.h>
#include <math.h>

long double choose(int x, int y) {
    return (tgammal(x+1) / (tgammal(y+1)*tgammal(x-y+1)));
}

int main(int argc, char **argv) {
    int value = 0;

    if (argc == 1) {
        printf("Missing Argument\n");
        return 1;
    }

    if (argc != 2) {
        printf("Invalid Argument\n");
        return 1;
    }

    if ((sscanf(argv[1],"%d",&value) != 1)) {
        printf("Missing Argument\n");
        return 1;
    }

    if (value < 0) {
        printf("Invalid Argument\n");
        return 1;
    }

    for (int i = 0; i < value+1; i++) {
        for (int j = 0; j < i+1; j++)
            printf("%s%.Lf",(j == 0) ? "" : " ",choose(i,j));
        printf("\n");
    }
}