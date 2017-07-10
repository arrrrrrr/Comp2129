#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>

#define BUF_SIZE 1024

int main(void) {
    printf("Input strand: ");

    char buf[BUF_SIZE], c;
    int read = 0;

    while ((c = getchar()) != EOF) {
        if (c == '\n')
            break;

        if (read < BUF_SIZE)
            buf[read++] = c;
    }

    if (c == EOF)
        printf("\n");

    buf[read] = '\0';

    char dstbuf[BUF_SIZE];
    char *pbuf = buf, *pdstbuf = dstbuf, *pintron = NULL;

    while (*pbuf) {
        if (!pintron) {
            char *found = strstr(pbuf, "GUGU");

            if (found != NULL) {
                pintron = found;
                continue;
            } else {
                // copy the rest of the string
                memcpy(pdstbuf, pbuf, strlen(pbuf));
                pdstbuf += strlen(pbuf);
                break;
            }
        } else {
            char *found = strstr(pintron, "AGAG");

            if (found != NULL) {
                // copy the characters before the sequence
                memcpy(pdstbuf, pbuf, pintron-pbuf);
                // update the dstbuf
                pdstbuf += (pintron-pbuf);
                // update pbuf
                pbuf = found+4;
                // null the pointer
                pintron = NULL;
            } else {
                // copy the rest of the string
                memcpy(pdstbuf, pintron, strlen(pintron));
                pdstbuf += strlen(pintron);
                break;
            }
        }
    }

    *pdstbuf = '\0';

    printf("\nOutput is %s\n", dstbuf);

    return 0;
}