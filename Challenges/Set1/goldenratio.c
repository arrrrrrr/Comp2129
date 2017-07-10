/*
 * Copyright (c) 2017 Michael Zammit
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

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

