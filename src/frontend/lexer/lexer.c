#include "flyuxc/frontend/lexer.h"
#include "flyuxc/frontend/normalize.h"

#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* ========== 小工具 ========== */

static int is_space_c(int c) {
    return c == ' ' || c == '\t' || c == '\r' || c == '\n' || c == '\f' || c == '\v';
}

/* 标识符起始：字母 / _ / 非 ASCII 字节（emoji 之类） */
static int is_ident_start(unsigned char c) {
    return (c == '_') || isalpha(c) || (c >= 0x80);
}

/* 标识符中间：字母 / 数字 / _ / 非 ASCII */
static int is_ident_char(unsigned char c) {
    return (c == '_') || isalnum(c) || (c >= 0x80);
}

/* 简单的 strndup */
static char* str_dup_n(const char* s, size_t len) {
    char* out = (char*)malloc(len + 1);
    if (!out) return NULL;
    memcpy(out, s, len);
    out[len] = '\0';
    return out;
}

/* 动态数组扩容 */
static int ensure_token_capacity(Token** arr, size_t* cap, size_t needed) {
    if (*cap >= needed) return 1;
    size_t new_cap = (*cap == 0) ? 32 : (*cap * 2);
    while (new_cap < needed) new_cap *= 2;
    Token* p = (Token*)realloc(*arr, new_cap * sizeof(Token));
    if (!p) return 0;
    *arr = p;
    *cap = new_cap;
    return 1;
}

/* ===== 内置函数表 ===== */
/* 
 * FLYUX 内置函数完整列表 (64个)
 * 分类: 输入输出、字符串、数学、数组、对象、类型、时间、工具
 * 最后更新: 2025-11-17
 */
static const char* BUILTIN_FUNC_TABLE[] = {
    /* 输入输出 (4) */
    "print",
    "input",
    "readFile",
    "writeFile",
    
    /* 字符串操作 (11) */
    "length",
    "substr",
    "indexOf",
    "replace",
    "split",
    "join",
    "toUpper",
    "toLower",
    "trim",
    "startsWith",
    "endsWith",
    
    /* 数学函数 (9) */
    "abs",
    "floor",
    "ceil",
    "round",
    "sqrt",
    "pow",
    "min",
    "max",
    "random",
    "randomInt",
    
    /* 数组操作 (16) */
    "push",
    "pop",
    "shift",
    "unshift",
    "slice",
    "concat",
    "reverse",
    "sort",
    "filter",
    "map",
    "reduce",
    "find",
    "includes",
    
    /* 对象操作 (7) */
    "keys",
    "values",
    "entries",
    "hasKey",
    "merge",
    "clone",
    "deepClone",
    
    /* 类型转换和检查 (11) */
    "toNum",
    "toStr",
    "toBl",
    "typeOf",
    "isNum",
    "isStr",
    "isBl",
    "isArr",
    "isObj",
    "isNull",
    "isUndef",
    
    /* 时间函数 (3) */
    "now",
    "sleep",
    "dateStr",
    
    /* 实用工具 (3) */
    "assert",
    "exit",
    "range",
    
    NULL  /* 结束标记 */
};

static int is_builtin_func_name(const char* name) {
    for (int i = 0; BUILTIN_FUNC_TABLE[i] != NULL; i++) {
        if (strcmp(name, BUILTIN_FUNC_TABLE[i]) == 0) {
            return 1;
        }
    }
    return 0;
}

/* 关键字 / 类型 / 布尔字面量检查 */
static TokenKind classify_identifier(const char* lexeme) {
    /* 关键字 */
    if (strcmp(lexeme, "if") == 0)   return TK_KW_IF;

    /* 类型 */
    if (strcmp(lexeme, "num") == 0)  return TK_TYPE_NUM;
    if (strcmp(lexeme, "str") == 0)  return TK_TYPE_STR;
    if (strcmp(lexeme, "bl") == 0)   return TK_TYPE_BL;
    if (strcmp(lexeme, "obj") == 0)  return TK_TYPE_OBJ;
    if (strcmp(lexeme, "func") == 0) return TK_TYPE_FUNC;

    /* 布尔 / 特殊值 */
    if (strcmp(lexeme, "true") == 0)  return TK_TRUE;
    if (strcmp(lexeme, "false") == 0) return TK_FALSE;
    if (strcmp(lexeme, "null") == 0)  return TK_NULL;
    if (strcmp(lexeme, "undef") == 0) return TK_UNDEF;

    /* 内置函数 */
    if (is_builtin_func_name(lexeme)) return TK_BUILTIN_FUNC;

    return TK_IDENT;
}

