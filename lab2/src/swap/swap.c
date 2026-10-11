#include "swap.h"
void Swap(char *left, char *right)
{
char temp = *left; // Сохраняем первый символ перед перезаписью.
*left = *right; // Меняем значение по переданному адресу.
*right = temp;
}