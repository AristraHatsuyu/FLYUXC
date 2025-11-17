#include "flyuxc/utils/io.h"
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

char* read_file_to_string(const char *path) {
    if (!path) {
        fprintf(stderr, "No path provided\n");
        return NULL;
    }

    FILE *f = fopen(path, "rb");
    if (!f) {
        perror(path);
        return NULL;
    }

    // 获取文件大小
    fseek(f, 0, SEEK_END);
    long file_size = ftell(f);
    fseek(f, 0, SEEK_SET);

    if (file_size < 0) {
        perror("ftell");
        fclose(f);
        return NULL;
    }

    // 分配内存
    char *content = malloc(file_size + 1);
    if (!content) {
        fprintf(stderr, "Memory allocation failed\n");
        fclose(f);
        return NULL;
    }

    // 读取内容
    size_t bytes_read = fread(content, 1, file_size, f);
    if (bytes_read != file_size) {
        perror("fread");
        free(content);
        fclose(f);
        return NULL;
    }

    content[file_size] = '\0';
    fclose(f);
    return content;
}
