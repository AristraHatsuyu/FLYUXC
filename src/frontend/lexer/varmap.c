#include "flyuxc/frontend/varmap.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>

/* ========== Utility functions ========== */

static int is_space_c(int c) {
    return c==' ' || c=='\t' || c=='\n' || c=='\r' || c=='\v' || c=='\f';
}

static int is_ident_char(int c) {
    /* Allow ASCII letters, digits, _, and all non-ASCII bytes (emoji etc.) */
    return (c == '_' || isalnum((unsigned char)c) || (unsigned char)c >= 0x80);
}

static int is_ident_start(int c) {
    /* Allow starting with letters, _, non-ASCII; digits not allowed at start */
    return (c == '_' || isalpha((unsigned char)c) || (unsigned char)c >= 0x80);
}

/* Simple string copy, returns NULL on error */
static char* str_dup_n(const char* s, size_t len) {
    char* out = (char*)malloc(len + 1);
    if (!out) return NULL;
    memcpy(out, s, len);
    out[len] = '\0';
    return out;
}

/**
 * Get a specific line from original source code using source map
 * Returns a newly allocated string containing the line (caller must free)
 */
static char* get_original_source_line(const char* original_source, int line_num) {
    if (!original_source || line_num < 1) return NULL;
    
    const char* p = original_source;
    int current_line = 1;
    
    // Find the start of the target line
    while (*p && current_line < line_num) {
        if (*p == '\n') {
            current_line++;
        }
        p++;
    }
    
    if (!*p && current_line < line_num) return NULL;
    
    // Find the end of the line
    const char* line_start = p;
    while (*p && *p != '\n') {
        p++;
    }
    
    size_t line_len = p - line_start;
    char* line = malloc(line_len + 1);
    if (!line) return NULL;
    
    memcpy(line, line_start, line_len);
    line[line_len] = '\0';
    return line;
}

/**
 * Create a formatted error message with source line and position indicator
 */
static char* create_varmap_formatted_error(const char* original_source,
                                           int line, int column,
                                           const char* keyword,
                                           const char* flyux_equiv,
                                           const char* suggestion) {
    char* src_line = get_original_source_line(original_source, line);
    
    // Build the error message with proper formatting
    size_t buf_size = 1024;
    if (src_line) buf_size += strlen(src_line);
    if (suggestion) buf_size += strlen(suggestion);
    
    char* error_buf = malloc(buf_size);
    if (!error_buf) {
        free(src_line);
        return NULL;
    }
    
    // Create position indicator (spaces followed by ^)
    char indicator[256] = {0};
    int spaces = column - 1;
    if (spaces > 200) spaces = 200;  // Limit to prevent buffer overflow
    for (int i = 0; i < spaces; i++) {
        indicator[i] = ' ';
    }
    indicator[spaces] = '^';
    indicator[spaces + 1] = '\0';
    
    if (flyux_equiv != NULL) {
        snprintf(error_buf, buf_size,
                 "Error at line %d, column %d: Invalid keyword '%s'\n"
                 "    %d | %s\n"
                 "      | %s\n"
                 "  Hint: FLYUX uses '%s' instead\n"
                 "  Suggestion: %s",
                 line, column, keyword,
                 line, src_line ? src_line : "(unable to read line)",
                 indicator,
                 flyux_equiv, suggestion);
    } else {
        snprintf(error_buf, buf_size,
                 "Error at line %d, column %d: Invalid keyword '%s'\n"
                 "    %d | %s\n"
                 "      | %s\n"
                 "  Suggestion: %s",
                 line, column, keyword,
                 line, src_line ? src_line : "(unable to read line)",
                 indicator,
                 suggestion);
    }
    
    free(src_line);
    return error_buf;
}

/**
 * Create a formatted error message for invalid statement types (X>)
 */
static char* create_varmap_stmt_type_error(const char* original_source,
                                           int line, int column,
                                           char stmt_char) {
    char* src_line = get_original_source_line(original_source, line);
    
    size_t buf_size = 1024;
    if (src_line) buf_size += strlen(src_line);
    
    char* error_buf = malloc(buf_size);
    if (!error_buf) {
        free(src_line);
        return NULL;
    }
    
    // Create position indicator
    char indicator[256] = {0};
    int spaces = column - 1;
    if (spaces > 200) spaces = 200;
    for (int i = 0; i < spaces; i++) {
        indicator[i] = ' ';
    }
    indicator[spaces] = '^';
    indicator[spaces + 1] = '\0';
    
    snprintf(error_buf, buf_size,
             "Error at line %d, column %d: Invalid statement type '%c>'\n"
             "    %d | %s\n"
             "      | %s\n"
             "  Hint: FLYUX supports these statement types:\n"
             "    L> - loop statement\n"
             "    R> - return statement\n"
             "    B> - break statement\n"
             "    N> - next/continue statement\n"
             "    T> - try/exception handling",
             line, column, stmt_char,
             line, src_line ? src_line : "(unable to read line)",
             indicator);
    
    free(src_line);
    return error_buf;
}

