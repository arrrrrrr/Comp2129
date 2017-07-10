#include <stdio.h>
#include <string.h>

#define BUF_SIZE 1024

int main(void) {
    printf("Enter binary: ");

    char buf[BUF_SIZE] = {0};

    if (!scanf("%s", buf)) {
        printf("\nNot binary!\n");
        return 1;
    }

    char *pbuf = buf;

    while (*pbuf) {
        if (*pbuf != '0' && *pbuf != '1') {
            printf("\nNot binary!\n");
            return 1;
        }
        pbuf++;
    }

    unsigned int result = 0;

    for (int i = strlen(buf)-1, j = 0; i >= 0; i--, j++)
        result += ((int)(buf[i] - '0') << j);

    printf("\n%d in decimal\n", result);
    return 0;
}