#include <errno.h>
#include <getopt.h>
#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "find_min_max.h"
#include "utils.h"

int main(int argc, char **argv) {
  int values[3] = {-1, -1, -1}; // seed, array_size, pnum
  bool with_files = false;
  static struct option options[] = {
      {"seed", required_argument, NULL, 0},
      {"array_size", required_argument, NULL, 0},
      {"pnum", required_argument, NULL, 0},
      {"by_files", no_argument, NULL, 'f'},
      {NULL, 0, NULL, 0}};

  int c, option_index;
  while ((c = getopt_long(argc, argv, "f", options, &option_index)) != -1) {
    if (c == 'f') {
      with_files = true;
    } else if (c == 0) {
      char *end;
      errno = 0;
      long value = strtol(optarg, &end, 10);
      if (errno || end == optarg || *end != '\0' ||
          value <= 0 || value > INT_MAX) {
        fprintf(stderr, "--%s must be an integer from 1 to INT_MAX\n",
                options[option_index].name);
        return 1;
      }
      values[option_index] = (int)value;
    } else {
      return 1;
    }
  }
  if (optind != argc || values[0] == -1 || values[1] == -1 || values[2] == -1) {
    fprintf(stderr, "Usage: %s --seed N --array_size N --pnum N [--by_files]\n",
            argv[0]);
    return 1;
  }

  int seed = values[0], array_size = values[1], pnum = values[2];
  int *array = calloc((size_t)array_size, sizeof(int));
  pid_t *pids = calloc((size_t)pnum, sizeof(pid_t));
  FILE **files = NULL;
  int (*pipes)[2] = NULL;
  int prepared = 0, started = 0, failed = 0;
  if (with_files) files = calloc((size_t)pnum, sizeof(FILE *));
  else pipes = calloc((size_t)pnum, sizeof(*pipes));
  if (!array || !pids || (with_files ? files == NULL : pipes == NULL)) {
    perror("calloc");
    failed = 1;
    goto cleanup;
  }
  GenerateArray(array, array_size, seed);

  struct timeval start_time, finish_time;
  gettimeofday(&start_time, NULL);

  // Для каждого ребёнка создаём отдельный файл или канал до fork().
  for (int i = 0; i < pnum; i++) {
    if (with_files) {
      files[i] = tmpfile();
      if (files[i] == NULL) {
        perror("tmpfile");
        failed = 1;
        goto cleanup;
      }
    } else if (pipe(pipes[i]) == -1) {
      perror("pipe");
      failed = 1;
      goto cleanup;
    }
    prepared++;
  }

  for (int i = 0; i < pnum; i++) {
    pids[i] = fork();
    if (pids[i] == -1) {
      perror("fork");
      failed = 1;
      break;
    }
    if (pids[i] == 0) {
      for (int j = 0; j < pnum; j++) {
        if (with_files) {
          if (j != i) fclose(files[j]);
        } else {
          close(pipes[j][0]);
          if (j != i) close(pipes[j][1]);
        }
      }

      // Такая разбивка учитывает остаток; long long исключает переполнение.
      unsigned int begin = (long long)array_size * i / pnum;
      unsigned int end = (long long)array_size * (i + 1) / pnum;
      struct MinMax part = GetMinMax(array, begin, end);
      int ok;
      if (with_files) {
        ok = fprintf(files[i], "%d %d\n", part.min, part.max) > 0;
        if (fclose(files[i]) == EOF) ok = 0;
      } else {
        size_t sent = 0;
        while (sent < sizeof(part)) {
          ssize_t n = write(pipes[i][1], (char *)&part + sent,
                            sizeof(part) - sent);
          if (n == -1 && errno == EINTR) continue;
          if (n <= 0) break;
          sent += (size_t)n;
        }
        ok = sent == sizeof(part);
        close(pipes[i][1]);
      }
      // Ребёнок завершается здесь и не создаёт следующие процессы.
      _exit(ok ? 0 : 1);
    }
    started++;
  }

  if (!with_files) {
    for (int i = 0; i < prepared; i++) {
      close(pipes[i][1]);
      pipes[i][1] = -1;
    }
  }
  for (int i = 0; i < started; i++) {
    int status;
    pid_t result;
    do {
      result = waitpid(pids[i], &status, 0);
    } while (result == -1 && errno == EINTR);
    if (result == -1 || !WIFEXITED(status) || WEXITSTATUS(status) != 0) {
      fprintf(stderr, "Child process failed\n");
      failed = 1;
    }
  }
  if (failed) goto cleanup;

  struct MinMax min_max = {INT_MAX, INT_MIN};
  for (int i = 0; i < pnum; i++) {
    struct MinMax part;
    int ok;
    if (with_files) {
      // После waitpid данные записаны; возвращаемся к началу файла.
      rewind(files[i]);
      ok = fscanf(files[i], "%d %d", &part.min, &part.max) == 2;
    } else {
      size_t received = 0;
      while (received < sizeof(part)) {
        ssize_t n = read(pipes[i][0], (char *)&part + received,
                         sizeof(part) - received);
        if (n == -1 && errno == EINTR) continue;
        if (n <= 0) break;
        received += (size_t)n;
      }
      ok = received == sizeof(part);
    }
    if (!ok) {
      fprintf(stderr, "Cannot read child result\n");
      failed = 1;
      goto cleanup;
    }
    if (part.min < min_max.min) min_max.min = part.min;
    if (part.max > min_max.max) min_max.max = part.max;
  }

  gettimeofday(&finish_time, NULL);
  double elapsed_time = (finish_time.tv_sec - start_time.tv_sec) * 1000.0;
  elapsed_time += (finish_time.tv_usec - start_time.tv_usec) / 1000.0;
  printf("Min: %d\nMax: %d\nElapsed time: %fms\n",
         min_max.min, min_max.max, elapsed_time);

cleanup:
  for (int i = 0; i < prepared; i++) {
    if (with_files) fclose(files[i]);
    else {
      close(pipes[i][0]);
      if (pipes[i][1] != -1) close(pipes[i][1]);
    }
  }
  free(files);
  free(pipes);
  free(pids);
  free(array);
  return failed;
}
