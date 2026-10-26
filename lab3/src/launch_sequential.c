#include <errno.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

int main(int argc, char **argv) {
  if (argc != 3) {
    fprintf(stderr, "Usage: %s seed arraysize\n", argv[0]);
    return 1;
  }

  pid_t pid = fork();
  if (pid == -1) {
    perror("fork");
    return 1;
  }
  if (pid == 0) {
    // exec заменяет программу ребёнка, сохраняя его PID.
    execl("./sequential_min_max", "sequential_min_max",
          argv[1], argv[2], (char *)NULL);
    perror("execl"); // Сюда попадём только при ошибке exec.
    _exit(1);
  }

  int status;
  pid_t result;
  do {
    result = waitpid(pid, &status, 0);
  } while (result == -1 && errno == EINTR);
  if (result == -1) {
    perror("waitpid");
    return 1;
  }
  if (WIFEXITED(status)) return WEXITSTATUS(status);
  return 1;
}
