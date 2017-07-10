#include <stdio.h>

int main(int argc, char **argv) {
    if (argc != 2)
        return 1;

    printf("Hello, %s\n", argv[1]);
}