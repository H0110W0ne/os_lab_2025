#include <stdio.h>
#include "swap.h"

int main(void)
{
    char ch1 = 'a';
    char ch2 = 'b';

    // Передаём адреса символов, чтобы Swap изменила их значения.
    Swap(&ch1, &ch2);

    printf("%c %c\n", ch1, ch2);
    return 0;
}