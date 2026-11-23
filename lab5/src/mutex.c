/* Based on the course's mutex.c example, adapted there by A. Kostin
 * from Using POSIX Threads by Brad Nichols, Dick Buttlar, and Jackie Farrell.
 */
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int counter = 0;
#ifdef USE_MUTEX
static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
#endif

static void check_thread(int status, const char *operation) {
    if (status != 0) {
        fprintf(stderr, "%s: %s\n", operation, strerror(status));
        exit(EXIT_FAILURE);
    }
}

static void *increment(void *argument) {
    const char *message = argument;

    for (int i = 0; i < 50; ++i) {
#ifdef USE_MUTEX
        check_thread(pthread_mutex_lock(&mutex), "pthread_mutex_lock");
#endif
        puts(message);
        int value = counter;
        printf("counter = %d\n", value);
        ++value;

        // Задержка увеличивает интервал между чтением и записью счётчика.
        for (volatile unsigned long delay = 0; delay < 500000; ++delay) {
        }
        counter = value;
#ifdef USE_MUTEX
        check_thread(pthread_mutex_unlock(&mutex), "pthread_mutex_unlock");
#endif
    }
    return NULL;
}

int main(void) {
    pthread_t first, second;

    check_thread(pthread_create(&first, NULL, increment, "doing one thing"),
                 "pthread_create");
    check_thread(pthread_create(&second, NULL, increment, "doing another thing"),
                 "pthread_create");
    check_thread(pthread_join(first, NULL), "pthread_join");
    check_thread(pthread_join(second, NULL), "pthread_join");

    printf("All done, counter = %d\n", counter);
    return EXIT_SUCCESS;
}