/* 生成形如 "_00001" 的映射名 */
static char* generate_mapped_name(size_t index) {
    char buf[32];
    /* 5 位宽，不够你再改 */
    snprintf(buf, sizeof(buf), "_%05zu", index);
    size_t len = strlen(buf);
    char* out = (char*)malloc(len + 1);
    if (!out) return NULL;
    memcpy(out, buf, len + 1);
    return out;
}

/* 识别保留标识符（关键字 / 类型名 / 特殊字面量 / 特殊保留 main） */
static int is_reserved_identifier(const char* name, size_t len) {
    /* 关键字 if */
    if (len == 2 && name[0]=='i' && name[1]=='f') return 1;

    /* self 关键字（对象方法隐式参数） */
    if (len == 4 && name[0]=='s' && name[1]=='e' && name[2]=='l' && name[3]=='f') return 1;

    /* 注意: break 和 next 已不再是保留字，改用 B> 和 N> 语法 */

    /* L / R / T / B / N 用于 L> / R> / T> / B> / N>，简单起见直接视为保留 */
    if (len == 1 && (name[0] == 'L' || name[0] == 'R' || name[0] == 'T' || name[0] == 'B' || name[0] == 'N')) return 1;

    /* 类型名 */
    if (len == 3 && name[0]=='n' && name[1]=='u' && name[2]=='m') return 1; // num
    if (len == 3 && name[0]=='s' && name[1]=='t' && name[2]=='r') return 1; // str
    if (len == 2 && name[0]=='b' && name[1]=='l')                 return 1; // bl
    if (len == 3 && name[0]=='o' && name[1]=='b' && name[2]=='j') return 1; // obj
    if (len == 4 && name[0]=='f' && name[1]=='u' && name[2]=='n' && name[3]=='c') return 1; // func

    /* 布尔 / 特殊值 */
    if (len == 4 && name[0]=='t' && name[1]=='r' && name[2]=='u' && name[3]=='e')  return 1; // true
    if (len == 5 && name[0]=='f' && name[1]=='a' && name[2]=='l' && name[3]=='s' && name[4]=='e') return 1; // false
    if (len == 4 && name[0]=='n' && name[1]=='u' && name[2]=='l' && name[3]=='l')  return 1; // null
    if (len == 5 && name[0]=='u' && name[1]=='n' && name[2]=='d' && name[3]=='e' && name[4]=='f') return 1; // undef

    /* main 函数名保留，避免入口被改名 */
    if (len == 4 && name[0]=='m' && name[1]=='a' && name[2]=='i' && name[3]=='n') return 1;

    return 0;
}

/* 内置标识符（如内置函数）——不会被加入映射表，也不会替换 */
/* 
 * FLYUX 内置函数完整列表
 * 与实际实现保持一致
 * 最后更新: 2025-11-19
 */
static const char* BUILTIN_IDENTIFIERS[] = {
    /* 输入输出 & 文件I/O (6 -> 23) */
    "print", "println", "printf", "input", 
    "readFile", "writeFile", "appendFile",
    "readBytes", "writeBytes",
    "fileExists", "deleteFile", "getFileSize",
    "readLines", "renameFile", "copyFile",
    "createDir", "removeDir", "listDir", "dirExists",
    "parseJSON", "toJSON",
    
    /* 字符串操作 (15) */
    "substr", "indexOf", "replace", "split", "join",
    "upper", "lower", "trim", "startsWith", "endsWith", "contains",
    "len", "charAt",
    
    /* 数学函数 (13) */
    "abs", "floor", "ceil", "round", "sqrt", "pow",
    "min", "max", "random", "randomInt",
    "isNaN", "isFinite", "clamp",
    
    /* 数组操作 (19) */
    "push", "pop", "shift", "unshift", "slice", "concat",
    "reverse", "sort", "filter", "map", "reduce", "find", "includes",
    
    /* 对象操作 (10) */
    "keys", "values", "entries", "hasKey", "merge", "clone", "deepClone",
    "setField", "deleteField", "hasField",
    
    /* 类型转换和检查 (15) */
    "toNum", "toStr", "toBl", "toInt", "toFloat", "typeOf",
    "isNum", "isStr", "isBl", "isArr", "isObj", "isNull", "isUndef",
    
    /* 时间函数 (4) */
    "now", "sleep", "time", "date",
    
    /* 系统操作 (4) */
    "exit", "getEnv", "setEnv",
    
    /* 实用工具 (3) */
    "assert", "exit", "range",
    
    NULL  /* 结束标记 */
};

