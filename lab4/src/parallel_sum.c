#define _POSIX_C_SOURCE 200809L
#include <errno.h>
#include <getopt.h>
#include <inttypes.h>
#include <limits.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "sum.h"
#include "utils.h"

static int ReadNumber(const char *text) {
    char *end;
    errno = 0;
    long value = strtol(text, &end, 10);
    if (errno || *text == '\0' || *end != '\0' ||
        value < 0 || value > INT_MAX)
        return -1;
    return (int)value;
}

static void *ThreadSum(void *data) {
    struct SumArgs *args = data;
    args->result = Sum(args); /* У каждого потока свой результат. */
    return NULL;
}

int main(int argc, char **argv) {
    int threads_num = -1, seed = -1, array_size = -1;
    const struct option options[] = {
        {"threads_num", required_argument, NULL, 't'},
        {"seed", required_argument, NULL, 's'},
        {"array_size", required_argument, NULL, 'a'},
        {NULL, 0, NULL, 0}
    };
    int option;
    while ((option = getopt_long(argc, argv, "", options, NULL)) != -1) {
        int value = optarg ? ReadNumber(optarg) : -1;
        if (value < 0) goto usage;
        switch (option) {
            case 't': threads_num = value; break;
            case 's': seed = value; break;
            case 'a': array_size = value; break;
            default: goto usage;
        }
    }
    if (optind != argc || threads_num <= 0 || array_size <= 0 || seed < 0)
        goto usage;

    int *array = calloc((size_t)array_size, sizeof(*array));
    pthread_t *threads = calloc((size_t)threads_num, sizeof(*threads));
    struct SumArgs *args = calloc((size_t)threads_num, sizeof(*args));
    if (!array || !threads || !args) {
        perror("calloc");
        free(array); free(threads); free(args);
        return 1;
    }
    GenerateArray(array, (unsigned int)array_size, (unsigned int)seed);
    for (int i = 0; i < threads_num; ++i) {
        args[i].array = array;
        /* uint64_t предотвращает переполнение при умножении индексов. */
        args[i].begin = (uint64_t)i * array_size / threads_num;
        args[i].end = (uint64_t)(i + 1) * array_size / threads_num;
    }

    struct timespec start, finish;
    clock_gettime(CLOCK_MONOTONIC, &start); /* Генерация уже завершена. */
    int created = 0, failed = 0;
    for (int i = 0; i < threads_num; ++i) {
        /* Передаём каждому потоку адрес его собственной структуры. */
        int error = pthread_create(&threads[i], NULL, ThreadSum, &args[i]);
        if (error) {
            fprintf(stderr, "pthread_create: %s\n", strerror(error));
            failed = 1;
            break;
        }
        ++created;
    }
    int64_t total = 0;
    for (int i = 0; i < created; ++i) {
        int error = pthread_join(threads[i], NULL);
        if (error) {
            fprintf(stderr, "pthread_join: %s\n", strerror(error));
            exit(EXIT_FAILURE);
        }
        total += args[i].result; /* После join результат потока уже готов. */
    }
    clock_gettime(CLOCK_MONOTONIC, &finish);
    double elapsed = (finish.tv_sec - start.tv_sec) * 1000.0
                   + (finish.tv_nsec - start.tv_nsec) / 1000000.0;
    free(args); free(threads); free(array);
    if (failed) return 1;
    printf("Total: %" PRId64 "\nElapsed time: %fms\n", total, elapsed);
    return 0;

usage:
    fprintf(stderr, "Usage: %s --threads_num N --seed N --array_size N\n",
            argv[0]);
    return 1;
}
