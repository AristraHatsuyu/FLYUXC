#include "flyuxc/frontend/normalize.h"
#include <stdio.h>
#include <stdlib.h>

int main() {
    const char *source = "123 := 456";
    printf("Original: %s\n", source);
    
    NormalizeResult result = flyux_normalize(source);
    if (result.error_code == 0) {
        printf("Normalized: %s\n", result.normalized);
        normalize_result_free(&result);
    } else {
        printf("Error: %s\n", result.error_msg);
    }
    
    return 0;
}
