#include "flyuxc/cli.h"
#include "flyuxc/io.h"
#include "flyuxc/normalize.h"
#include "flyuxc/varmap.h"
#include "flyuxc/lexer.h"
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[])
{
    CliOptions options = parse_arguments(argc, argv);

    if (argc == 1 || options.help)
    {
        print_help();
        return options.help ? 0 : 1;
    }

    if (options.version)
    {
        print_version();
        return 0;
    }

        if (options.input) {
        // 读取源文件
        char* source_code = read_file_to_string(options.input);
        if (!source_code) {
            fprintf(stderr, "Failed to read file: %s\n", options.input);
            return 1;
        }

        // Step 1: 规范化代码
        NormalizeResult norm_result = flyux_normalize(source_code);
        free(source_code);

        if (norm_result.error_code != 0) {
            fprintf(stderr, "Normalization error: %s\n", norm_result.error_msg);
            normalize_result_free(&norm_result);
            return 1;
        }

        printf("=== Normalized Source ===\n");
        printf("%s\n", norm_result.normalized);

        // Step 2: 变量名映射
        VarMapResult vm_result = flyux_varmap_process(norm_result.normalized);
        if (vm_result.error_code != 0) {
            fprintf(stderr, "VarMap error: %s\n", vm_result.error_msg ? vm_result.error_msg : "(unknown)");
            normalize_result_free(&norm_result);
            varmap_result_free(&vm_result);
            return 1;
        }

        printf("\n=== Variable Mapping Table ===\n");
        varmap_print_table(&vm_result, stdout);

        printf("\n=== Mapped Source ===\n");
        printf("%s\n", vm_result.mapped_source);

        // 将来如果你要把映射后的代码喂给 Lexer，可以在这里调用：
        // LexerResult lex = lexer_tokenize(vm_result.mapped_source);
        // ...

        normalize_result_free(&norm_result);
        varmap_result_free(&vm_result);

        return 0;
    }

    // 如果没有输入文件，则提示并退出
    fprintf(stderr, "No input file specified. Use -h for help.\n");
    return 1;
}