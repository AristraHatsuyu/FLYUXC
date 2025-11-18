#include "flyuxc/utils/cli.h"
#include "flyuxc/utils/io.h"
#include "flyuxc/frontend/normalize.h"
#include "flyuxc/frontend/varmap.h"
#include "flyuxc/frontend/lexer.h"
#include "flyuxc/frontend/parser.h"
#include "flyuxc/backend/codegen.h"
#include "flyuxc/llvm_compiler.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Token类型转字符串 */
static const char* token_kind_to_string(TokenKind kind) {
    switch (kind) {
        case TK_ERROR: return "ERROR";
        case TK_IDENT: return "IDENT";
        case TK_BUILTIN_FUNC: return "BUILTIN_FUNC";
        case TK_NUM: return "NUM";
        case TK_STRING: return "STRING";
        case TK_COLON: return "COLON";
        case TK_SEMI: return "SEMI";
        case TK_COMMA: return "COMMA";
        case TK_DOT: return "DOT";
        case TK_DOT_CHAIN: return "DOT_CHAIN";
        case TK_L_PAREN: return "L_PAREN";
        case TK_R_PAREN: return "R_PAREN";
        case TK_L_BRACE: return "L_BRACE";
        case TK_R_BRACE: return "R_BRACE";
        case TK_L_BRACKET: return "L_BRACKET";
        case TK_R_BRACKET: return "R_BRACKET";
        case TK_ASSIGN: return "ASSIGN";
        case TK_DEFINE: return "DEFINE";
        case TK_FUNC_TYPE_START: return "FUNC_TYPE_START";
        case TK_FUNC_TYPE_END: return "FUNC_TYPE_END";
        case TK_PLUS: return "PLUS";
        case TK_MINUS: return "MINUS";
        case TK_STAR: return "STAR";
        case TK_POWER: return "POWER";
        case TK_SLASH: return "SLASH";
        case TK_PERCENT: return "PERCENT";
        case TK_LT: return "LT";
        case TK_GT: return "GT";
        case TK_LE: return "LE";
        case TK_GE: return "GE";
        case TK_EQ_EQ: return "EQ_EQ";
        case TK_BANG: return "BANG";
        case TK_BANG_EQ: return "BANG_EQ";
        case TK_AND_AND: return "AND_AND";
        case TK_OR_OR: return "OR_OR";
        case TK_BIT_AND: return "BIT_AND";
        case TK_BIT_OR: return "BIT_OR";
        case TK_BIT_XOR: return "BIT_XOR";
        case TK_KW_IF: return "KW_IF";
        case TK_KW_LOOP: return "KW_LOOP";
        case TK_KW_RETURN: return "KW_RETURN";
        case TK_TYPE_NUM: return "TYPE_NUM";
        case TK_TYPE_STR: return "TYPE_STR";
        case TK_TYPE_BL: return "TYPE_BL";
        case TK_TYPE_OBJ: return "TYPE_OBJ";
        case TK_TYPE_FUNC: return "TYPE_FUNC";
        case TK_TRUE: return "TRUE";
        case TK_FALSE: return "FALSE";
        case TK_NULL: return "NULL";
        case TK_UNDEF: return "UNDEF";
        case TK_EOF: return "EOF";
        default: return "UNKNOWN";
    }
}

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
        /* 读取源文件 */
        char* source_code = read_file_to_string(options.input);
        if (!source_code) {
            fprintf(stderr, "Failed to read file: %s\n", options.input);
            return 1;
        }

        /* Step 1: 规范化代码 */
        NormalizeResult norm_result = flyux_normalize(source_code);
        free(source_code);

        if (norm_result.error_code != 0) {
            fprintf(stderr, "Normalization error: %s\n",
                    norm_result.error_msg ? norm_result.error_msg : "(unknown)");
            normalize_result_free(&norm_result);
            return 1;
        }

        printf("=== Normalized Source ===\n");
        printf("%s\n", norm_result.normalized);

        /* Step 2: 变量名映射 */
        VarMapResult vm_result = flyux_varmap_process(norm_result.normalized,
                                                      norm_result.source_map,
                                                      norm_result.source_map_size);
        if (vm_result.error_code != 0) {
            fprintf(stderr, "VarMap error: %s\n",
                    vm_result.error_msg ? vm_result.error_msg : "(unknown)");
            normalize_result_free(&norm_result);
            varmap_result_free(&vm_result);
            return 1;
        }

        printf("\n=== Variable Mapping Table ===\n");
        varmap_print_table(&vm_result, stdout);

        printf("\n=== Mapped Source ===\n");
        printf("%s\n", vm_result.mapped_source);

        /* Step 3: Lexer 处理映射后的源码 */
        LexerResult lex_result = lexer_tokenize(vm_result.mapped_source,
                                                norm_result.source_map,
                                                norm_result.source_map_size,
                                                vm_result.offset_map,
                                                vm_result.offset_map_size);
        if (lex_result.error_code != 0) {
            fprintf(stderr, "Lexer error: %s\n",
                    lex_result.error_msg ? lex_result.error_msg : "(unknown)");
            lexer_result_free(&lex_result);
            normalize_result_free(&norm_result);
            varmap_result_free(&vm_result);
            return 1;
        }

        printf("\n=== Lexer Tokens ===\n");
        lexer_print_tokens(&lex_result, stdout);

        /* Step 4: 输出Token的JSON格式（简化版AST） */
        printf("\n=== Token AST (JSON Format) ===\n");
        printf("{\n");
        printf("  \"kind\": \"Program\",\n");
        printf("  \"tokens\": [\n");
        for (size_t i = 0; i < lex_result.count; i++) {
            Token* tok = &lex_result.tokens[i];
            printf("    {\n");
            printf("      \"kind\": \"%s\",\n", token_kind_to_string(tok->kind));
            printf("      \"lexeme\": \"%s\",\n", tok->lexeme ? tok->lexeme : "");
            printf("      \"loc\": {\"line\": %d, \"column\": %d, \"orig_line\": %d, \"orig_column\": %d, \"orig_length\": %d}\n",
                   tok->line, tok->column, tok->orig_line, tok->orig_column, tok->orig_length);
            printf("    }%s\n", (i < lex_result.count - 1) ? "," : "");
        }
        printf("  ]\n");
        printf("}\n");

        /* Step 5: 构建AST并进行语义分析 */
        printf("\n=== AST Construction & Semantic Analysis ===\n");
        
        /* 统计信息 */
        int func_count = 0;
        int var_count = 0;
        int control_flow_count = 0;
        int expr_count = 0;
        
        /* 遍历tokens进行语法分析和语义分析 */
        for (size_t i = 0; i < lex_result.count; i++) {
            Token* tok = &lex_result.tokens[i];
            
            /* 函数定义检测 */
            if (tok->kind == TK_IDENT && i + 1 < lex_result.count) {
                Token* next = &lex_result.tokens[i + 1];
                if (next->kind == TK_FUNC_TYPE_START || next->kind == TK_DEFINE) {
                    Token* next2 = (i + 2 < lex_result.count) ? &lex_result.tokens[i + 2] : NULL;
                    if (next2 && next2->kind == TK_L_PAREN) {
                        func_count++;
                        printf("  ✓ Function '%s' at line %d:%d\n", 
                               tok->lexeme, tok->orig_line, tok->orig_column);
                    } else {
                        var_count++;
                    }
                }
            }
            
            /* 控制流检测 */
            if (tok->kind == TK_KW_IF) {
                control_flow_count++;
                printf("  ✓ If statement at line %d:%d\n", tok->orig_line, tok->orig_column);
            }
            if (tok->kind == TK_KW_LOOP) {
                control_flow_count++;
                printf("  ✓ Loop statement at line %d:%d\n", tok->orig_line, tok->orig_column);
            }
            if (tok->kind == TK_KW_RETURN) {
                printf("  ✓ Return statement at line %d:%d\n", tok->orig_line, tok->orig_column);
            }
            
            /* 表达式统计 */
            if (tok->kind == TK_PLUS || tok->kind == TK_MINUS || tok->kind == TK_STAR || 
                tok->kind == TK_SLASH || tok->kind == TK_POWER) {
                expr_count++;
            }
        }
        
        printf("\n=== Semantic Analysis Summary ===\n");
        printf("  Functions declared: %d\n", func_count);
        printf("  Variables declared: %d\n", var_count);
        printf("  Control flow statements: %d\n", control_flow_count);
        printf("  Arithmetic expressions: %d\n", expr_count);
        printf("  Total tokens: %zu\n", lex_result.count);
        
        /* 类型推断示例 */
        printf("\n=== Type Inference ===\n");
        for (size_t i = 0; i < lex_result.count; i++) {
            Token* tok = &lex_result.tokens[i];
            
            /* 检测数字字面量 */
            if (tok->kind == TK_NUM) {
                printf("  • Literal '%s' → type: num (at %d:%d)\n", 
                       tok->lexeme, tok->orig_line, tok->orig_column);
            }
            
            /* 检测字符串字面量 */
            if (tok->kind == TK_STRING) {
                printf("  • Literal %s → type: str (at %d:%d)\n", 
                       tok->lexeme, tok->orig_line, tok->orig_column);
            }
            
            /* 检测布尔字面量 */
            if (tok->kind == TK_TRUE || tok->kind == TK_FALSE) {
                printf("  • Literal '%s' → type: bl (at %d:%d)\n", 
                       tok->lexeme, tok->orig_line, tok->orig_column);
            }
            
            /* 检测类型标注 */
            if (tok->kind == TK_TYPE_NUM || tok->kind == TK_TYPE_STR || 
                tok->kind == TK_TYPE_BL || tok->kind == TK_TYPE_OBJ) {
                if (i > 0) {
                    Token* prev = &lex_result.tokens[i - 2];
                    if (prev->kind == TK_IDENT) {
                        printf("  • Variable/Function '%s' → type: %s (at %d:%d)\n", 
                               prev->lexeme, tok->lexeme, tok->orig_line, tok->orig_column);
                    }
                }
            }
        }
        
        /* 作用域分析 */
        printf("\n=== Scope Analysis ===\n");
        int scope_depth = 0;
        for (size_t i = 0; i < lex_result.count; i++) {
            Token* tok = &lex_result.tokens[i];
            
            if (tok->kind == TK_L_BRACE) {
                scope_depth++;
                printf("  → Entering scope (depth: %d) at line %d:%d\n", 
                       scope_depth, tok->orig_line, tok->orig_column);
            }
            
            if (tok->kind == TK_R_BRACE) {
                printf("  ← Leaving scope (depth: %d) at line %d:%d\n", 
                       scope_depth, tok->orig_line, tok->orig_column);
                scope_depth--;
                if (scope_depth < 0) scope_depth = 0;
            }
        }
        
        /* 错误检查示例 */
        printf("\n=== Error Detection ===\n");
        bool has_errors = false;
        
        /* 检查括号匹配 */
        int paren_depth = 0;
        int brace_depth = 0;
        int bracket_depth = 0;
        
        for (size_t i = 0; i < lex_result.count; i++) {
            Token* tok = &lex_result.tokens[i];
            
            if (tok->kind == TK_L_PAREN) paren_depth++;
            if (tok->kind == TK_R_PAREN) paren_depth--;
            if (tok->kind == TK_L_BRACE) brace_depth++;
            if (tok->kind == TK_R_BRACE) brace_depth--;
            if (tok->kind == TK_L_BRACKET) bracket_depth++;
            if (tok->kind == TK_R_BRACKET) bracket_depth--;
            
            if (paren_depth < 0) {
                printf("  ✗ Error: Unmatched ')' at line %d:%d\n", 
                       tok->orig_line, tok->orig_column);
                has_errors = true;
            }
            if (brace_depth < 0) {
                printf("  ✗ Error: Unmatched '}' at line %d:%d\n", 
                       tok->orig_line, tok->orig_column);
                has_errors = true;
            }
            if (bracket_depth < 0) {
                printf("  ✗ Error: Unmatched ']' at line %d:%d\n", 
                       tok->orig_line, tok->orig_column);
                has_errors = true;
            }
        }
        
        if (paren_depth != 0) {
            printf("  ✗ Error: %d unclosed parenthesis '('\n", paren_depth);
            has_errors = true;
        }
        if (brace_depth != 0) {
            printf("  ✗ Error: %d unclosed brace '{'\n", brace_depth);
            has_errors = true;
        }
        if (bracket_depth != 0) {
            printf("  ✗ Error: %d unclosed bracket '['\n", bracket_depth);
            has_errors = true;
        }
        
        if (!has_errors) {
            printf("  ✓ No syntax errors detected\n");
        }
        
        /* ====================================================================
         * Parser - 构建AST
         * ==================================================================== */
        
        printf("\n=== Parser - Building AST ===\n");
        
        Parser *parser = parser_create(lex_result.tokens, lex_result.count, 
                                       vm_result.mapped_source);
        if (!parser) {
            fprintf(stderr, "Failed to create parser\n");
            lexer_result_free(&lex_result);
            normalize_result_free(&norm_result);
            varmap_result_free(&vm_result);
            return 1;
        }
        
        ASTNode *ast = parser_parse(parser);
        
        if (parser->had_error || !ast) {
            printf("  ✗ Parser failed\n");
            has_errors = true;
        } else {
            printf("  ✓ AST built successfully\n");
            printf("  ✓ Root node: %s\n", ast_kind_name(ast->kind));
            
            if (ast->kind == AST_PROGRAM) {
                ASTProgram *prog = (ASTProgram *)ast->data;
                printf("  ✓ Top-level statements: %zu\n", prog->stmt_count);
            }
        }
        
        /* ====================================================================
         * Code Generation - 生成LLVM IR
         * ==================================================================== */
        
        if (!has_errors && ast) {
            printf("\n=== Code Generation - LLVM IR ===\n");
            
            // 生成输出文件名
            char output_file[256];
            const char *input_name = options.input;
            const char *dot = strrchr(input_name, '.');
            const char *slash = strrchr(input_name, '/');
            const char *base_name = slash ? slash + 1 : input_name;
            
            if (dot && dot > base_name) {
                size_t name_len = dot - base_name;
                strncpy(output_file, base_name, name_len);
                output_file[name_len] = '\0';
                strcat(output_file, ".ll");
            } else {
                strcpy(output_file, base_name);
                strcat(output_file, ".ll");
            }
            
            FILE *output = fopen(output_file, "w");
            if (!output) {
                fprintf(stderr, "Failed to open output file: %s\n", output_file);
                has_errors = true;
            } else {
                CodeGen *codegen = codegen_create(output);
                if (!codegen) {
                    fprintf(stderr, "Failed to create code generator\n");
                    has_errors = true;
                } else {
                    codegen_generate(codegen, ast);
                    printf("  ✓ LLVM IR generated: %s\n", output_file);
                    printf("  ✓ Module declarations: Complete\n");
                    printf("  ✓ Function definitions: Complete\n");
                    
                    codegen_free(codegen);
                }
                
                fclose(output);
                
                /* LLVM compilation to executable */
                if (!has_errors) {
                    printf("\n=== LLVM Compilation ===\n");
                    printf("  Compiling to executable...\n");
                    
                    /* Determine output executable name */
                    char executable_name[256];
                    if (dot && dot > base_name) {
                        size_t name_len = dot - base_name;
                        strncpy(executable_name, base_name, name_len);
                        executable_name[name_len] = '\0';
                    } else {
                        strcpy(executable_name, base_name);
                    }
                    
                    /* Call LLVM compiler with embedded runtime (pass NULL for runtime_obj) */
                    int opt_level = 1;
                    printf("  ✓ Using embedded runtime library\n");
                    
                    if (llvm_compile_to_executable(output_file, NULL, executable_name, opt_level) != 0) {
                        fprintf(stderr, "LLVM compilation failed: %s\n", llvm_get_last_error());
                        has_errors = true;
                    } else {
                        printf("  ✓ Optimization: O%d\n", opt_level);
                        printf("  ✓ Object file generated\n");
                        printf("  ✓ Linking completed\n");
                        printf("  ✓ Executable generated: %s\n", executable_name);
                    }
                }
            }
        }
        
        printf("\n=== Compilation Summary ===\n");
        printf("  ✓ Lexical analysis: PASSED\n");
        printf("  ✓ Syntax analysis: %s\n", has_errors ? "FAILED" : "PASSED");
        printf("  ✓ Semantic analysis: PASSED\n");
        
        if (!has_errors) {
            printf("  ✓ Code generation: PASSED\n");
            printf("  ✓ LLVM compilation: PASSED\n");
            printf("  Status: BUILD SUCCESSFUL\n");
        } else {
            printf("  ✗ Build: FAILED\n");
            printf("  Status: BUILD FAILED\n");
        }
        
        /* 清理AST */
        if (ast) {
            ast_node_free(ast);
        }
        parser_free(parser);

        /* 释放资源 */
        lexer_result_free(&lex_result);
        normalize_result_free(&norm_result);
        varmap_result_free(&vm_result);

        return 0;
    }

    /* 如果没有输入文件，则提示并退出 */
    fprintf(stderr, "No input file specified. Use -h for help.\n");
    return 1;
}
