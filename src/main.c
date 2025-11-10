#include "mylib.h"
#include <stdio.h>

int main() {
    printf("Hello, CMake Project!\n");
    int result = add(5, 3);
    printf("5 + 3 = %d\n", result);
    return 0;
}