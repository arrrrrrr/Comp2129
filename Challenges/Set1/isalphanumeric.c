#include <stdio.h>

int main(void) {
    int value = 0;

    int read = scanf("%d", &value);

    if (read != 1)
        return 1;

    printf("Is %d\nAlpha-Numeric?\n", value);

    if ((value >= 0x0 && value <= 0x7f) &&
        ((value >= 0x30 && value <= 0x39) ||
         (value >= 0x41 && value <= 0x5a) ||
         (value >= 0x61 && value <= 0x7a)))
        printf("Yes, it is '%c'\n",(char)value);

}