static int is_builtin_identifier(const char* name, size_t len) {
    for (int i = 0; BUILTIN_IDENTIFIERS[i] != NULL; i++) {
        size_t builtin_len = strlen(BUILTIN_IDENTIFIERS[i]);
        if (len == builtin_len && memcmp(name, BUILTIN_IDENTIFIERS[i], len) == 0) {
            return 1;
        }
    }
    return 0;
}

/* Non-FLYUX syntax keyword detection table */
typedef struct {
    const char* keyword;        /* Invalid keyword */
    const char* flyux_equiv;    /* FLYUX equivalent */
    const char* suggestion;     /* Suggestion message */
} InvalidKeywordInfo;

static const InvalidKeywordInfo INVALID_KEYWORDS[] = {
    /* Variable declaration keywords */
    {"let", ":=", "Use ':=' to declare variables, e.g.: x := 10"},
    {"const", ":=", "Use ':=' to declare variables, e.g.: x := 10"},
    {"var", ":=", "Use ':=' to declare variables, e.g.: x := 10"},
    
    /* Control flow keywords */
    {"else", "{} {}", "FLYUX has no 'else' keyword, use the second block of if, e.g.: if (cond) { true_branch } { false_branch }"},
    {"while", "L>", "Use 'L>' for loops, e.g.: L> (true_condition) { body }"},
    {"for", "L>", "Use 'L>' for loops, e.g.: L> (i; i < 10; i++) { body }"},
    {"do", "L>", "Use 'L>' for loops, e.g.: L> (condition) { body }"},
    {"switch", "if", "FLYUX has no 'switch' statement, use multiple if conditions"},
    {"case", "if", "FLYUX has no 'case' statement, use multiple if conditions"},
    {"default", "if", "FLYUX has no 'default' statement, use the else branch of if"},
    
    /* Jump keywords */
    {"return", "R>", "Use 'R>' to return values, e.g.: R> value"},
    {"break", "B>", "Use 'B>' to break out of loops, e.g.: B> or B> @label"},
    {"continue", "N>", "Use 'N>' to continue to next iteration, e.g.: N> or N> @label"},
    
    /* Function definition keywords */
    {"function", ":=", "FLYUX uses ':=' to define functions, e.g.: add := (a, b) { R> a + b }"},
    {"def", ":=", "FLYUX uses ':=' to define functions, e.g.: add := (a, b) { R> a + b }"},
    {"fn", ":=", "FLYUX uses ':=' to define functions, e.g.: add := (a, b) { R> a + b }"},
    {"lambda", ":=", "FLYUX uses ':=' to define functions, e.g.: add := (a, b) { R> a + b }"},
    
    /* Exception handling keywords */
    {"try", "T>", "Use 'T>' for exception handling, e.g.: T> { risky_code } (e) { handle_error }"},
    {"catch", "T>", "Use 'T>' for exception handling, e.g.: T> { risky_code } (e) { handle_error }"},
    {"finally", NULL, "FLYUX's T> structure has no finally block"},
    {"throw", NULL, "FLYUX uses built-in error object mechanism"},
    {"raise", NULL, "FLYUX uses built-in error object mechanism"},
    
    /* Class/object keywords */
    {"class", "obj", "FLYUX uses 'obj' type to create objects, e.g.: person := {name: \"John\", age: 30}"},
    {"new", NULL, "FLYUX does not need 'new' keyword, create objects directly"},
    {"this", "self", "FLYUX uses 'self' keyword in object methods, e.g.: self.property"},
    /* self is now a valid keyword */
    
    /* Module keywords */
    {"import", NULL, "FLYUX currently does not support module imports"},
    {"export", NULL, "FLYUX currently does not support module exports"},
    {"from", NULL, "FLYUX currently does not support module system"},
    {"require", NULL, "FLYUX currently does not support module system"},
    
    /* Other common keywords */
    {"async", NULL, "FLYUX currently does not support async programming"},
    {"await", NULL, "FLYUX currently does not support async programming"},
    {"yield", NULL, "FLYUX currently does not support generators"},
    {"void", NULL, "FLYUX does not use 'void' type"},
    {"int", "num", "FLYUX uses 'num' type to represent numbers"},
    {"float", "num", "FLYUX uses 'num' type to represent numbers"},
    {"double", "num", "FLYUX uses 'num' type to represent numbers"},
    {"string", "str", "FLYUX uses 'str' type to represent strings"},
    {"boolean", "bl", "FLYUX uses 'bl' type to represent booleans"},
    {"bool", "bl", "FLYUX uses 'bl' type to represent booleans"},
    {"undefined", "undef", "FLYUX uses 'undef' to represent undefined values"},
    {"nil", "null", "FLYUX uses 'null' to represent null values"},
    {"None", "null", "FLYUX uses 'null' to represent null values"},
    {"True", "true", "FLYUX uses lowercase 'true'"},
    {"False", "false", "FLYUX uses lowercase 'false'"},
    {"elif", "if", "FLYUX has no 'elif', use nested if statements"},
    {"elsif", "if", "FLYUX has no 'elsif', use nested if statements"},
    {"unless", "if", "FLYUX has no 'unless', use if (!condition)"},
    {"until", "L>", "FLYUX has no 'until', use 'L>' loop"},
    {"in", ":", "FLYUX foreach loop uses ':', e.g.: L> (arr : item) { }"},
    {"of", ":", "FLYUX foreach loop uses ':', e.g.: L> (arr : item) { }"},
    {"and", "&&", "FLYUX uses '&&' for logical AND"},
    {"or", "||", "FLYUX uses '||' for logical OR"},
    {"not", "!", "FLYUX uses '!' for logical NOT"},
    
    {NULL, NULL, NULL}  /* End marker */
};