/* token kind -> 调试用名字 */
static const char* token_kind_name(TokenKind kind) {
    switch (kind) {
        case TK_ERROR:          return "ERROR";
        case TK_IDENT:          return "IDENT";
        case TK_BUILTIN_FUNC:   return "BUILTIN_FUNC";
        case TK_NUM:            return "NUM";
        case TK_STRING:         return "STRING";

        case TK_COLON:          return "COLON";
        case TK_SEMI:           return "SEMI";
        case TK_COMMA:          return "COMMA";
        case TK_DOT:            return "DOT";
        case TK_DOT_CHAIN:      return "DOT_CHAIN";
        case TK_L_PAREN:        return "L_PAREN";
        case TK_R_PAREN:        return "R_PAREN";
        case TK_L_BRACE:        return "L_BRACE";
        case TK_R_BRACE:        return "R_BRACE";
        case TK_L_BRACKET:      return "L_BRACKET";
        case TK_R_BRACKET:      return "R_BRACKET";

        case TK_ASSIGN:         return "ASSIGN";
        case TK_DEFINE:         return "DEFINE";
        case TK_FUNC_TYPE_START:return "FUNC_TYPE_START";
        case TK_FUNC_TYPE_END:  return "FUNC_TYPE_END";

        case TK_PLUS:           return "PLUS";
        case TK_MINUS:          return "MINUS";
        case TK_STAR:           return "STAR";
        case TK_POWER:          return "POWER";
        case TK_SLASH:          return "SLASH";
        case TK_PERCENT:        return "PERCENT";

        case TK_LT:             return "LT";
        case TK_GT:             return "GT";
        case TK_LE:             return "LE";
        case TK_GE:             return "GE";
        case TK_EQ_EQ:          return "EQ_EQ";
        case TK_BANG:           return "BANG";
        case TK_BANG_EQ:        return "BANG_EQ";
        case TK_AND_AND:        return "AND_AND";
        case TK_OR_OR:          return "OR_OR";

        case TK_BIT_AND:        return "BIT_AND";
        case TK_BIT_OR:         return "BIT_OR";
        case TK_BIT_XOR:        return "BIT_XOR";

        case TK_KW_IF:          return "KW_IF";
        case TK_KW_LOOP:        return "KW_LOOP";
        case TK_KW_RETURN:      return "KW_RETURN";

        case TK_TYPE_NUM:       return "TYPE_NUM";
        case TK_TYPE_STR:       return "TYPE_STR";
        case TK_TYPE_BL:        return "TYPE_BL";
        case TK_TYPE_OBJ:       return "TYPE_OBJ";
        case TK_TYPE_FUNC:      return "TYPE_FUNC";

        case TK_TRUE:           return "TRUE";
        case TK_FALSE:          return "FALSE";
        case TK_NULL:           return "NULL";
        case TK_UNDEF:          return "UNDEF";

        case TK_EOF:            return "EOF";
        default:                return "UNKNOWN";
    }
}

/* 生成错误信息 */
static char* make_unexpected_char_msg(char ch) {
    char buf[64];
    snprintf(buf, sizeof(buf), "Unexpected character: 0x%02X", (unsigned char)ch);
    return str_dup_n(buf, strlen(buf));
}

