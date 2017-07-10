#include <stdio.h>
#include <string.h>
#include <ctype.h>

#define BUF_SIZE 1024

int main(int argc, char **argv)
{
    printf("Enter strand: ");

    char buf[BUF_SIZE] = {0}, c;
    int read = 0;

    while ((c = getchar()) != EOF) {
        if (c == '\n')
            break;

        if (read < BUF_SIZE)
            buf[read++] = c;
    }

    if (c == EOF)
        printf("\n");

    printf("\n");

    if (strlen(buf) == 0) {
        printf("No strand provided.\n");
        return 1;
    }

    char *pbuf = buf;

    while (*pbuf) {
        switch (toupper(*pbuf)) {
            case 'A': *pbuf = *pbuf + ('T'-'A'); break;
            case 'T': *pbuf = *pbuf - ('T'-'A'); break;
            case 'G': *pbuf = *pbuf - ('G'-'C'); break;
            case 'C': *pbuf = *pbuf + ('G'-'C'); break;
            default:  *pbuf = 'x'; break;
        }
        pbuf++;
    }

    printf("Complementary strand is %s\n", buf);

    return 0;
}