/* Check for invalid keywords, return info struct pointer, NULL if not found */
static const InvalidKeywordInfo* check_invalid_keyword(const char* name, size_t len) {
    for (int i = 0; INVALID_KEYWORDS[i].keyword != NULL; i++) {
        size_t kw_len = strlen(INVALID_KEYWORDS[i].keyword);
        if (len == kw_len && memcmp(name, INVALID_KEYWORDS[i].keyword, len) == 0) {
            return &INVALID_KEYWORDS[i];
        }
    }
    return NULL;
}

/* 动态数组扩容 */
static int ensure_entry_capacity(VarMapEntry** arr, size_t* cap, size_t needed) {
    if (*cap >= needed) return 1;
    size_t new_cap = (*cap == 0) ? 8 : (*cap * 2);
    while (new_cap < needed) new_cap *= 2;
    VarMapEntry* p = (VarMapEntry*)realloc(*arr, new_cap * sizeof(VarMapEntry));
    if (!p) return 0;
    *arr = p;
    *cap = new_cap;
    return 1;
}

/* 在映射表中查找名字，返回索引或 -1 */
static int varmap_find(const VarMapEntry* entries, size_t count, const char* name, size_t len) {
    for (size_t i = 0; i < count; i++) {
        if (strlen(entries[i].original) == len &&
            memcmp(entries[i].original, name, len) == 0) {
            return (int)i;
        }
    }
    return -1;
}

/* 向映射表添加新名字（调用前确保不存在同名） */
static int varmap_add(VarMapEntry** entries,
                      size_t* count,
                      size_t* cap,
                      const char* name,
                      size_t len,
                      VarKind kind,
                      size_t index_for_name) {
    if (!ensure_entry_capacity(entries, cap, *count + 1)) {
        return -1;
    }

    VarMapEntry* e = &((*entries)[*count]);
    e->original = str_dup_n(name, len);
    if (!e->original) return -1;

    e->mapped = generate_mapped_name(index_for_name);
    if (!e->mapped) {
        free(e->original);
        return -1;
    }

    e->kind = kind;
    e->first_line = 0;
    e->first_column = 0;

    (*count)++;
    return (int)(*count - 1);
}

/* 判断当前 identifier 是否像是 “类型注解/变量定义的名字”，而不是对象 key */
static int looks_like_typed_definition(const char* src, size_t len, size_t ident_end_index) {
    /* ident_end_index 是标识符结束位置之后的第一个字符下标
       即 src[ident_end_index] 若是 ':'，我们从 ':' 后开始检查 */
    if (ident_end_index >= len) return 0;
    if (src[ident_end_index] != ':') return 0;

    size_t p = ident_end_index + 1;
    /* 简单向前扫描，直到遇到 '=', ',', ';', '}', ')' 或字符串结束 */
    while (p < len) {
        char c = src[p];
        if (c == '=') {
            /* 在遇到终止符之前看到了 '='，像是 x:(type)=... 之类的定义 */
            return 1;
        }
        if (c == ',' || c == ';' || c == '}' || c == ')') {
            break;  /* 没有遇到 '='，更像是对象 key: value */
        }
        p++;
    }
    return 0;
}

/* ========== 辅助宏：同时写入 out 和 offset_map ========== */

