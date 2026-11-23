#define _POSIX_C_SOURCE 200809L
#include <errno.h>
#include <getopt.h>
#include <limits.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#include "find_min_max.h"
#include "utils.h"

/* Флаг можно безопасно менять из обработчика сигнала. */
static volatile sig_atomic_t timed_out = 0;

static void OnAlarm(int signal_number) {
    (void)signal_number;
    timed_out = 1; /* В обработчике только устанавливаем флаг. */
}

static int ReadNumber(const char *text) {
    char *end;
    errno = 0;
    long value = strtol(text, &end, 10);
    if (errno || *text == '\0' || *end != '\0' ||
        value < 0 || value > INT_MAX)
        return -1;
    return (int)value;
}

struct Child {
    pid_t pid;
    int fd;
    FILE *file;
};

int main(int argc, char **argv) {
    int seed = -1, array_size = -1, pnum = -1;
    int timeout = 0, by_files = 0;
    const struct option options[] = {
        {"seed", required_argument, NULL, 's'},
        {"array_size", required_argument, NULL, 'a'},
        {"pnum", required_argument, NULL, 'p'},
        {"by_files", no_argument, NULL, 'f'},
        {"timeout", required_argument, NULL, 't'},
        {NULL, 0, NULL, 0}
    };
    int option;
    while ((option = getopt_long(argc, argv, "f", options, NULL)) != -1) {
        if (option == 'f') {
            by_files = 1;
            continue;
        }
        int value = optarg ? ReadNumber(optarg) : -1;
        if (value < 0) goto usage;
        switch (option) {
            case 's': seed = value; break;
            case 'a': array_size = value; break;
            case 'p': pnum = value; break;
            case 't':
                if (value == 0) goto usage;
                timeout = value;
                break;
            default: goto usage;
        }
    }
    if (optind != argc || seed < 0 || array_size <= 0 || pnum <= 0)
        goto usage;

    int *array = calloc((size_t)array_size, sizeof(*array));
    struct Child *children = calloc((size_t)pnum, sizeof(*children));
    if (!array || !children) {
        perror("calloc");
        free(array); free(children);
        return 1;
    }
    for (int i = 0; i < pnum; ++i) children[i].fd = -1;
    GenerateArray(array, (unsigned int)array_size, (unsigned int)seed);

    int result = 0, created = 0;
    struct timespec start, finish;
    clock_gettime(CLOCK_MONOTONIC, &start);
    if (timeout > 0) {
        if (signal(SIGALRM, OnAlarm) == SIG_ERR) {
            perror("signal");
            result = 1;
            goto cleanup;
        }
        alarm((unsigned int)timeout);
    }

    for (int i = 0; i < pnum && !timed_out; ++i) {
        int channel[2] = {-1, -1};
        if (by_files) {
            /* Файл создаётся до fork: он доступен родителю и ребёнку. */
            children[i].file = tmpfile();
            if (!children[i].file) {
                perror("tmpfile"); result = 1; break;
            }
        } else if (pipe(channel) == -1) {
            perror("pipe"); result = 1; break;
        }

        pid_t pid = fork();
        if (pid == -1) {
            perror("fork");
            if (!by_files) { close(channel[0]); close(channel[1]); }
            result = 1;
            break;
        }
        if (pid == 0) {
            if (!by_files) close(channel[0]);
            /* Делим [0, array_size) без пропусков, даже при наличии остатка. */
            unsigned int begin = (uint64_t)i * array_size / pnum;
            unsigned int end = (uint64_t)(i + 1) * array_size / pnum;
            struct MinMax part = GetMinMax(array, begin, end);
            int ok;
            if (by_files) {
                ok = fwrite(&part, sizeof(part), 1, children[i].file) == 1;
                if (fflush(children[i].file) != 0) ok = 0;
            } else {
                ssize_t written;
                do {
                    written = write(channel[1], &part, sizeof(part));
                } while (written < 0 && errno == EINTR);
                ok = written == (ssize_t)sizeof(part);
                close(channel[1]);
            }
            _exit(ok ? 0 : 1); /* Не продолжаем родительскую часть программы. */
        }
        children[i].pid = pid;
        ++created;
        if (!by_files) {
            close(channel[1]);
            children[i].fd = channel[0];
        }
    }

    int remaining = created;
    while (remaining > 0 && !result && !timed_out) {
        for (int i = 0; i < created; ++i) {
            if (children[i].pid == 0) continue;
            int status;
            /* WNOHANG возвращает 0, если ребёнок ещё работает. */
            pid_t done = waitpid(children[i].pid, &status, WNOHANG);
            if (done > 0) {
                children[i].pid = 0;
                --remaining;
                if (!WIFEXITED(status) || WEXITSTATUS(status) != 0) {
                    fprintf(stderr, "Child failed\n");
                    result = 1;
                    break;
                }
            } else if (done < 0 && errno != EINTR) {
                perror("waitpid");
                result = 1;
                break;
            }
        }
        /* Не расходуем целое ядро на опрос waitpid. */
        if (remaining > 0 && !result && !timed_out) {
            struct timespec pause_time = {0, 1000000};
            nanosleep(&pause_time, NULL);
        }
    }
    alarm(0);
    if (timed_out) {
        fprintf(stderr, "Timeout: sending SIGKILL to unfinished children.\n");
        result = 124;
    }
    if (result) goto cleanup;

    struct MinMax total = {INT_MAX, INT_MIN};
    for (int i = 0; i < created; ++i) {
        struct MinMax part;
        int ok;
        if (by_files) {
            ok = fseek(children[i].file, 0, SEEK_SET) == 0;
            if (ok) ok = fread(&part, sizeof(part), 1, children[i].file) == 1;
        } else {
            ssize_t received;
            do {
                received = read(children[i].fd, &part, sizeof(part));
            } while (received < 0 && errno == EINTR);
            ok = received == (ssize_t)sizeof(part);
        }
        if (!ok) {
            fprintf(stderr, "Cannot read child result\n");
            result = 1;
            goto cleanup;
        }
        if (part.min < total.min) total.min = part.min;
        if (part.max > total.max) total.max = part.max;
    }
    clock_gettime(CLOCK_MONOTONIC, &finish);
    double elapsed = (finish.tv_sec - start.tv_sec) * 1000.0
                   + (finish.tv_nsec - start.tv_nsec) / 1000000.0;
    printf("Min: %d\nMax: %d\nElapsed time: %fms\n",
           total.min, total.max, elapsed);

cleanup:
    alarm(0);
    /* Сначала завершаем всех оставшихся детей, затем собираем их статусы. */
    for (int i = 0; i < created; ++i) {
        if (children[i].pid > 0 &&
            kill(children[i].pid, SIGKILL) == -1 && errno != ESRCH)
            perror("kill");
    }
    for (int i = 0; i < created; ++i) {
        if (children[i].pid > 0) {
            while (waitpid(children[i].pid, NULL, 0) == -1) {
                if (errno != EINTR) { perror("waitpid"); break; }
            }
        }
    }
    for (int i = 0; i < pnum; ++i) {
        if (children[i].fd >= 0) close(children[i].fd);
        if (children[i].file) fclose(children[i].file);
    }
    free(children); free(array);
    return result;

usage:
    fprintf(stderr, "Usage: %s --seed N --array_size N --pnum N "
                    "[--by_files] [--timeout N]\n", argv[0]);
    return 1;
}
