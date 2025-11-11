#ifndef FLYUXC_IO_H
#define FLYUXC_IO_H

#include <stddef.h>

// 打印文件到 stdout，成功返回 0，失败返回非 0
int print_file_to_stdout(const char *path);

#endif // FLYUXC_IO_H
