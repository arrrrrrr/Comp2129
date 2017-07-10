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

/*
grid

+-+ +---+ +-----+
| | | | | | | | |
+-+ +---+ +-----+

width = (2*width-1)+2
height = height + 2
*/

typedef struct PLANE_DATA_T {
    char **plane;
    int rows;
    int cols;
    int width;
    int height;
    int ox;
    int oy;
    int status;
} PLANE_DATA, *PPLANE_DATA;

void create_plane(PPLANE_DATA ppd);
void free_plane(PPLANE_DATA ppd);
char get_from_plane(PPLANE_DATA ppd, int x, int y);

void draw(PPLANE_DATA ppd, char c, int x, int y);
void draw_column(PPLANE_DATA ppd, int ox, int oy, int dir);
void draw_row(PPLANE_DATA ppd, int ox, int oy, int dir);

int validate_coords(PPLANE_DATA ppd, int x, int y);
int x_to_plane_col(int x);
int y_to_plane_row(int y);

int main(int argc, char **argv) {
    // usage ./$0 width height x y limit
    if (argc != 6) {
        printf("Invalid Number Of Arguments\n");
        return 1;
    }

    long gridsize[] = { 0L, 0L };
    long charprop[] = { 0L, 0L, 0L };
    char *pEnd;

    PLANE_DATA  pds = {0};
    PPLANE_DATA ppd = &pds;

    for (int i = 1; i < 3; i++) {
        gridsize[i-1] = strtol(argv[i], &pEnd, 10);

        if (gridsize[i-1] < 1 || *pEnd != '\0') {
            printf("%s\n", (i == 1) ? "Invalid Width" : "Invalid Height");
            return 1;
        }
    }

    pds.width  = (int)gridsize[0];
    pds.height = (int)gridsize[1];
    pds.cols   = (pds.width*2)-1+2;
    pds.rows   = pds.height+2;

    for (int i = 3; i < argc; i++) {
        charprop[i-3] = strtol(argv[i], &pEnd, 10);

        if (*pEnd != '\0')
            charprop[i-3] = -1L;
    }

    if (charprop[0] < 0 || charprop[1] < 0 || charprop[0] >= pds.width ||
        charprop[1] >= pds.height || charprop[2] < 0 || charprop[2] > 10) {
        printf("Invalid Character Properties\n");
        return 1;
    }

    pds.ox   = (int)charprop[0];
    pds.oy   = (int)charprop[1];

    // create a 2d plane
    create_plane(ppd);

    // draw limit at origin
    draw(ppd, (int)charprop[2]+'0', pds.ox, pds.oy);
    // draw all values in the same column as the origin
    draw_column(ppd, pds.ox, pds.oy, -1);
    draw_column(ppd, pds.ox, pds.oy,  1);
    // draw all values in the rows along the origin column
    for (int i=0; i <= pds.height-1; i++) {
        draw_row(ppd, pds.ox, i, -1);
        draw_row(ppd, pds.ox, i,  1);
    }
    // replace origin with C
    draw(ppd, 'C', pds.ox, pds.oy);

    // print the 2d plane
    for (int i = 0; i < ppd->rows; i++)
        printf("%s\n",ppd->plane[i]);

    // free the 2d plane
    free_plane(ppd);
}

void draw(PPLANE_DATA ppd, char c, int x, int y) {
    if (validate_coords(ppd, x, y)) {
        ppd->plane[y_to_plane_row(y)][x_to_plane_col(x)] = c;
    }
}

void draw_column(PPLANE_DATA ppd, int ox, int oy, int dir) {
    if (get_from_plane(ppd, ox, oy) == ' ') {
        return;
    }

    int start = get_from_plane(ppd, ox, oy)-'0';

    for (int i = start, j=oy; i >= 0; i--, j += dir) {
        if (get_from_plane(ppd, ox, j) == ' ') {
            draw(ppd, i+'0', ox, j);
        }
    }
}

void draw_row(PPLANE_DATA ppd, int ox, int oy, int dir) {
    if (get_from_plane(ppd, ox, oy) == ' ') {
        return;
    }

    int start = get_from_plane(ppd, ox, oy)-'0';

    for (int i = start, j=ox; i >= 0; i--, j += dir) {
        if (get_from_plane(ppd, j, oy) == ' ') {
            draw(ppd, i+'0', j, oy);
        }
    }
}

char get_from_plane(PPLANE_DATA ppd, int x, int y) {
    if (validate_coords(ppd, x, y)) {
        return ppd->plane[y_to_plane_row(y)][x_to_plane_col(x)];
    }

    return '\0';
}

int validate_coords(PPLANE_DATA ppd, int x, int y) {
    if ((x_to_plane_col(x) < ppd->cols-1 && x_to_plane_col(x) >= 1) &&
        (y_to_plane_row(y) < ppd->rows-1 && y_to_plane_row(y) >= 1))
        return 1;

    return 0;
}

int x_to_plane_col(int x) { return x*2+1; }
int y_to_plane_row(int y) { return y+1; }

void create_plane(PPLANE_DATA ppd) {
    if (ppd->status == 1) {
        free_plane(ppd);
    }

    ppd->plane = NULL;
    ppd->plane = malloc(sizeof(*(ppd->plane))*ppd->rows);

    if (ppd->plane == NULL) {
        printf("Malloc failed\n");
        exit(1);
    }

    for (int i = 0; i < ppd->rows; i++) {
        ppd->plane[i] = malloc((sizeof(**(ppd->plane))*ppd->cols)+1);
        if (ppd->plane[i] == NULL) {
            printf("Malloc failed\n");
            exit(1);
        }

        // prepare grid for filling
        if (i == 0 || i == ppd->rows-1) {
            ppd->plane[i][0] = ppd->plane[i][ppd->cols-1] = '+';

            for (int j = 1; j < ppd->cols-1; j++)
                ppd->plane[i][j] = '-';
        } else {
            for (int j = 0; j < ppd->cols; j++)
                ppd->plane[i][j] = (j % 2 == 0) ? '|' : ' ';
        }

        ppd->plane[i][ppd->cols] = '\0';
    }

    ppd->status = 1;
}

void free_plane(PPLANE_DATA ppd) {
    for (int i = 0; i < ppd->rows; i++)
        free(ppd->plane[i]);

    free(ppd->plane);

    ppd->plane = NULL;
    ppd->status = 0;
}