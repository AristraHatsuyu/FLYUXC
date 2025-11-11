#include "flyuxc/cli.h"
#include "flyuxc/io.h"
#include "flyuxc/normalize.h"
#include <stdio.h>

int main(int argc, char *argv[]) {
    CliOptions options = parse_arguments(argc, argv);

    if (argc == 1 || options.help) {
        print_help();
        return options.help ? 0 : 1;
    }

    if (options.version) {
        print_version();
        return 0;
    }

    if (options.input) {
        // 当前阶段：先做规范化（去注释、格式化），再输出
        int rc = normalize_file_to_stdout(options.input);
        return rc;
    }

    // 如果没有输入文件，则提示并退出
    fprintf(stderr, "No input file specified. Use -h for help.\n");
    return 1;
}