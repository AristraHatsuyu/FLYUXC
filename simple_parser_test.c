/* simple_parser_test.c - 简单的Parser测试 */
#include "flyuxc/lexer.h"
#include "flyuxc/io.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* 简单的AST打印（树形格式） */
static void print_tokens_tree(Token* tokens, size_t count) {
    printf("=== Token Tree ===\n");
    int indent = 0;
    
    for (size_t i = 0; i < count; i++) {
        Token* tok = &tokens[i];
        
        /* 根据token类型调整缩进 */
        if (tok->kind == TK_R_BRACE || tok->kind == TK_R_PAREN || tok->kind == TK_R_BRACKET) {
            indent -= 2;
            if (indent < 0) indent = 0;
        }
        
        /* 打印缩进 */
        for (int j = 0; j < indent; j++) {
            printf(" ");
        }
        
        /* 打印token信息 */
        printf("%-15s ", token_kind_to_string(tok->kind));
        
        /* 打印token内容（如果有） */
        if (tok->lexeme && strlen(tok->lexeme) > 0) {
            printf("'%s'", tok->lexeme);
        }
        
        /* 打印位置信息 */
        printf(" [%d:%d]", tok->line, tok->column);
        
        printf("\n");
        
        /* 调整缩进 */
        if (tok->kind == TK_L_BRACE || tok->kind == TK_L_PAREN || tok->kind == TK_L_BRACKET) {
            indent += 2;
        }
    }
}

/* 简单的JSON输出 */
static void print_tokens_json(Token* tokens, size_t count) {
    printf("\n=== Token JSON ===\n");
    printf("{\n");
    printf("  \"tokens\": [\n");
    
    for (size_t i = 0; i < count; i++) {
        Token* tok = &tokens[i];
        
        printf("    {\n");
        printf("      \"kind\": \"%s\",\n", token_kind_to_string(tok->kind));
        printf("      \"loc\": {\"line\": %d, \"column\": %d, \"orig_line\": %d, \"orig_column\": %d}",
               tok->line, tok->column, tok->orig_line, tok->orig_column);
        
        /* 打印token值 */
        if (tok->lexeme && strlen(tok->lexeme) > 0) {
            printf(",\n      \"value\": \"");
            /* 转义特殊字符 */
            for (const char* p = tok->lexeme; *p; p++) {
                if (*p == '"') printf("\\\"");
                else if (*p == '\\') printf("\\\\");
                else if (*p == '\n') printf("\\n");
                else if (*p == '\t') printf("\\t");
                else putchar(*p);
            }
            printf("\"");
        }
        
        printf("\n    }");
        if (i < count - 1) printf(",");
        printf("\n");
    }
    
    printf("  ]\n");
    printf("}\n");
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <file.fx> [--json]\n", argv[0]);
        return 1;
    }
    
    const char* filename = argv[1];
    bool json_mode = (argc >= 3 && strcmp(argv[2], "--json") == 0);
    
    /* 读取源文件 */
    char* source = read_file_to_string(filename);
    if (!source) {
        fprintf(stderr, "Failed to read file: %s\n", filename);
        return 1;
    }
    
    printf("=== Source: %s ===\n", filename);
    printf("%s\n\n", source);
    
    /* 初始化Lexer */
    Lexer lexer;
    lexer_init(&lexer, source, filename);
    
    /* Tokenize */
    Token* tokens = NULL;
    size_t capacity = 256;
    size_t count = 0;
    
    tokens = malloc(capacity * sizeof(Token));
    
    Token tok;
    while (lexer_next_token(&lexer, &tok), tok.kind != TK_EOF) {
        if (count >= capacity) {
            capacity *= 2;
            tokens = realloc(tokens, capacity * sizeof(Token));
        }
        tokens[count++] = tok;
    }
    
    /* 添加EOF */
    if (count >= capacity) {
        tokens = realloc(tokens, (capacity + 1) * sizeof(Token));
    }
    tokens[count++] = tok;
    
    /* 输出 */
    if (json_mode) {
        print_tokens_json(tokens, count);
    } else {
        print_tokens_tree(tokens, count);
    }
    
    /* 清理 */
    free(tokens);
    free(source);
    
    printf("\n✅ Parsing completed!\n");
    printf("   Total tokens: %zu\n", count);
    
    return 0;
}
