#include <string.h>
#include "revert_string.h"
void RevertString(char *str)
{
size_t length = strlen(str); // Длина без завершающего '\0'.
// Меняем симметричные символы; середину и '\0' не трогаем.
for (size_t i = 0; i < length / 2; ++i)
{
char temp = str[i];
str[i] = str[length - 1 - i];
str[length - 1 - i] = temp;
}
}