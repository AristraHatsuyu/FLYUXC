#ifndef FLYUXC_NORMALIZE_H
#define FLYUXC_NORMALIZE_H

// 读取文件，去除注释并进行简单的格式规范化，然后把结果输出到 stdout
// 成功返回 0，失败返回非 0
int normalize_file_to_stdout(const char *path);

#endif // FLYUXC_NORMALIZE_H
