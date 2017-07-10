#include "oddeven.h"

void* thread_even(void* arg) {
    thrdata_t *data = (thrdata_t *)arg;
    atomic_int *actr = data->actr;

    while (1) {
        if (*actr > data->value)
            break;

        if (*actr % 2 == 0 && *actr <= data->value) {
            printf("%d\n", *actr);
            *actr += 1;
        }
    }

    return NULL;
}

void* thread_odd(void* arg) {
    thrdata_t *data = (thrdata_t *)arg;
    atomic_int *actr = data->actr;

    while (1) {
        if (*actr > data->value)
            break;

        if (*actr % 2 == 1 && *actr <= data->value) {
            printf("%d\n", *actr);
            *actr += 1;
        }
    }


    return NULL;
}