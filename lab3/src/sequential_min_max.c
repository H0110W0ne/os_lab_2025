#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>

#include "find_min_max.h"
#include "utils.h"

int main(int argc, char **argv) {
  if (argc != 3) {
    fprintf(stderr, "Usage: %s seed arraysize\n", argv[0]);
    return 1;
  }

  int values[2];
  for (int i = 0; i < 2; i++) {
    char *end;
    errno = 0;
    long value = strtol(argv[i + 1], &end, 10);
    if (errno || end == argv[i + 1] || *end != '\0' ||
        value <= 0 || value > INT_MAX) {
      fprintf(stderr, "seed and arraysize must be integers from 1 to INT_MAX\n");
      return 1;
    }
    values[i] = (int)value;
  }

  int seed = values[0];
  int array_size = values[1];
  int *array = calloc((size_t)array_size, sizeof(int));
  if (array == NULL) {
    perror("calloc");
    return 1;
  }

  GenerateArray(array, array_size, seed);
  struct MinMax min_max = GetMinMax(array, 0, array_size);
  free(array);

  printf("min: %d\n", min_max.min);
  printf("max: %d\n", min_max.max);
  return 0;
}