/* 确保 out 和 offset_map 有足够容量 */
#define ENSURE_CAPACITY(needed) do { \
    while (out_idx + (needed) >= out_cap) { \
        out_cap *= 2; \
        char* new_out = (char*)realloc(out, out_cap); \
        if (!new_out) { \
            result.error_code = -1; \
            result.error_msg = str_dup_n("Memory allocation failed", 25); \
            free(out); free(offset_map); \
            for (size_t k = 0; k < entry_count; k++) { \
                free(entries[k].original); \
                free(entries[k].mapped); \
            } \
            free(entries); \
            return result; \
        } \
        out = new_out; \
    } \
    while (out_idx + (needed) >= offset_map_cap) { \
        offset_map_cap *= 2; \
        size_t* new_map = (size_t*)realloc(offset_map, offset_map_cap * sizeof(size_t)); \
        if (!new_map) { \
            result.error_code = -1; \
            result.error_msg = str_dup_n("Memory allocation failed for offset_map", 41); \
            free(out); free(offset_map); \
            for (size_t k = 0; k < entry_count; k++) { \
                free(entries[k].original); \
                free(entries[k].mapped); \
            } \
            free(entries); \
            return result; \
        } \
        offset_map = new_map; \
    } \
} while(0)

/* 写入单个字符到 out，同时记录 offset_map */
#define EMIT_CHAR(ch, src_offset) do { \
    ENSURE_CAPACITY(1); \
    out[out_idx] = (ch); \
    offset_map[out_idx] = (src_offset); \
    out_idx++; \
} while(0)

/* 写入多个字符（映射后的标识符），所有字符都映射到同一个源位置 */
#define EMIT_STR_MAPPED(str, str_len, src_offset) do { \
    ENSURE_CAPACITY(str_len); \
    memcpy(out + out_idx, str, str_len); \
    for (size_t _k = 0; _k < (str_len); _k++) { \
        offset_map[out_idx + _k] = (src_offset); \
    } \
    out_idx += (str_len); \
} while(0)

/* ========== 主映射逻辑 ========== */

