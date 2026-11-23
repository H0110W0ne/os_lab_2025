#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define SHW_ADR(ID, I) \
    printf("ID %s\t is at virtual address: %p\n", ID, (void *)&(I))

/* Символы компоновщика: используются их адреса, а не значения. */
extern char etext, edata, end;

char *cptr = "This message is output by the function showit()\n";
char buffer1[25];
void showit(const char *p);

int main(void) {
    int i = 0;
    printf("Address etext: %p\n", (void *)&etext);
    printf("Address edata: %p\n", (void *)&edata);
    printf("Address end  : %p\n", (void *)&end);
    SHW_ADR("main", main);
    SHW_ADR("showit", showit);
    SHW_ADR("cptr", cptr);
    SHW_ADR("buffer1", buffer1);
    SHW_ADR("i", i);
    strcpy(buffer1, "A demonstration\n");
    fflush(stdout); /* Сохраняем порядок printf и write. */
    if (write(STDOUT_FILENO, buffer1, strlen(buffer1)) == -1) {
        perror("write");
        return 1;
    }
    showit(cptr);
    return 0;
}

void showit(const char *p) {
    char *buffer2;
    SHW_ADR("buffer2", buffer2); /* Адрес самого указателя: стек. */
    buffer2 = malloc(strlen(p) + 1);
    if (!buffer2) {
        perror("malloc");
        exit(EXIT_FAILURE);
    }
    printf("Allocated memory at %p\n", (void *)buffer2);
    strcpy(buffer2, p);
    printf("%s", buffer2);
    free(buffer2);
}