/* 添加一个 token */
static int emit_token(Token** tokens,
                      size_t* count,
                      size_t* cap,
                      TokenKind kind,
                      const char* lexeme_start,
                      size_t lexeme_len,
                      int line,
                      int column,
                      const SourceLocation* norm_source_map,
                      size_t norm_source_map_size,
                      const size_t* offset_map,
                      size_t offset_map_size,
                      size_t mapped_offset) {
    if (!ensure_token_capacity(tokens, cap, *count + 1)) {
        return 0;
    }
    Token* t = &(*tokens)[*count];
    t->kind = kind;
    t->lexeme = str_dup_n(lexeme_start, lexeme_len);
    if (!t->lexeme) return 0;
    t->line = line;
    t->column = column;
    
    // 查询原始源码位置（两级映射）
    if (offset_map && offset_map_size > 0 && mapped_offset < offset_map_size &&
        norm_source_map && norm_source_map_size > 0) {
        // 第1级：mapped_offset → normalized_offset
        size_t norm_offset = offset_map[mapped_offset];
        // 第2级：normalized_offset → original position
        if (norm_offset < norm_source_map_size) {
            const SourceLocation* loc = &norm_source_map[norm_offset];
            if (loc->is_synthetic) {
                // 合成字符，标记为0
                t->orig_line = 0;
                t->orig_column = 0;
                t->orig_length = 0;
            } else {
                t->orig_line = loc->orig_line;
                t->orig_column = loc->orig_column;
                
                // 计算整个token的原始长度
                // 策略1：检查是否所有字符映射到同一位置（varmap替换的标识符）
                // 策略2：计算从第一个到最后一个非synthetic字符的跨度
                int total_orig_len = 0;
                int all_same_pos = 1;
                size_t first_norm_off = norm_offset;
                const SourceLocation* first_loc = loc;
                const SourceLocation* last_loc = NULL;
                int found_valid = 0;
                
                // 遍历所有字符，找到最后一个有效字符
                for (size_t k = 0; k < lexeme_len; k++) {
                    size_t map_off = mapped_offset + k;
                    if (map_off < offset_map_size) {
                        size_t norm_off = offset_map[map_off];
                        if (norm_off < norm_source_map_size) {
                            const SourceLocation* cur_loc = &norm_source_map[norm_off];
                            if (!cur_loc->is_synthetic && cur_loc->orig_line > 0) {
                                last_loc = cur_loc;
                                found_valid = 1;
                                if (norm_off != first_norm_off) {
                                    all_same_pos = 0;
                                }
                            }
                        }
                    }
                }
                
                // 如果没找到有效字符，使用第一个字符
                if (!found_valid || !last_loc) {
                    last_loc = first_loc;
                }
                
                // 计算长度
                if (all_same_pos || first_loc == last_loc) {
                    // 所有字符映射到同一位置，或只有一个字符
                    total_orig_len = first_loc->orig_length;
                } else if (first_loc->orig_line == last_loc->orig_line) {
                    // 同一行：用跨度计算
                    total_orig_len = (last_loc->orig_column - first_loc->orig_column) + last_loc->orig_length;
                } else {
                    // 跨行或其他情况：累加所有字符长度
                    total_orig_len = 0;
                    for (size_t k = 0; k < lexeme_len; k++) {
                        size_t map_off = mapped_offset + k;
                        if (map_off < offset_map_size) {
                            size_t norm_off = offset_map[map_off];
                            if (norm_off < norm_source_map_size) {
                                const SourceLocation* cur_loc = &norm_source_map[norm_off];
                                if (!cur_loc->is_synthetic && cur_loc->orig_line > 0) {
                                    total_orig_len += cur_loc->orig_length;
                                }
                            }
                        }
                    }
                    // 如果累加结果为0，至少用第一个字符的长度
                    if (total_orig_len == 0) {
                        total_orig_len = first_loc->orig_length;
                    }
                }
                
                // 如果所有字符映射到同一位置（变量被替换），只用第一个字符的长度
                // 否则使用累加的总长度
                t->orig_length = total_orig_len > 0 ? total_orig_len : (int)lexeme_len;
            }
        } else {
            // 越界，使用规范化位置
            t->orig_line = line;
            t->orig_column = column;
            t->orig_length = (int)lexeme_len;
        }
    } else {
        // 无映射，使用规范化位置
        t->orig_line = line;
        t->orig_column = column;
        t->orig_length = (int)lexeme_len;
    }
    
    (*count)++;
    return 1;
}

/* ========== 主词法分析 ========== */