VarMapResult flyux_varmap_process(const char* normalized_source,
                                  const SourceLocation* source_map,
                                  size_t source_map_size,
                                  const char* original_source) {
    VarMapResult result;
    result.mapped_source = NULL;
    result.entries = NULL;
    result.entry_count = 0;
    result.offset_map = NULL;
    result.offset_map_size = 0;
    result.error_msg = NULL;
    result.error_code = 0;

    if (!normalized_source) {
        result.error_code = -1;
        result.error_msg = str_dup_n("normalized_source is NULL", strlen("normalized_source is NULL"));
        return result;
    }

    size_t len = strlen(normalized_source);
    /* 映射结果字符串：保守估计 4 倍长度 */
    size_t out_cap = len * 4 + 16;
    char* out = (char*)malloc(out_cap);
    if (!out) {
        result.error_code = -1;
        result.error_msg = str_dup_n("Memory allocation failed", strlen("Memory allocation failed"));
        return result;
    }
    
    /* 同时分配 offset_map，在主循环中填充 */
    size_t offset_map_cap = out_cap;
    size_t* offset_map = (size_t*)malloc(offset_map_cap * sizeof(size_t));
    if (!offset_map) {
        free(out);
        result.error_code = -1;
        result.error_msg = str_dup_n("Memory allocation failed for offset_map", 41);
        return result;
    }

    VarMapEntry* entries = NULL;
    size_t entry_count = 0;
    size_t entry_cap = 0;
    size_t next_index = 1;  /* 用于生成 _00001, _00002, ... */

    int in_str = 0;
    char str_quote = 0;  /* 记录开启字符串的引号类型 */
    int escape = 0;
    size_t i = 0;
    size_t out_idx = 0;

    while (i < len) {
        char c = normalized_source[i];

        /* 字符串内照抄 */
        if (escape) {
            EMIT_CHAR(c, i);
            escape = 0;
            i++;
            continue;
        }

        if (c == '\\') {
            EMIT_CHAR(c, i);
            escape = 1;
            i++;
            continue;
        }

        if (!in_str && (c == '"' || c == 39)) {  /* 39 is '\'' */
            /* 进入字符串 */
            EMIT_CHAR(c, i);
            in_str = 1;
            str_quote = c;
            i++;
            continue;
        }

        if (in_str && c == str_quote) {
            /* 退出字符串 */
            EMIT_CHAR(c, i);
            in_str = 0;
            str_quote = 0;
            i++;
            continue;
        }

        if (in_str) {
            /* 字符串内容照抄 */
            EMIT_CHAR(c, i);
            i++;
            continue;
        }

        /* 识别标识符起点 */
        if (is_ident_start((unsigned char)c)) {
            size_t start = i;
            size_t j = i + 1;
            while (j < len && is_ident_char((unsigned char)normalized_source[j])) {
                j++;
            }
            size_t ident_len = j - start;
            const char* ident_start = normalized_source + start;
            
            /* Check for invalid X> format (single uppercase letter followed by >, not valid L>/R>/T>/B>/N>) */
            if (ident_len == 1 && isupper((unsigned char)ident_start[0]) 
                && j < len && normalized_source[j] == '>'
                && ident_start[0] != 'L' && ident_start[0] != 'R' && ident_start[0] != 'T' 
                && ident_start[0] != 'B' && ident_start[0] != 'N') {
                /* Find original position */
                int orig_line = 1, orig_col = 1;
                if (source_map && start < source_map_size && source_map[start].orig_line > 0) {
                    orig_line = source_map[start].orig_line;
                    orig_col = source_map[start].orig_column;
                }
                
                result.error_code = 1;
                result.error_msg = create_varmap_stmt_type_error(original_source, orig_line, orig_col, ident_start[0]);
                free(out);
                if (entries) {
                    for (size_t k = 0; k < entry_count; k++) {
                        free(entries[k].original);
                        free(entries[k].mapped);
                    }
                    free(entries);
                }
                return result;
            }
            /* 确认 token 边界：前一个/后一个字符不能是标识符字符 */
            if ( (start == 0 || !is_ident_char((unsigned char)normalized_source[start - 1])) &&
                 (j >= len || !is_ident_char((unsigned char)normalized_source[j])) ) {

                /* 检测非 FLYUX 关键字用法（不是作为变量名使用时才报错） */
                const InvalidKeywordInfo* inv_kw = check_invalid_keyword(ident_start, ident_len);
                if (inv_kw != NULL) {
                    /* 先检查这个关键字是否已经在映射表中（之前被定义过） */
                    int already_defined = varmap_find(entries, entry_count, ident_start, ident_len) >= 0;
                    
                    /* 检查是否是作为变量名使用 */
                    /* 在 varmap 阶段，空格已经被删除，所以直接检查后面的字符 */
                    char after_char = (j < len) ? normalized_source[j] : '\0';
                    int is_var_usage = 0;
                    
                    /* 特殊值关键字 - 永远不允许使用（即使作为表达式的一部分）*/
                    int is_special_value = (ident_len == 4 && memcmp(ident_start, "None", 4) == 0) ||
                                           (ident_len == 3 && memcmp(ident_start, "nil", 3) == 0) ||
                                           (ident_len == 9 && memcmp(ident_start, "undefined", 9) == 0) ||
                                           (ident_len == 4 && memcmp(ident_start, "True", 4) == 0) ||
                                           (ident_len == 5 && memcmp(ident_start, "False", 5) == 0);
                    
                    /* 如果是特殊值且没有被定义，直接报错 */
                    if (is_special_value && !already_defined) {
                        /* 只有当它被用作 := 左边时才允许（定义同名变量） */
                        if (!(after_char == ':' && j + 1 < len && normalized_source[j + 1] == '=')) {
                            /* Find original position */
                            int orig_line = 1, orig_col = 1;
                            if (source_map && start < source_map_size && source_map[start].orig_line > 0) {
                                orig_line = source_map[start].orig_line;
                                orig_col = source_map[start].orig_column;
                            }
                            
                            result.error_code = 1;
                            result.error_msg = create_varmap_formatted_error(original_source, orig_line, orig_col,
                                                                             inv_kw->keyword,
                                                                             inv_kw->flyux_equiv,
                                                                             inv_kw->suggestion);
                            free(out);
                            if (entries) {
                                for (size_t k = 0; k < entry_count; k++) {
                                    free(entries[k].original);
                                    free(entries[k].mapped);
                                }
                                free(entries);
                            }
                            return result;
                        }
                    }
                    
                    /* 如果已经被定义，则后续使用都是合法的 */
                    if (already_defined) {
                        is_var_usage = 1;
                    } else {
                        /* 对于 for/while/do，如果后面跟着 ( 是其他语言的循环语法，不算变量使用 */
                        int is_loop_keyword = (ident_len == 3 && memcmp(ident_start, "for", 3) == 0) ||
                                              (ident_len == 5 && memcmp(ident_start, "while", 5) == 0) ||
                                              (ident_len == 2 && memcmp(ident_start, "do", 2) == 0);
                        
                        if (after_char == ':' && j + 1 < len && normalized_source[j + 1] == '=') is_var_usage = 1;  // :=
                        else if (after_char == '=') is_var_usage = 1;   // = assignment
                        else if (after_char == '.') is_var_usage = 1;   // member access
                        else if (after_char == '[') is_var_usage = 1;   // index access
                        else if (after_char == '(' && !is_loop_keyword) is_var_usage = 1;   // function call (not for/while/do)
                        else if (after_char == '+' || after_char == '-' || after_char == '*' || after_char == '/') is_var_usage = 1;  // operators
                        else if (after_char == '<' || after_char == '>' || after_char == '!' || after_char == '&' || after_char == '|') is_var_usage = 1;  // comparison/logic
                        else if (after_char == ')' || after_char == ',' || after_char == ';' || after_char == '}') is_var_usage = 1;  // expression/statement end
                        else if (after_char == '?' || after_char == ':') is_var_usage = 1;  // ternary operator
                        else if (after_char == '\0') is_var_usage = 1;  // end of file
                    }
                    
                    if (!is_var_usage) {
                        /* Find original position */
                        int orig_line = 1, orig_col = 1;
                        if (source_map && start < source_map_size && source_map[start].orig_line > 0) {
                            orig_line = source_map[start].orig_line;
                            orig_col = source_map[start].orig_column;
                        }
                        
                        result.error_code = 1;
                        result.error_msg = create_varmap_formatted_error(original_source, orig_line, orig_col,
                                                                         inv_kw->keyword,
                                                                         inv_kw->flyux_equiv,
                                                                         inv_kw->suggestion);
                        free(out);
                        if (entries) {
                            for (size_t k = 0; k < entry_count; k++) {
                                free(entries[k].original);
                                free(entries[k].mapped);
                            }
                            free(entries);
                        }
                        return result;
                    }
                }

                /* Reserved/builtin identifiers: don't participate in mapping */
                int reserved = is_reserved_identifier(ident_start, ident_len) ||
                               is_builtin_identifier(ident_start, ident_len);

                /* 上下文判断：成员访问 / 方法名 / 对象 key */
                char before = (start > 0) ? normalized_source[start - 1] : '\0';
                char after  = (j < len) ? normalized_source[j] : '\0';

                int is_method_after_chain = 0;   /* .>methodName */
                int is_property_access    = 0;   /* obj.property or obj.@property */
                int is_spread_expr        = 0;   /* ...expr spread 操作符 */
                
                if (before == '>' && start >= 2 && normalized_source[start - 2] == '.') {
                    /* ".>method" 场景 */
                    is_method_after_chain = 1;
                } else if (before == '@' && start >= 2 && normalized_source[start - 2] == '.') {
                    /* ".@property" 解绑属性访问 */
                    is_property_access = 1;
                } else if (before == '.') {
                    /* 检查是否是 spread 语法 "..." */
                    /* ...obj 的情况: 前面是 . 且前两个字符也是 . */
                    if (start >= 3 && normalized_source[start - 2] == '.' && normalized_source[start - 3] == '.') {
                        /* 这是 spread 语法 ...identifier */
                        is_spread_expr = 1;
                    } else {
                        /* 普通属性访问 obj.prop */
                        is_property_access = 1;
                    }
                }

                int is_object_key = 0;
                if (after == ':') {
                    /* 区分类型注解/变量定义、对象 key、三元运算符、以及 foreach 循环中的迭代变量 */
                    // 检查是否是 foreach: L> (arr : item) 或 L>arr:item
                    // 向前搜索，看是否有 "L>" 后面跟 "(" 或直接跟标识符
                    int is_foreach = 0;
                    int is_ternary = 0;
                    
                    if (start >= 2) {
                        // 向前搜索 L> (，允许中间有空格
                        size_t k = start - 1;
                        while (k > 0 && (normalized_source[k] == ' ' || normalized_source[k] == '\t')) {
                            k--;
                        }
                        
                        // 情况1: L> (arr:item)
                        if (k > 0 && normalized_source[k] == '(') {
                            k--;
                            while (k > 0 && (normalized_source[k] == ' ' || normalized_source[k] == '\t')) {
                                k--;
                            }
                            if (k >= 1 && normalized_source[k] == '>' && normalized_source[k-1] == 'L') {
                                is_foreach = 1;
                            }
                        }
                        // 情况2: L>arr:item (不带括号)
                        // k 此时指向 arr 的前一个字符（应该是 >）
                        else if (k >= 1 && normalized_source[k] == '>') {
                            if (k >= 1 && normalized_source[k-1] == 'L') {
                                is_foreach = 1;
                            }
                        }
                    }
                    
                    // 检查是否是三元运算符的一部分：向前搜索 ?
                    // 三元表达式: cond ? true_val : false_val
                    // 如果在 : 之前找到 ? (在同一表达式中)，说明这是三元运算符
                    if (!is_foreach && start > 0) {
                        int paren_depth = 0;
                        int brace_depth = 0;
                        int bracket_depth = 0;
                        for (size_t k = start - 1; k > 0; k--) {
                            char ch = normalized_source[k];
                            // 跳过括号内部
                            if (ch == ')') paren_depth++;
                            else if (ch == '(') {
                                if (paren_depth > 0) paren_depth--;
                                else break;  // 到达表达式开始
                            }
                            else if (ch == '}') brace_depth++;
                            else if (ch == '{') {
                                if (brace_depth > 0) brace_depth--;
                                else break;  // 对象字面量开始
                            }
                            else if (ch == ']') bracket_depth++;
                            else if (ch == '[') {
                                if (bracket_depth > 0) bracket_depth--;
                                else break;
                            }
                            else if (paren_depth == 0 && brace_depth == 0 && bracket_depth == 0) {
                                if (ch == '?') {
                                    is_ternary = 1;
                                    break;
                                }
                                // 到达语句边界就停止
                                if (ch == ';' || ch == ':' && k > 0 && normalized_source[k-1] == '=') {
                                    break;
                                }
                            }
                        }
                    }
                    
                    if (!is_foreach && !is_ternary && !looks_like_typed_definition(normalized_source, len, j)) {
                        is_object_key = 1;
                    }
                }

                const char* replacement = ident_start;
                size_t replacement_len = ident_len;

                if (!reserved && !is_object_key) {
                    if (is_method_after_chain) {
                        /* .>method：若 method 在映射表中，则替换；否则保持原名 */
                        int idx = varmap_find(entries, entry_count, ident_start, ident_len);
                        if (idx >= 0) {
                            replacement = entries[idx].mapped;
                            replacement_len = strlen(replacement);
                        }
                        /* 如果没找到，不新增映射，保持原名（length 这类） */
                    } else if (!is_property_access) {
                        /* 普通变量/函数名：正常参与映射 */
                        int idx = varmap_find(entries, entry_count, ident_start, ident_len);
                        if (idx < 0) {
                            int add_idx = varmap_add(&entries,
                                                     &entry_count,
                                                     &entry_cap,
                                                     ident_start,
                                                     ident_len,
                                                     VARKIND_UNKNOWN,
                                                     next_index++);
                            if (add_idx < 0) {
                                result.error_code = -1;
                                result.error_msg = str_dup_n("Memory allocation failed", strlen("Memory allocation failed"));
                                free(out);
                                for (size_t k = 0; k < entry_count; k++) {
                                    free(entries[k].original);
                                    free(entries[k].mapped);
                                }
                                free(entries);
                                return result;
                            }
                            idx = add_idx;
                        }
                        replacement = entries[idx].mapped;
                        replacement_len = strlen(replacement);
                    }
                    /* is_property_access：对象属性名一律不改名 */
                }

                /* 写出 token（要么映射名，要么原名）
                 * 使用 EMIT_STR_MAPPED 将所有输出字符映射到原始标识符的起始位置
                 */
                EMIT_STR_MAPPED(replacement, replacement_len, start);

                i = j;
                continue;
            }

            /* 不满足 token 边界，按普通字符抄 */
            EMIT_CHAR(c, i);
            i++;
            continue;
        }

        /* 其他字符照抄 */
        EMIT_CHAR(c, i);
        i++;
    }

    out[out_idx] = '\0';

    // offset_map 已在主循环中构建完成

    result.mapped_source = out;
    result.entries = entries;
    result.entry_count = entry_count;
    result.offset_map = offset_map;
    result.offset_map_size = out_idx;
    result.error_code = 0;
    result.error_msg = NULL;

    return result;
}

