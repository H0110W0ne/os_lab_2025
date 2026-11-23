#include "sum.h"

int64_t Sum(const struct SumArgs *args) {
    int64_t sum = 0; /* Сумма нескольких rand() может не поместиться в int. */
    for (unsigned int i = args->begin; i < args->end; ++i)
        sum += args->array[i];
    return sum;
}
