#ifndef FLYUXC_IO_H
#define FLYUXC_IO_H

#include <stddef.h>

// 打印文件到 stdout，成功返回 0，失败返回非 0
int print_file_to_stdout(const char *path);

// 读取文件内容到字符串，返回字符串指针（需要 free），失败返回 NULL
char* read_file_to_string(const char *path);

#endif // FLYUXC_IO_H
