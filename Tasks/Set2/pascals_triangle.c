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