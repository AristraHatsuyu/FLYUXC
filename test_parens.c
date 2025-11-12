#include <stdio.h>
#include <string.h>
#include <stdlib.h>

char* simplify_parens_test(const char* expr);

int main() {
    const char* tests[] = {
        "((1+2))",
        "(((x)))",
        "(x)",
        "((x))",
        "print(x,y,z)",
    };
    
    for (int i = 0; i < 5; i++) {
        char* result = simplify_parens_test(tests[i]);
        printf("'%s' -> '%s'\n", tests[i], result);
        free(result);
    }
    
    return 0;
}
