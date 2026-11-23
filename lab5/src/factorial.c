#include <errno.h>
#include <getopt.h>
#include <inttypes.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct Task {
    pthread_t thread;
    uint64_t offset;
    uint64_t count;
};

static uint64_t modulus;
static uint64_t result;
static pthread_mutex_t result_mutex = PTHREAD_MUTEX_INITIALIZER;

static int parse_number(const char *text, uint64_t *number) {
    if (text == NULL || text[0] < '0' || text[0] > '9') {
        return 0;
    }

    errno = 0;
    char *end;
    uintmax_t value = strtoumax(text, &end, 10);
    if (errno == ERANGE || *end != '\0' || value > UINT64_MAX) {
        return 0;
    }

    *number = (uint64_t)value;
    return 1;
}

static void check_thread(int status, const char *operation) {
    if (status != 0) {
        fprintf(stderr, "%s: %s\n", operation, strerror(status));
        exit(EXIT_FAILURE);
    }
}

static uint64_t multiply_mod(uint64_t left, uint64_t right) {
    // 128 бит позволяют сохранить произведение двух 64-битных чисел.
    return (uint64_t)((__uint128_t)left * right % modulus);
}

static void *multiply_range(void *argument) {
    const struct Task *task = argument;
    uint64_t partial = 1 % modulus;

    for (uint64_t i = 0; i < task->count; ++i) {
        partial = multiply_mod(partial, task->offset + i + 1);
    }

    // Поток берёт мьютекс один раз, чтобы объединить результаты.
    check_thread(pthread_mutex_lock(&result_mutex), "pthread_mutex_lock");
    result = multiply_mod(result, partial);
    check_thread(pthread_mutex_unlock(&result_mutex), "pthread_mutex_unlock");
    return NULL;
}

int main(int argc, char *argv[]) {
    const struct option options[] = {
        {"pnum", required_argument, NULL, 'p'},
        {"mod", required_argument, NULL, 'm'},
        {0, 0, NULL, 0}
    };
    const char *k_arg = NULL, *pnum_arg = NULL, *mod_arg = NULL;
    int option;
    while ((option = getopt_long(argc, argv, "k:", options, NULL)) != -1) {
        switch (option) {
            case 'k': k_arg = optarg; break;
            case 'p': pnum_arg = optarg; break;
            case 'm': mod_arg = optarg; break;
            default: return EXIT_FAILURE;
        }
    }

    uint64_t k, pnum;
    if (optind != argc || !parse_number(k_arg, &k) ||
        !parse_number(pnum_arg, &pnum) || !parse_number(mod_arg, &modulus) ||
        pnum == 0 || modulus == 0 || pnum > SIZE_MAX / sizeof(struct Task)) {
        fprintf(stderr, "Usage: %s -k K --pnum=PNUM --mod=MOD\n", argv[0]);
        fprintf(stderr, "K >= 0, PNUM > 0, MOD > 0\n");
        return EXIT_FAILURE;
    }

    struct Task *tasks = calloc((size_t)pnum, sizeof *tasks);
    if (tasks == NULL) {
        perror("calloc");
        return EXIT_FAILURE;
    }

    result = 1 % modulus;
    uint64_t offset = 0;
    for (uint64_t i = 0; i < pnum; ++i) {
        tasks[i].offset = offset;
        tasks[i].count = k / pnum + (i < k % pnum);
        offset += tasks[i].count;
        check_thread(pthread_create(&tasks[i].thread, NULL, multiply_range,
                                    &tasks[i]), "pthread_create");
    }
    for (uint64_t i = 0; i < pnum; ++i) {
        check_thread(pthread_join(tasks[i].thread, NULL), "pthread_join");
    }

    printf("%" PRIu64 "! mod %" PRIu64 " = %" PRIu64 "\n", k, modulus, result);
    free(tasks);
    return EXIT_SUCCESS;
}
