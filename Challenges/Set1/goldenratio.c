#include <stdio.h>
#include <math.h>
#include <string.h>

float fround(float x, int dp);

float fround(float x, int dp) {
    float y = powf(10, (float)dp);
    // add 5 * 10^-(dp-1) to round towards the centre
    x += (5 * powf(10, (float)(-(dp+1))));
    return floorf(x * y)/y;
}

int main(void) {
    float a = 0.f, b = 0.f;

    printf("Enter two numbers: ");

    int read = scanf("%f %f", &a, &b);

    printf("\n");

    if (read != 2) {
        printf("Invalid input.\n");
        return 1;
    }

    if (b > a) {
        float tmp = a;
        a = b;
        b = tmp;
    }

    if (fround((a + b)/a, 3) == 1.618f && fround(a/b, 3) == 1.618f)
        printf("Golden ratio!\n");
    else
        printf("Maybe next time.\n");

}

