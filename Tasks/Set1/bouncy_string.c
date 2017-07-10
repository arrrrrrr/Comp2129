#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum dir_t { BACK, FORWARD };

int main(int argc, char **argv) {
    int start = -1, n = -1, len = 0, pos = 0;
    enum dir_t bounce = FORWARD;
    char *pEnd;

    if (argc != 4 ) {
        printf("Invalid arguments length\n");
        return 1;
    }

    len = strlen(argv[1]);
    long tmpl = strtol(argv[2],&pEnd,10);

    if (*pEnd != '\0') {
        tmpl = -1L;
    }

    start = (int)tmpl;

    tmpl = strtol(argv[3],&pEnd,10);

    if (*pEnd != '\0') {
        tmpl = -1L;
    }

    n = (int)tmpl;

    if (start < 0 || start > (len-1)) {
        printf("Invalid start position\n");
        return 1;
    }

    if (n < 0) {
        printf("Invalid iteration count\n");
        return 1;
    }

    char *tmp, *pstr = NULL;
    // allocate for n+1 + null terminator
    char *buf = malloc(sizeof(char)*(n+2));

    pstr = argv[1];
    tmp = pstr+start;

    if (len == 1) {
        while(pos <= n) {
            buf[pos++] = *tmp;
        }
    } else {

        while(pos <= n) {
            buf[pos++] = *tmp;

            bounce = (tmp-pstr+1) == len ? BACK :
                ((tmp == pstr) ? FORWARD : bounce);

            switch (bounce) {
                case FORWARD: tmp++; break;
                case BACK: tmp--; break;
            }
        }
    }

    buf[pos] = '\0';

    //printf("len = %d, start = %d, n = %d, pos = %d, bufpos = %d, buflen = %d\n", len, start, n, tmp-pstr, pos, strlen(buf));
    printf("%s\n", buf);

    free(buf);

    return 0;
}