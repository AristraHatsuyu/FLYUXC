#include "flyuxc/io.h"
#include <stdio.h>
#include <stdlib.h>

int print_file_to_stdout(const char *path) {
    if (!path) {
        fprintf(stderr, "No path provided\n");
        return 1;
    }

    FILE *f = fopen(path, "rb");
    if (!f) {
        perror(path);
        return 1;
    }

    char buf[4096];
    size_t n;
    while ((n = fread(buf, 1, sizeof(buf), f)) > 0) {
        size_t written = fwrite(buf, 1, n, stdout);
        if (written != n) {
            perror("write to stdout");
            fclose(f);
            return 1;
        }
    }

    if (ferror(f)) {
        perror("read file");
        fclose(f);
        return 1;
    }

    fclose(f);
    return 0;
}
