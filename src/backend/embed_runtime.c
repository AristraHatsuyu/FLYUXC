/**
 * 嵌入的运行时库源代码
 * 将在编译时自动包含到编译器中
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

const char* get_embedded_runtime_source(void) {
    return
#include "runtime_embedded.h"
    ;
}
