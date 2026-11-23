#ifndef SUM_H
#define SUM_H

#include <stdint.h>

struct SumArgs {
    int *array;
    unsigned int begin;
    unsigned int end;
    int64_t result;
};

int64_t Sum(const struct SumArgs *args);

#endif
