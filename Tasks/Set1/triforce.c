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
#include <stdlib.h>

int main(int argc, char **argv) {
    int height = 0, ret = 0, width = 0;

    printf("Enter height: ");

    if ((ret = scanf("%d", &height)) != 1) {
        printf("\nInvalid height.\n");
        return 1;
    }

    if (height < 2 || height > 20) {
        printf("\nInvalid height.\n");
        return 1;
    }

    width = height*2;

    //char plane[height*2][width*2+1];
    char **plane;
    int plane_w = width*2+1, plane_h = height*2;

    // allocate plane_h pointers to char arrays
    plane = malloc(sizeof(*plane)*plane_h);

    // allocate plane_h char arrays
    for (int i = 0; i < plane_h; i++)
        plane[i] = malloc(sizeof(char)*plane_w);

    // initialize the plane with spaces
    for (int i = 0; i < plane_h; i++)
        memset(plane[i], ' ', plane_w);

    int tri_x[3] = { height, 0, width };
    int tri_y[3] = { 0, height, height };

    for ( int tri = 0; tri < 3; tri++ ) {
        for ( int pos = tri_x[tri], rel = 0; rel <= width; pos++, rel++ ) {
            for ( int row = tri_y[tri], i = 0; row < (tri_y[tri]+height); row++, i++ ) {
                if ( i == height - 1 ) {
                    plane[row][pos] = (rel == 0) ? '/' : (rel == width-1) ? '\\' : '_';
                    if ( rel == width && tri != 1)
                        plane[row][pos] = '\0'; // null terminate the right side
                } else {
                    if ( rel >= (width/2)-1-i && rel <= (width/2)+i )
                        plane[row][pos] = (rel == (width/2)-1-i) ? '/' : (rel == (width/2)+i) ? '\\' : ' ';
                    else if ( rel > width/2+i && tri == 1 )
                        plane[row][pos] = ' ';
                    else if ( rel == (width/2)+i+1 && tri != 1)
                        plane[row][pos] = '\0'; // null termninate the right side
                }
            }
        }
    }

    printf("\n");

    for ( int pos = 0; pos < plane_h; pos++ ) {
        printf("%s\n", plane[pos]);
    }

    // matching free for plane_h char arrays
    for ( int pos = 0; pos < plane_h; pos++ )
        free(plane[pos]);

    // matching free for plane_h pointers to char arrays
    free(plane);
}