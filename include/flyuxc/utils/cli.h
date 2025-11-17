#ifndef FLYUXC_CLI_H
#define FLYUXC_CLI_H

#include <stdbool.h>

// 命令行参数结构
typedef struct {
    bool help;           // -h, --help
    bool version;        // -v, --version
    const char* output;  // -o, --output
    const char* input;   // 输入文件
} CliOptions;

// CLI 函数声明
void print_help(void);
void print_version(void);
CliOptions parse_arguments(int argc, char *argv[]);

#endif // FLYUXC_CLI_H