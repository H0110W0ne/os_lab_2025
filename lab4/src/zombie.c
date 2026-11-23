#define _POSIX_C_SOURCE 200809L
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>

int main(void) {
    pid_t child = fork();
    if (child == -1) {
        perror("fork");
        return 1;
    }
    if (child == 0)
        _exit(0); /* Родитель пока не вызвал waitpid: появится зомби. */

    printf("Parent PID: %ld\nChild PID: %ld\n", (long)getpid(), (long)child);
    printf("Run in another terminal:\n"
           "ps -o pid,ppid,stat,cmd -p %ld\n", (long)child);
    printf("Press Enter to call waitpid...\n");
    fflush(stdout);
    getchar();

    while (waitpid(child, NULL, 0) == -1) {
        if (errno != EINTR) {
            perror("waitpid");
            return 1;
        }
    }
    printf("Child reaped. Repeat the ps command: the child is gone.\n");
    return 0;
}
