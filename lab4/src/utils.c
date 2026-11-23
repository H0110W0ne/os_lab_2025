#include "utils.h"
#include <stdlib.h>

/* Тот же генератор, что в лабораторной №3. */
void GenerateArray(int *array, unsigned int size, unsigned int seed) {
    srand(seed);
    for (unsigned int i = 0; i < size; ++i)
        array[i] = rand();
}
