#include "flyuxc/utils/cli.h"
#include "flyuxc/utils/io.h"
#include "flyuxc/frontend/normalize.h"
#include "flyuxc/frontend/varmap.h"
#include "flyuxc/frontend/lexer.h"
#include "flyuxc/frontend/parser.h"
#include "flyuxc/backend/codegen.h"
#include "flyuxc/llvm_compiler.h"
#include "flyuxc/version.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

/* ANSI 颜色代码 */
#define COLOR_BLUE    "\033[38;5;27m"
#define COLOR_CYAN    "\033[38;5;39m"
#define COLOR_GREEN   "\033[32m"
#define COLOR_YELLOW  "\033[33m"
#define COLOR_RED     "\033[31m"
#define COLOR_RESET   "\033[0m"

/* 获取当前时间（毫秒） */
static double get_time_ms(void) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec * 1000.0 + tv.tv_usec / 1000.0;
}

int main(int argc, char *argv[])
{
    // 尽早输出,用于测量启动时间
    double t_init_start = get_time_ms();
    
    CliOptions options = parse_arguments(argc, argv);

    /* 打印编译器信息 */
    if (argc == 1 || options.help)
    {
        printf("%s%s %s%s%s\n", COLOR_BLUE, FLYUXC_COMPILER_NAME, COLOR_CYAN, FLYUXC_VERSION, COLOR_RESET);
        printf("%sTarget: %s%s\n", COLOR_CYAN, COLOR_RESET, FLYUXC_TARGET);
        printf("%sThread model: %s%s\n", COLOR_CYAN, COLOR_RESET, FLYUXC_THREAD_MODEL);
        printf("-----------------------------------\n");
        print_help();
        return options.help ? 0 : 1;
    }

    if (options.version)
    {
        print_version();
        return 0;
    }
    

    if (options.input) {
        // 提前初始化 LLVM 以减少首次编译时的延迟
        llvm_initialize();
        
        double t_start = get_time_ms();
        
        printf("%s%s %s%s%s\n", COLOR_BLUE, FLYUXC_COMPILER_NAME, COLOR_CYAN, FLYUXC_VERSION, COLOR_RESET);
        printf("%sTarget: %s%s\n", COLOR_CYAN, COLOR_RESET, FLYUXC_TARGET);
        printf("%sThread model: %s%s\n", COLOR_CYAN, COLOR_RESET, FLYUXC_THREAD_MODEL);
        printf("-----------------------------------\n");
        printf("%s⚡ Compiling %s%s\n\n", COLOR_GREEN, options.input, COLOR_RESET);
        
        /* 读取源文件 */
        double t1 = get_time_ms();
        char* source_code = read_file_to_string(options.input);
        if (!source_code) {
            fprintf(stderr, "%sError:%s Failed to read file: %s\n", COLOR_RED, COLOR_RESET, options.input);
            return 1;
        }
        
        /* 保存原始源代码副本用于错误报告 */
        char* original_source = strdup(source_code);
        
        double t2 = get_time_ms();
        printf("%sSource loaded: %.2fms%s\n", COLOR_YELLOW, t2 - t1, COLOR_RESET);

        /* Step 1: 规范化代码 */
        double t3 = get_time_ms();
        
        // DEBUG: 输出原始代码
        if (getenv("DEBUG_NORM")) {
            fprintf(stderr, "=== SOURCE CODE ===\n%s\n=== END SOURCE ===\n", source_code);
        }
        
        NormalizeResult norm_result = flyux_normalize(source_code);
        free(source_code);

        if (norm_result.error_code != 0) {
            // 如果 error_msg 非空才打印（空字符串表示错误已通过全局接口输出）
            if (norm_result.error_msg && norm_result.error_msg[0] != '\0') {
                fprintf(stderr, "%sNormalization error:%s %s\n", COLOR_RED, COLOR_RESET, 
                        norm_result.error_msg);
            }
            normalize_result_free(&norm_result);
            return 1;
        }

        // DEBUG: 输出normalize结果
        if (getenv("DEBUG_NORM")) {
            fprintf(stderr, "=== NORMALIZED CODE ===\n%s\n=== END ===\n", norm_result.normalized);
        }

        /* Step 2: 变量名映射 */
        VarMapResult vm_result = flyux_varmap_process(norm_result.normalized,
                                                      norm_result.source_map,
                                                      norm_result.source_map_size,
                                                      original_source);
        if (vm_result.error_code != 0) {
            fprintf(stderr, "%sVarmap error:%s %s\n", COLOR_RED, COLOR_RESET,
                    vm_result.error_msg ? vm_result.error_msg : "Unknown error");
            normalize_result_free(&norm_result);
            varmap_result_free(&vm_result);
            return 1;
        }
        
        if (getenv("DEBUG_VARMAP")) {
            fprintf(stderr, "=== VARMAP RESULT ===\n%s\n=== END ===\n", vm_result.mapped_source);
            fprintf(stderr, "=== VARMAP ENTRIES ===\n");
            for (size_t i = 0; i < vm_result.entry_count; i++) {
                fprintf(stderr, "  %s -> %s\n", vm_result.entries[i].original, vm_result.entries[i].mapped);
            }
            fprintf(stderr, "=== END ===\n");
        }

        /* Step 3: 词法分析 */
        LexerResult lex_result = lexer_tokenize(vm_result.mapped_source,
                                                norm_result.source_map,
                                                norm_result.source_map_size,
                                                vm_result.offset_map,
                                                vm_result.offset_map_size);
        if (lex_result.error_code != 0) {
            fprintf(stderr, "%sLexer error:%s %s\n", COLOR_RED, COLOR_RESET,
                    lex_result.error_msg ? lex_result.error_msg : "Unknown error");
            lexer_result_free(&lex_result);
            normalize_result_free(&norm_result);
            varmap_result_free(&vm_result);
            return 1;
        }
        double t4 = get_time_ms();
        printf("%sLexical analysis: %.2fms%s\n", COLOR_YELLOW, t4 - t3, COLOR_RESET);

        /* Step 4: 语法分析 */
        double t5 = get_time_ms();
        Parser *parser = parser_create(lex_result.tokens, lex_result.count, vm_result.mapped_source);
        if (!parser) {
            fprintf(stderr, "%sParser error:%s Failed to create parser\n", COLOR_RED, COLOR_RESET);
            free(original_source);
            lexer_result_free(&lex_result);
            normalize_result_free(&norm_result);
            varmap_result_free(&vm_result);
            return 1;
        }
        
        /* 设置原始源代码用于错误报告 */
        parser_set_original_source(parser, original_source);

        ASTNode *ast = parser_parse(parser);
        bool has_errors = (parser->error_count > 0);
        bool has_warnings = (parser->warning_count > 0);
        
        if (has_warnings) {
            fprintf(stderr, "\n%s⚠️  %d warning(s)%s\n", COLOR_YELLOW, parser->warning_count, COLOR_RESET);
        }
        
        if (has_errors) {
            fprintf(stderr, "\n%sParsing failed with %d error(s)%s\n", COLOR_RED, parser->error_count, COLOR_RESET);
            if (ast) ast_node_free(ast);
            parser_free(parser);
            free(original_source);
            lexer_result_free(&lex_result);
            normalize_result_free(&norm_result);
            varmap_result_free(&vm_result);
            return 1;
        }
        double t6 = get_time_ms();
        printf("%sParsing: %.2fms%s\n", COLOR_YELLOW, t6 - t5, COLOR_RESET);

        /* Step 5: 代码生成 */
        if (!has_errors && ast) {
            double t7 = get_time_ms();
            
            // 确定基础文件名和可执行文件名
            const char *input_name = options.input;
            const char *dot = strrchr(input_name, '.');
            const char *slash = strrchr(input_name, '/');
            const char *base_name = slash ? slash + 1 : input_name;
            
            char executable_name[256];
            if (dot && dot > base_name) {
                size_t name_len = dot - base_name;
                strncpy(executable_name, base_name, name_len);
                executable_name[name_len] = '\0';
            } else {
                strcpy(executable_name, base_name);
            }
            
            CodeGen *codegen = NULL;
            FILE *output = NULL;
            char *ir_buffer = NULL;
            size_t ir_size = 0;
            
            // 根据是否需要输出 IR 文件选择不同的生成方式
            if (options.emit_ir) {
                // 生成 .ll 文件
                char output_file[256];
                snprintf(output_file, sizeof(output_file), "%s.ll", executable_name);
                
                output = fopen(output_file, "w");
                if (!output) {
                    fprintf(stderr, "%sError:%s Failed to open output file: %s\n", 
                            COLOR_RED, COLOR_RESET, output_file);
                    has_errors = true;
                }
            } else {
                // 使用内存流
                output = open_memstream(&ir_buffer, &ir_size);
                if (!output) {
                    fprintf(stderr, "%sError:%s Failed to create memory stream for IR generation\n",
                            COLOR_RED, COLOR_RESET);
                    has_errors = true;
                }
            }
            
            if (!has_errors && output) {
                codegen = codegen_create(output);
                if (!codegen) {
                    fprintf(stderr, "%sError:%s Failed to create code generator\n", COLOR_RED, COLOR_RESET);
                    has_errors = true;
                } else {
                    // 设置变量映射表用于错误消息
                    codegen_set_varmap(codegen, vm_result.entries, vm_result.entry_count);
                    // 设置原始源代码用于错误消息
                    codegen_set_original_source(codegen, original_source);
                    
                    codegen_generate(codegen, ast);
                    
                    // 检查 codegen 是否有错误（错误已经在 codegen 内部输出）
                    if (codegen_has_error(codegen)) {
                        has_errors = true;
                    }
                    
                    codegen_free(codegen);
                }
                
                fclose(output);
            }
            double t8 = get_time_ms();
            printf("%sIR generation: %.2fms%s\n", COLOR_YELLOW, t8 - t7, COLOR_RESET);
            
            /* Step 6: LLVM 编译 */
            if (!has_errors) {
                double t9 = get_time_ms();
                
                int opt_level = 1;
                
                if (options.emit_ir) {
                    // 从 .ll 文件编译
                    char output_file[256];
                    snprintf(output_file, sizeof(output_file), "%s.ll", executable_name);
                    
                    if (llvm_compile_to_executable(output_file, NULL, executable_name, opt_level) != 0) {
                        fprintf(stderr, "%sLLVM compilation failed:%s %s\n", 
                                COLOR_RED, COLOR_RESET, llvm_get_last_error());
                        has_errors = true;
                    }
                } else {
                    // 从内存中的 IR 字符串编译
                    if (ir_buffer) {
                        // 如果有DEBUG_NORM，输出IR以便调试
                        if (getenv("DEBUG_NORM")) {
                            fprintf(stderr, "\n=== GENERATED IR (first 5000 chars) ===\n");
                            fprintf(stderr, "%.5000s\n", ir_buffer);
                            fprintf(stderr, "=== END IR ===\n\n");
                        }
                        
                        if (llvm_compile_string_to_executable(ir_buffer, NULL, executable_name, opt_level) != 0) {
                            fprintf(stderr, "%sLLVM compilation failed:%s %s\n", 
                                    COLOR_RED, COLOR_RESET, llvm_get_last_error());
                            
                            // 验证失败时输出更多IR以便调试
                            if (getenv("DEBUG_NORM")) {
                                fprintf(stderr, "\n=== FULL IR ===\n");
                                fprintf(stderr, "%s\n", ir_buffer);
                                fprintf(stderr, "=== END FULL IR ===\n");
                            }
                            
                            has_errors = true;
                        }
                        free(ir_buffer);
                    }
                }
                
                double t10 = get_time_ms();
                printf("%sBinary emission: %.2fms%s\n", COLOR_YELLOW, t10 - t9, COLOR_RESET);
                
                if (!has_errors) {
                    double t_end = get_time_ms();
                    printf("\n%s✨ Compilation successful! (%.2fms)%s\n", 
                           COLOR_GREEN, t_end - t_start, COLOR_RESET);
                    if (options.emit_ir) {
                        printf("%sLLIR: %s%s.ll\n", COLOR_CYAN, COLOR_RESET, executable_name);
                    }
                }
            }
        }
        
        if (has_errors) {
            fprintf(stderr, "\n%s✗ Compilation failed%s\n", COLOR_RED, COLOR_RESET);
        }

        /* 清理AST */
        if (ast) {
            ast_node_free(ast);
        }
        parser_free(parser);

        /* 释放资源 */
        free(original_source);
        lexer_result_free(&lex_result);
        normalize_result_free(&norm_result);
        varmap_result_free(&vm_result);

        return has_errors ? 1 : 0;
    }

    /* 如果没有输入文件，则提示并退出 */
    fprintf(stderr, "No input file specified. Use -h for help.\n");
    return 1;
}