LexerResult lexer_tokenize(const char* source,
                          const SourceLocation* norm_source_map,
                          size_t norm_source_map_size,
                          const size_t* offset_map,
                          size_t offset_map_size) {
    LexerResult result;
    result.tokens = NULL;
    result.count = 0;
    result.error_msg = NULL;
    result.error_code = 0;

    if (!source) {
        result.error_code = -1;
        result.error_msg = str_dup_n("source is NULL", strlen("source is NULL"));
        return result;
    }

    size_t len = strlen(source);
    size_t cap = 0;
    Token* tokens = NULL;

    size_t i = 0;
    int line = 1;
    int col  = 1;

    while (i < len) {
        char c = source[i];

        /* 跳过空白 */
        if (is_space_c((unsigned char)c)) {
            if (c == '\n') {
                line++;
                col = 1;
            } else {
                col++;
            }
            i++;
            continue;
        }

        int start_line = line;
        int start_col  = col;
        size_t start_offset = i;

        /* L> / R> 作为关键字 */
        if (c == 'L' && i + 1 < len && source[i + 1] == '>') {
            if (!emit_token(&tokens, &result.count, &cap,
                            TK_KW_LOOP, source + i, 2, start_line, start_col,
                            norm_source_map, norm_source_map_size, offset_map, offset_map_size, start_offset)) {
                result.error_code = -1;
                result.error_msg = str_dup_n("Memory allocation failed", strlen("Memory allocation failed"));
                goto fail;
            }
            i += 2;
            col += 2;
            continue;
        }

        if (c == 'R' && i + 1 < len && source[i + 1] == '>') {
            if (!emit_token(&tokens, &result.count, &cap,
                            TK_KW_RETURN, source + i, 2, start_line, start_col,
                            norm_source_map, norm_source_map_size, offset_map, offset_map_size, start_offset)) {
                result.error_code = -1;
                result.error_msg = str_dup_n("Memory allocation failed", strlen("Memory allocation failed"));
                goto fail;
            }
            i += 2;
            col += 2;
            continue;
        }

        /* 标识符 / 关键字 / 内置函数 / 类型名 */
        if (is_ident_start((unsigned char)c)) {
            size_t start = i;
            i++;
            col++;
            while (i < len && is_ident_char((unsigned char)source[i])) {
                i++;
                col++;
            }
            size_t ident_len = i - start;
            char* lexeme = str_dup_n(source + start, ident_len);
            if (!lexeme) {
                result.error_code = -1;
                result.error_msg = str_dup_n("Memory allocation failed", strlen("Memory allocation failed"));
                goto fail;
            }

            TokenKind kind = classify_identifier(lexeme);

            /* emit_token 会自己再复制一份 lexeme，这里这份临时的要先 free 掉 */
            free(lexeme);
            if (!emit_token(&tokens, &result.count, &cap,
                            kind, source + start, ident_len, start_line, start_col, norm_source_map, norm_source_map_size, offset_map, offset_map_size, start_offset)) {
                result.error_code = -1;
                result.error_msg = str_dup_n("Memory allocation failed", strlen("Memory allocation failed"));
                goto fail;
            }
            continue;
        }

        /* 数字常量（支持整数和浮点数） */
        if (isdigit((unsigned char)c)) {
            size_t start = i;
            i++;
            col++;
            /* 整数部分 */
            while (i < len && isdigit((unsigned char)source[i])) {
                i++;
                col++;
            }
            
            /* 小数点和小数部分 */
            if (i < len && source[i] == '.' && i + 1 < len && isdigit((unsigned char)source[i + 1])) {
                i++; col++; /* 跳过 '.' */
                while (i < len && isdigit((unsigned char)source[i])) {
                    i++;
                    col++;
                }
            }
            
            /* 科学计数法 (e/E) */
            if (i < len && (source[i] == 'e' || source[i] == 'E')) {
                i++; col++;
                /* 可选的正负号 */
                if (i < len && (source[i] == '+' || source[i] == '-')) {
                    i++; col++;
                }
                /* 指数部分必须有数字 */
                if (i < len && isdigit((unsigned char)source[i])) {
                    while (i < len && isdigit((unsigned char)source[i])) {
                        i++;
                        col++;
                    }
                } else {
                    /* 无效的科学计数法格式 */
                    result.error_code = 1;
                    result.error_msg = str_dup_n("Invalid number format: expected digit after exponent", strlen("Invalid number format: expected digit after exponent"));
                    goto fail;
                }
            }
            
            size_t num_len = i - start;
            if (!emit_token(&tokens, &result.count, &cap,
                            TK_NUM, source + start, num_len, start_line, start_col, norm_source_map, norm_source_map_size, offset_map, offset_map_size, start_offset)) {
                result.error_code = -1;
                result.error_msg = str_dup_n("Memory allocation failed", strlen("Memory allocation failed"));
                goto fail;
            }
            continue;
        }

        /* 字符串字面量：支持 "..." 或 '...'，简单跳过，内部不做转义 */
        if (c == '"' || c == '\'') {
            char quote = c;
            size_t start = i;
            i++;
            col++;
            while (i < len && source[i] != quote) {
                if (source[i] == '\n') {
                    line++;
                    col = 1;
                    i++;
                } else if (source[i] == '\\' && (i + 1) < len) {
                    /* 简单跳过转义对 */
                    i += 2;
                    col += 2;
                } else {
                    i++;
                    col++;
                }
            }
            if (i < len && source[i] == quote) {
                i++;
                col++;
            } else {
                /* 未闭合的字符串 */
                result.error_code = 1;
                result.error_msg = str_dup_n("Unterminated string literal", strlen("Unterminated string literal"));
                goto fail;
            }

            size_t str_len = i - start;
            if (!emit_token(&tokens, &result.count, &cap,
                            TK_STRING, source + start, str_len, start_line, start_col, norm_source_map, norm_source_map_size, offset_map, offset_map_size, start_offset)) {
                result.error_code = -1;
                result.error_msg = str_dup_n("Memory allocation failed", strlen("Memory allocation failed"));
                goto fail;
            }
            continue;
        }

        /* 操作符 & 标点 */

        /* . 或 .> */
        if (c == '.') {
            if (i + 1 < len && source[i + 1] == '>') {
                if (!emit_token(&tokens, &result.count, &cap,
                                TK_DOT_CHAIN, source + i, 2, start_line, start_col, norm_source_map, norm_source_map_size, offset_map, offset_map_size, start_offset)) {
                    result.error_code = -1;
                    result.error_msg = str_dup_n("Memory allocation failed", strlen("Memory allocation failed"));
                    goto fail;
                }
                i += 2;
                col += 2;
            } else {
                if (!emit_token(&tokens, &result.count, &cap,
                                TK_DOT, source + i, 1, start_line, start_col, norm_source_map, norm_source_map_size, offset_map, offset_map_size, start_offset)) {
                    result.error_code = -1;
                    result.error_msg = str_dup_n("Memory allocation failed", strlen("Memory allocation failed"));
                    goto fail;
                }
                i++;
                col++;
            }
            continue;
        }

        /* : / := / :< */
        if (c == ':') {
            if (i + 1 < len && source[i + 1] == '=') {
                if (!emit_token(&tokens, &result.count, &cap,
                                TK_DEFINE, source + i, 2, start_line, start_col, norm_source_map, norm_source_map_size, offset_map, offset_map_size, start_offset)) {
                    result.error_code = -1;
                    result.error_msg = str_dup_n("Memory allocation failed", strlen("Memory allocation failed"));
                    goto fail;
                }
                i += 2;
                col += 2;
            } else if (i + 1 < len && source[i + 1] == '<') {
                if (!emit_token(&tokens, &result.count, &cap,
                                TK_FUNC_TYPE_START, source + i, 2, start_line, start_col, norm_source_map, norm_source_map_size, offset_map, offset_map_size, start_offset)) {
                    result.error_code = -1;
                    result.error_msg = str_dup_n("Memory allocation failed", strlen("Memory allocation failed"));
                    goto fail;
                }
                i += 2;
                col += 2;
            } else {
                if (!emit_token(&tokens, &result.count, &cap,
                                TK_COLON, source + i, 1, start_line, start_col, norm_source_map, norm_source_map_size, offset_map, offset_map_size, start_offset)) {
                    result.error_code = -1;
                    result.error_msg = str_dup_n("Memory allocation failed", strlen("Memory allocation failed"));
                    goto fail;
                }
                i++;
                col++;
            }
            continue;
        }

        /* & / && */
        if (c == '&') {
            if (i + 1 < len && source[i + 1] == '&') {
                if (!emit_token(&tokens, &result.count, &cap,
                                TK_AND_AND, source + i, 2, start_line, start_col, norm_source_map, norm_source_map_size, offset_map, offset_map_size, start_offset)) {
                    result.error_code = -1;
                    result.error_msg = str_dup_n("Memory allocation failed", strlen("Memory allocation failed"));
                    goto fail;
                }
                i += 2;
                col += 2;
            } else {
                /* 单独的 & ：位与 */
                if (!emit_token(&tokens, &result.count, &cap,
                                TK_BIT_AND, source + i, 1, start_line, start_col, norm_source_map, norm_source_map_size, offset_map, offset_map_size, start_offset)) {
                    result.error_code = -1;
                    result.error_msg = str_dup_n("Memory allocation failed", strlen("Memory allocation failed"));
                    goto fail;
                }
                i++;
                col++;
            }
            continue;
        }

        /* | / || */
        if (c == '|') {
            if (i + 1 < len && source[i + 1] == '|') {
                if (!emit_token(&tokens, &result.count, &cap,
                                TK_OR_OR, source + i, 2, start_line, start_col, norm_source_map, norm_source_map_size, offset_map, offset_map_size, start_offset)) {
                    result.error_code = -1;
                    result.error_msg = str_dup_n("Memory allocation failed", strlen("Memory allocation failed"));
                    goto fail;
                }
                i += 2;
                col += 2;
            } else {
                /* 单独的 | ：位或 */
                if (!emit_token(&tokens, &result.count, &cap,
                                TK_BIT_OR, source + i, 1, start_line, start_col, norm_source_map, norm_source_map_size, offset_map, offset_map_size, start_offset)) {
                    result.error_code = -1;
                    result.error_msg = str_dup_n("Memory allocation failed", strlen("Memory allocation failed"));
                    goto fail;
                }
                i++;
                col++;
            }
            continue;
        }

        /* ! / != */
        if (c == '!') {
            if (i + 1 < len && source[i + 1] == '=') {
                if (!emit_token(&tokens, &result.count, &cap,
                                TK_BANG_EQ, source + i, 2, start_line, start_col, norm_source_map, norm_source_map_size, offset_map, offset_map_size, start_offset)) {
                    result.error_code = -1;
                    result.error_msg = str_dup_n("Memory allocation failed", strlen("Memory allocation failed"));
                    goto fail;
                }
                i += 2;
                col += 2;
            } else {
                if (!emit_token(&tokens, &result.count, &cap,
                                TK_BANG, source + i, 1, start_line, start_col, norm_source_map, norm_source_map_size, offset_map, offset_map_size, start_offset)) {
                    result.error_code = -1;
                    result.error_msg = str_dup_n("Memory allocation failed", strlen("Memory allocation failed"));
                    goto fail;
                }
                i++;
                col++;
            }
            continue;
        }

        /* = / == */
        if (c == '=') {
            if (i + 1 < len && source[i + 1] == '=') {
                if (!emit_token(&tokens, &result.count, &cap,
                                TK_EQ_EQ, source + i, 2, start_line, start_col, norm_source_map, norm_source_map_size, offset_map, offset_map_size, start_offset)) {
                    result.error_code = -1;
                    result.error_msg = str_dup_n("Memory allocation failed", strlen("Memory allocation failed"));
                    goto fail;
                }
                i += 2;
                col += 2;
            } else {
                if (!emit_token(&tokens, &result.count, &cap,
                                TK_ASSIGN, source + i, 1, start_line, start_col, norm_source_map, norm_source_map_size, offset_map, offset_map_size, start_offset)) {
                    result.error_code = -1;
                    result.error_msg = str_dup_n("Memory allocation failed", strlen("Memory allocation failed"));
                    goto fail;
                }
                i++;
                col++;
            }
            continue;
        }

        /* < / <= */
        if (c == '<') {
            if (i + 1 < len && source[i + 1] == '=') {
                if (!emit_token(&tokens, &result.count, &cap,
                                TK_LE, source + i, 2, start_line, start_col, norm_source_map, norm_source_map_size, offset_map, offset_map_size, start_offset)) {
                    result.error_code = -1;
                    result.error_msg = str_dup_n("Memory allocation failed", strlen("Memory allocation failed"));
                    goto fail;
                }
                i += 2;
                col += 2;
            } else {
                if (!emit_token(&tokens, &result.count, &cap,
                                TK_LT, source + i, 1, start_line, start_col, norm_source_map, norm_source_map_size, offset_map, offset_map_size, start_offset)) {
                    result.error_code = -1;
                    result.error_msg = str_dup_n("Memory allocation failed", strlen("Memory allocation failed"));
                    goto fail;
                }
                i++;
                col++;
            }
            continue;
        }

        /* > / >=（这里 >= 默认作为 FUNC_TYPE_END；如果以后要区分 GE，可以再增加状态机） */
        if (c == '>') {
            if (i + 1 < len && source[i + 1] == '=') {
                if (!emit_token(&tokens, &result.count, &cap,
                                TK_FUNC_TYPE_END, source + i, 2, start_line, start_col, norm_source_map, norm_source_map_size, offset_map, offset_map_size, start_offset)) {
                    result.error_code = -1;
                    result.error_msg = str_dup_n("Memory allocation failed", strlen("Memory allocation failed"));
                    goto fail;
                }
                i += 2;
                col += 2;
            } else {
                if (!emit_token(&tokens, &result.count, &cap,
                                TK_GT, source + i, 1, start_line, start_col, norm_source_map, norm_source_map_size, offset_map, offset_map_size, start_offset)) {
                    result.error_code = -1;
                    result.error_msg = str_dup_n("Memory allocation failed", strlen("Memory allocation failed"));
                    goto fail;
                }
                i++;
                col++;
            }
            continue;
        }

        /* + - * / % ; , () {} [] */
        switch (c) {
            case '+':
                if (!emit_token(&tokens, &result.count, &cap,
                                TK_PLUS, source + i, 1, start_line, start_col, norm_source_map, norm_source_map_size, offset_map, offset_map_size, start_offset)) {
                    result.error_code = -1;
                    result.error_msg = str_dup_n("Memory allocation failed", strlen("Memory allocation failed"));
                    goto fail;
                }
                i++; col++;
                continue;
            case '-':
                if (!emit_token(&tokens, &result.count, &cap,
                                TK_MINUS, source + i, 1, start_line, start_col, norm_source_map, norm_source_map_size, offset_map, offset_map_size, start_offset)) {
                    result.error_code = -1;
                    result.error_msg = str_dup_n("Memory allocation failed", strlen("Memory allocation failed"));
                    goto fail;
                }
                i++; col++;
                continue;
            case '*':
                /* * / ** */
                if (i + 1 < len && source[i + 1] == '*') {
                    if (!emit_token(&tokens, &result.count, &cap,
                                    TK_POWER, source + i, 2, start_line, start_col, norm_source_map, norm_source_map_size, offset_map, offset_map_size, start_offset)) {
                        result.error_code = -1;
                        result.error_msg = str_dup_n("Memory allocation failed", strlen("Memory allocation failed"));
                        goto fail;
                    }
                    i += 2; col += 2;
                } else {
                    if (!emit_token(&tokens, &result.count, &cap,
                                    TK_STAR, source + i, 1, start_line, start_col, norm_source_map, norm_source_map_size, offset_map, offset_map_size, start_offset)) {
                        result.error_code = -1;
                        result.error_msg = str_dup_n("Memory allocation failed", strlen("Memory allocation failed"));
                        goto fail;
                    }
                    i++; col++;
                }
                continue;
            case '/':
                if (!emit_token(&tokens, &result.count, &cap,
                                TK_SLASH, source + i, 1, start_line, start_col, norm_source_map, norm_source_map_size, offset_map, offset_map_size, start_offset)) {
                    result.error_code = -1;
                    result.error_msg = str_dup_n("Memory allocation failed", strlen("Memory allocation failed"));
                    goto fail;
                }
                i++; col++;
                continue;
            case '%':
                if (!emit_token(&tokens, &result.count, &cap,
                                TK_PERCENT, source + i, 1, start_line, start_col, norm_source_map, norm_source_map_size, offset_map, offset_map_size, start_offset)) {
                    result.error_code = -1;
                    result.error_msg = str_dup_n("Memory allocation failed", strlen("Memory allocation failed"));
                    goto fail;
                }
                i++; col++;
                continue;
            case '^':
                /* 位异或 */
                if (!emit_token(&tokens, &result.count, &cap,
                                TK_BIT_XOR, source + i, 1, start_line, start_col, norm_source_map, norm_source_map_size, offset_map, offset_map_size, start_offset)) {
                    result.error_code = -1;
                    result.error_msg = str_dup_n("Memory allocation failed", strlen("Memory allocation failed"));
                    goto fail;
                }
                i++; col++;
                continue;
            case ';':
                if (!emit_token(&tokens, &result.count, &cap,
                                TK_SEMI, source + i, 1, start_line, start_col, norm_source_map, norm_source_map_size, offset_map, offset_map_size, start_offset)) {
                    result.error_code = -1;
                    result.error_msg = str_dup_n("Memory allocation failed", strlen("Memory allocation failed"));
                    goto fail;
                }
                i++; col++;
                continue;
            case ',':
                if (!emit_token(&tokens, &result.count, &cap,
                                TK_COMMA, source + i, 1, start_line, start_col, norm_source_map, norm_source_map_size, offset_map, offset_map_size, start_offset)) {
                    result.error_code = -1;
                    result.error_msg = str_dup_n("Memory allocation failed", strlen("Memory allocation failed"));
                    goto fail;
                }
                i++; col++;
                continue;
            case '(':
                if (!emit_token(&tokens, &result.count, &cap,
                                TK_L_PAREN, source + i, 1, start_line, start_col, norm_source_map, norm_source_map_size, offset_map, offset_map_size, start_offset)) {
                    result.error_code = -1;
                    result.error_msg = str_dup_n("Memory allocation failed", strlen("Memory allocation failed"));
                    goto fail;
                }
                i++; col++;
                continue;
            case ')':
                if (!emit_token(&tokens, &result.count, &cap,
                                TK_R_PAREN, source + i, 1, start_line, start_col, norm_source_map, norm_source_map_size, offset_map, offset_map_size, start_offset)) {
                    result.error_code = -1;
                    result.error_msg = str_dup_n("Memory allocation failed", strlen("Memory allocation failed"));
                    goto fail;
                }
                i++; col++;
                continue;
            case '{':
                if (!emit_token(&tokens, &result.count, &cap,
                                TK_L_BRACE, source + i, 1, start_line, start_col, norm_source_map, norm_source_map_size, offset_map, offset_map_size, start_offset)) {
                    result.error_code = -1;
                    result.error_msg = str_dup_n("Memory allocation failed", strlen("Memory allocation failed"));
                    goto fail;
                }
                i++; col++;
                continue;
            case '}':
                if (!emit_token(&tokens, &result.count, &cap,
                                TK_R_BRACE, source + i, 1, start_line, start_col, norm_source_map, norm_source_map_size, offset_map, offset_map_size, start_offset)) {
                    result.error_code = -1;
                    result.error_msg = str_dup_n("Memory allocation failed", strlen("Memory allocation failed"));
                    goto fail;
                }
                i++; col++;
                continue;
            case '[':
                if (!emit_token(&tokens, &result.count, &cap,
                                TK_L_BRACKET, source + i, 1, start_line, start_col, norm_source_map, norm_source_map_size, offset_map, offset_map_size, start_offset)) {
                    result.error_code = -1;
                    result.error_msg = str_dup_n("Memory allocation failed", strlen("Memory allocation failed"));
                    goto fail;
                }
                i++; col++;
                continue;
            case ']':
                if (!emit_token(&tokens, &result.count, &cap,
                                TK_R_BRACKET, source + i, 1, start_line, start_col, norm_source_map, norm_source_map_size, offset_map, offset_map_size, start_offset)) {
                    result.error_code = -1;
                    result.error_msg = str_dup_n("Memory allocation failed", strlen("Memory allocation failed"));
                    goto fail;
                }
                i++; col++;
                continue;
            default:
                break;
        }

        /* 未知字符 */
        result.error_code = 1;
        result.error_msg = make_unexpected_char_msg(c);
        goto fail;
    }

    /* 注意：不再生成 EOF token，语法结束完全靠 ';' 和 token 数组长度 */
    result.tokens = tokens;
    result.error_code = 0;
    result.error_msg = NULL;
    return result;

fail:
    if (tokens) {
        for (size_t k = 0; k < result.count; k++) {
            free(tokens[k].lexeme);
        }
        free(tokens);
    }
    result.tokens = NULL;
    result.count = 0;
    return result;
}

