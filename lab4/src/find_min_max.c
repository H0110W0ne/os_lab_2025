#include "find_min_max.h"
#include <limits.h>

struct MinMax GetMinMax(int *array, unsigned int begin, unsigned int end) {
    struct MinMax result = {INT_MAX, INT_MIN};
    /* Полуинтервал: begin включён, end не включён. */
    for (unsigned int i = begin; i < end; ++i) {
        if (array[i] < result.min) result.min = array[i];
        if (array[i] > result.max) result.max = array[i];
    }
    return result;
}
