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