/* ========== 释放 / 打印 ========== */

void lexer_result_free(LexerResult* result) {
    if (!result) return;
    if (result->tokens) {
        for (size_t i = 0; i < result->count; i++) {
            free(result->tokens[i].lexeme);
        }
        free(result->tokens);
        result->tokens = NULL;
    }
    if (result->error_msg) {
        free(result->error_msg);
        result->error_msg = NULL;
    }
    result->count = 0;
    result->error_code = 0;
}

void lexer_print_tokens(const LexerResult* result, FILE* out) {
    if (!result || !out) return;
    for (size_t i = 0; i < result->count; i++) {
        const Token* t = &result->tokens[i];

        /* 现在不生成 EOF；即便将来生成，也不在调试输出里打印 EOF */
        if (t->kind == TK_EOF) {
            continue;
        }

        if (t->orig_line == 0) {
            // 合成token
            fprintf(out, "%s\t\"%s\"\t(synthetic)\n",
                    token_kind_name(t->kind),
                    t->lexeme ? t->lexeme : "");
        } else {
            // 原始位置格式：行:列+长度
            fprintf(out, "%s\t\"%s\"\t%d:%d+%d\n",
                    token_kind_name(t->kind),
                    t->lexeme ? t->lexeme : "",
                    t->orig_line,
                    t->orig_column,
                    t->orig_length);
        }
    }
}
