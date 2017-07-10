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