/* ========== 释放与打印 ========== */

void varmap_result_free(VarMapResult* result) {
    if (!result) return;
    if (result->mapped_source) {
        free(result->mapped_source);
        result->mapped_source = NULL;
    }
    if (result->offset_map) {
        free(result->offset_map);
        result->offset_map = NULL;
    }
    if (result->entries) {
        for (size_t i = 0; i < result->entry_count; i++) {
            free(result->entries[i].original);
            free(result->entries[i].mapped);
        }
        free(result->entries);
        result->entries = NULL;
    }
    if (result->error_msg) {
        free(result->error_msg);
        result->error_msg = NULL;
    }
    result->entry_count = 0;
    result->error_code = 0;
}

void varmap_print_table(const VarMapResult* result, FILE* out) {
    if (!result || !out) return;
    for (size_t i = 0; i < result->entry_count; i++) {
        const VarMapEntry* e = &result->entries[i];
        const char* kind_str = "UNKNOWN";
        switch (e->kind) {
            case VARKIND_LOCAL:   kind_str = "LOCAL"; break;
            case VARKIND_PARAM:   kind_str = "PARAM"; break;
            case VARKIND_GLOBAL:  kind_str = "GLOBAL"; break;
            case VARKIND_UNKNOWN: default: break;
        }
        fprintf(out, "[%zu] %s -> %s (%s)\n",
                i + 1,
                e->original ? e->original : "(null)",
                e->mapped ? e->mapped : "(null)",
                kind_str);
    }
}
