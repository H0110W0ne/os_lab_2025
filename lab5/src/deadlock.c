#define _XOPEN_SOURCE 700
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static pthread_mutex_t mutexes[2] = {
    PTHREAD_MUTEX_INITIALIZER, PTHREAD_MUTEX_INITIALIZER
};
static pthread_barrier_t barrier;

static void check_thread(int status, const char *operation) {
    if (status != 0) {
        fprintf(stderr, "%s: %s\n", operation, strerror(status));
        exit(EXIT_FAILURE);
    }
}

static void *lock_both(void *argument) {
    const int first = *(const int *)argument;
    const int second = 1 - first;

    check_thread(pthread_mutex_lock(&mutexes[first]), "pthread_mutex_lock");
    printf("Thread %d locked mutex %d\n", first + 1, first + 1);

    // Оба потока должны захватить по мьютексу до второй попытки.
    int status = pthread_barrier_wait(&barrier);
    if (status != PTHREAD_BARRIER_SERIAL_THREAD) {
        check_thread(status, "pthread_barrier_wait");
    }

    printf("Thread %d waits for mutex %d\n", first + 1, second + 1);
    check_thread(pthread_mutex_lock(&mutexes[second]), "pthread_mutex_lock");

    check_thread(pthread_mutex_unlock(&mutexes[second]), "pthread_mutex_unlock");
    check_thread(pthread_mutex_unlock(&mutexes[first]), "pthread_mutex_unlock");
    return NULL;
}

int main(void) {
    pthread_t threads[2];
    int first_mutex[] = {0, 1};

    setvbuf(stdout, NULL, _IONBF, 0);
    check_thread(pthread_barrier_init(&barrier, NULL, 2), "pthread_barrier_init");
    for (int i = 0; i < 2; ++i) {
        check_thread(pthread_create(&threads[i], NULL, lock_both, &first_mutex[i]),
                     "pthread_create");
    }
    for (int i = 0; i < 2; ++i) {
        check_thread(pthread_join(threads[i], NULL), "pthread_join");
    }

    check_thread(pthread_barrier_destroy(&barrier), "pthread_barrier_destroy");
    return EXIT_SUCCESS;
}
