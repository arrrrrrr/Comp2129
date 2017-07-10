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