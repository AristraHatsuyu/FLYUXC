// normalize.c
#include "flyuxc/frontend/normalize.h"
#include "flyuxc/error.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>

/**
 * 外部模块声明
 */
extern char* normalize_remove_comments(const char* input);
extern Statement* normalize_split_statements(const char* input, int* stmt_count);
extern void normalize_filter_expressions(Statement* statements, int* count);
extern char* normalize_statement_content(const char* stmt);

/**
 * 检查字符是否可以作为标识符的开始
 * 允许：字母、下划线、中文、emoji等非ASCII字符
 */
static int is_valid_identifier_start(const char* str, int* bytes_consumed) {
    unsigned char c = (unsigned char)str[0];
    
    // ASCII字母或下划线
    if (isalpha(c) || c == '_') {
        *bytes_consumed = 1;
        return 1;
    }
    
    // 数字不能作为开始
    if (isdigit(c)) {
        *bytes_consumed = 1;
        return 0;
    }
    
    // UTF-8多字节字符（中文、emoji等）
    if (c >= 0x80) {
        // 简单检查：所有非ASCII字符都允许
        // 计算UTF-8字符的字节数
        if ((c & 0xE0) == 0xC0) {
            *bytes_consumed = 2;
        } else if ((c & 0xF0) == 0xE0) {
            *bytes_consumed = 3;
        } else if ((c & 0xF8) == 0xF0) {
            *bytes_consumed = 4;
        } else {
            *bytes_consumed = 1;
        }
        return 1;
    }
    
    // 其他字符不允许
    *bytes_consumed = 1;
    return 0;
}

/**
 * 验证变量声明：检查 := 左侧是否为有效标识符
 * 如果发现无效标识符（如数字、字符串字面量等），返回错误信息
 */
static char* validate_variable_declarations(const char* source) {
    if (!source) return NULL;
    
    const char* p = source;
    int line = 1;
    int column = 1;
    
    while (*p) {
        // 跳过空白
        while (*p && isspace(*p)) {
            if (*p == '\n') {
                line++;
                column = 1;
            } else {
                column++;
            }
            p++;
        }
        
        if (!*p) break;
        
        // 检查是否遇到 :=
        if (p[0] == ':' && p[1] == '=') {
            // 回溯找到 := 左侧的token
            const char* left_end = p - 1;
            
            // 跳过 := 左侧的空白
            while (left_end >= source && isspace(*left_end)) {
                left_end--;
            }
            
            if (left_end < source) {
                // := 前面什么都没有
                static char error_buf[256];
                snprintf(error_buf, sizeof(error_buf), 
                         "Line %d: Missing identifier before ':='", line);
                return strdup(error_buf);
            }
            
            // 找到左侧token的开始
            const char* left_start = left_end;
            
            // 检查最后一个字符，判断token类型
            if (*left_end == ')' || *left_end == ']' || *left_end == '}') {
                // 可能是 arr[i] := x 或 obj.prop := x，这是合法的
                // 暂时跳过，让parser处理更复杂的情况
                p += 2;
                column += 2;
                continue;
            }
            
            if (*left_end == '"' || *left_end == '\'') {
                // 字符串字面量
                static char error_buf[256];
                snprintf(error_buf, sizeof(error_buf), 
                         "Line %d: String literal cannot be used as variable name", line);
                return strdup(error_buf);
            }
            
            // 向前扫描找到token开始
            while (left_start > source) {
                unsigned char c = (unsigned char)*(left_start - 1);
                if (isalnum(c) || c == '_' || c >= 0x80 || c == '.') {
                    left_start--;
                } else {
                    break;
                }
            }
            
            // 检查token开始字符
            int bytes_consumed = 0;
            if (!is_valid_identifier_start(left_start, &bytes_consumed)) {
                // 无效的标识符开始
                static char error_buf[256];
                
                // 判断是什么类型的无效token
                if (isdigit((unsigned char)*left_start)) {
                    snprintf(error_buf, sizeof(error_buf), 
                             "Line %d: Number literal cannot be used as variable name", line);
                } else if (*left_start == '"' || *left_start == '\'') {
                    snprintf(error_buf, sizeof(error_buf), 
                             "Line %d: String literal cannot be used as variable name", line);
                } else if (strncmp(left_start, "true", 4) == 0 || strncmp(left_start, "false", 5) == 0) {
                    snprintf(error_buf, sizeof(error_buf), 
                             "Line %d: Boolean literal cannot be used as variable name", line);
                } else {
                    snprintf(error_buf, sizeof(error_buf), 
                             "Line %d: Invalid character '%c' at start of identifier", line, *left_start);
                }
                return strdup(error_buf);
            }
            
            // 检查是否是布尔字面量
            int token_len = left_end - left_start + 1;
            if ((token_len == 4 && strncmp(left_start, "true", 4) == 0) ||
                (token_len == 5 && strncmp(left_start, "false", 5) == 0)) {
                static char error_buf[256];
                snprintf(error_buf, sizeof(error_buf), 
                         "Line %d: Boolean literal cannot be used as variable name", line);
                return strdup(error_buf);
            }
            
            p += 2;
            column += 2;
            continue;
        }
        
        // 跳过其他字符
        if (*p == '\n') {
            line++;
            column = 1;
        } else {
            column++;
        }
        p++;
    }
    
    return NULL;  // 验证通过
}

/**
 * Create a formatted error message using global error module
 * Directly outputs colored error using report_error_at, returns empty string
 * to signal error to caller without duplicating output
 */
static char* create_formatted_error(const char* source, int line, int column,
                                   const char* keyword, const char* flyux_equiv,
                                   const char* suggestion) {
    // 构建简短的主消息（不包含 Hint 和 Suggestion）
    char message[256];
    snprintf(message, sizeof(message), "Invalid keyword '%s'", keyword);
    
    // 使用全局错误接口输出带颜色的错误（包含源码行和指示符）
    int keyword_len = (int)strlen(keyword);
    report_error_at(ERR_ERROR, PHASE_LEXER, source, line, column, keyword_len, message);
    
    // 在源码行下方输出 Hint 和 Suggestion
    if (flyux_equiv != NULL) {
        fprintf(stderr, "\033[32m  Hint:\033[0m FLYUX uses '%s' instead\n", flyux_equiv);
    }
    fprintf(stderr, "\033[32m  Suggestion:\033[0m %s\n", suggestion);
    
    // 返回空字符串标记有错误（caller 会检查是否为非空）
    return strdup("");
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
    {"elif", "if", "FLYUX has no 'elif', use nested if statements"},
    {"elsif", "if", "FLYUX has no 'elsif', use nested if statements"},
    {"unless", "if", "FLYUX has no 'unless', use if (!condition)"},
    {"until", "L>", "FLYUX has no 'until', use 'L>' loop"},
    
    /* Python/Ruby logic keywords */
    {"and", "&&", "Use '&&' for logical AND in FLYUX"},
    {"or", "||", "Use '||' for logical OR in FLYUX"},
    {"not", "!", "Use '!' for logical NOT in FLYUX"},
    {"pass", NULL, "FLYUX has no 'pass' statement, use empty block {}"},
    
    /* Type keywords from other languages */
    {"int", NULL, "FLYUX is dynamically typed, no 'int' keyword needed"},
    {"float", NULL, "FLYUX is dynamically typed, no 'float' keyword needed"},
    {"double", NULL, "FLYUX is dynamically typed, no 'double' keyword needed"},
    {"boolean", NULL, "FLYUX is dynamically typed, no 'boolean' keyword needed"},
    {"string", NULL, "FLYUX is dynamically typed, no 'string' keyword needed"},
    {"char", NULL, "FLYUX is dynamically typed, no 'char' keyword needed"},
    
    /* Special values from other languages */
    {"None", "null", "Use 'null' for null values in FLYUX"},
    {"nil", "null", "Use 'null' for null values in FLYUX"},
    {"undefined", "null", "Use 'null' for undefined values in FLYUX"},
    {"True", "true", "Use lowercase 'true' for boolean true in FLYUX"},
    {"False", "false", "Use lowercase 'false' for boolean false in FLYUX"},
    
    {NULL, NULL, NULL}  /* 结束标记 */
};

/**
 * Check if a keyword has been defined as a variable before a given position
 * Looks for patterns like "keyword:=" or "keyword :=" before the current position
 */
static int is_keyword_defined_before(const char* source, const char* current_pos, 
                                     const char* keyword, int kw_len) {
    if (!source || !current_pos || !keyword || current_pos <= source) return 0;
    
    const char* p = source;
    
    while (p < current_pos) {
        // Skip strings
        if (*p == '"' || *p == '\'') {
            char quote = *p++;
            while (p < current_pos && *p != quote) {
                if (*p == '\\' && *(p+1)) p++;
                p++;
            }
            if (p < current_pos) p++;  // skip closing quote
            continue;
        }
        
        // Look for keyword
        if (isalpha(*p) || *p == '_') {
            const char* start = p;
            while (p < current_pos && (isalnum(*p) || *p == '_')) p++;
            int ident_len = p - start;
            
            if (ident_len == kw_len && strncmp(start, keyword, kw_len) == 0) {
                // Check if followed by := (with optional whitespace)
                const char* after = p;
                while (after < current_pos && isspace(*after) && *after != '\n') after++;
                if (after < current_pos - 1 && after[0] == ':' && after[1] == '=') {
                    return 1;  // Found keyword:= before current position
                }
            }
            continue;
        }
        
        p++;
    }
    return 0;
}

/**
 * 检测非法关键字
 * 在源码中查找非 FLYUX 的关键字，如 let, const, var, return, break, continue 等
 * @param source 去注释后的源码（用于检测）
 * @param original_source 原始源码（用于显示错误）
 * 返回错误消息（需要调用者释放），如果没有错误返回 NULL
 */
static char* check_invalid_keywords(const char* source, const char* original_source) {
    if (!source) return NULL;
    
    const char* p = source;
    int line = 1;
    int column = 1;
    int in_string = 0;
    char str_quote = 0;
    int escape = 0;
    
    // 用于错误显示的源码
    const char* display_source = original_source ? original_source : source;
    
    while (*p) {
        // 处理转义
        if (escape) {
            escape = 0;
            if (*p == '\n') { line++; column = 1; } else { column++; }
            p++;
            continue;
        }
        if (*p == '\\' && in_string) {
            escape = 1;
            column++;
            p++;
            continue;
        }
        
        // 处理字符串边界
        if (!in_string && (*p == '"' || *p == '\'')) {
            in_string = 1;
            str_quote = *p;
            column++;
            p++;
            continue;
        }
        if (in_string && *p == str_quote) {
            in_string = 0;
            str_quote = 0;
            column++;
            p++;
            continue;
        }
        if (in_string) {
            if (*p == '\n') { line++; column = 1; } else { column++; }
            p++;
            continue;
        }
        
        // 跳过空白
        if (isspace(*p)) {
            if (*p == '\n') { line++; column = 1; } else { column++; }
            p++;
            continue;
        }
        
        // 检查标识符
        if (isalpha(*p) || *p == '_') {
            const char* start = p;
            int start_line = line;
            int start_column = column;
            
            // 扫描标识符
            while (*p && (isalnum(*p) || *p == '_')) {
                column++;
                p++;
            }
            
            int ident_len = p - start;
            
            // 跳过标识符后的空白（但不跳过换行）
            const char* after = p;
            while (*after && isspace(*after) && *after != '\n') {
                after++;
            }
            
            // 检查非法关键字用法
            // 只有当这些关键字被用作其他语言的语法时才报错
            // 如果它们用作变量名（后面跟 := 或 = 或 . 等）则允许
            for (int i = 0; INVALID_KEYWORDS[i].keyword != NULL; i++) {
                int kw_len = strlen(INVALID_KEYWORDS[i].keyword);
                if (ident_len == kw_len && strncmp(start, INVALID_KEYWORDS[i].keyword, kw_len) == 0) {
                    int should_error = 0;
                    
                    // 检查是否是作为变量名使用
                    // 对于 break/continue，只有 := 或 = 或 ( 才算变量名使用
                    // 单独出现在一行（后面是换行/分号）是其他语言的语句用法
                    int is_break_continue = (strcmp(INVALID_KEYWORDS[i].keyword, "break") == 0 ||
                                             strcmp(INVALID_KEYWORDS[i].keyword, "continue") == 0);
                    
                    int is_var_usage = 0;
                    if (*after == ':' && *(after+1) == '=') is_var_usage = 1;  // :=
                    else if (*after == '=' && *(after+1) != '=') is_var_usage = 1;  // = 赋值（排除 ==）
                    
                    // 对于循环/条件关键字，( 不算函数调用
                    int is_loop_or_cond_keyword = (strcmp(INVALID_KEYWORDS[i].keyword, "for") == 0 ||
                                                   strcmp(INVALID_KEYWORDS[i].keyword, "while") == 0 ||
                                                   strcmp(INVALID_KEYWORDS[i].keyword, "do") == 0 ||
                                                   strcmp(INVALID_KEYWORDS[i].keyword, "unless") == 0 ||
                                                   strcmp(INVALID_KEYWORDS[i].keyword, "until") == 0 ||
                                                   strcmp(INVALID_KEYWORDS[i].keyword, "switch") == 0 ||
                                                   strcmp(INVALID_KEYWORDS[i].keyword, "catch") == 0);
                    
                    // 函数调用 - 但排除循环/条件关键字
                    if (*after == '(' && !is_loop_or_cond_keyword) is_var_usage = 1;
                    
                    // 对于非 break/continue 关键字，更多情况算作变量用法
                    if (!is_break_continue) {
                        if (*after == '.') is_var_usage = 1;                   // 成员访问
                        else if (*after == '[') is_var_usage = 1;              // 索引访问
                        else if (*after == ')') is_var_usage = 1;              // 表达式结尾
                        else if (*after == ',') is_var_usage = 1;              // 参数分隔
                        else if (*after == ';') is_var_usage = 1;              // 语句结尾
                        else if (*after == '\n' || *after == '\0') is_var_usage = 1; // 行尾
                        else if (*after == '}') is_var_usage = 1;              // 块结尾
                        else if (*after == '+' || *after == '-' || *after == '*' || *after == '/') is_var_usage = 1;  // 运算符
                        else if (*after == '<' || *after == '>' || *after == '!' || *after == '&' || *after == '|') is_var_usage = 1;  // 比较/逻辑
                    }
                    
                    if (is_var_usage) {
                        // Allow as variable name usage, skip check
                        break;
                    }
                    
                    // For break/continue, if followed by semicolon/newline/} etc., it's used as a statement
                    if (is_break_continue) {
                        if (*after == '\0' || *after == '\n' || *after == ';' || *after == '}') {
                            return create_formatted_error(display_source, start_line, start_column,
                                                          INVALID_KEYWORDS[i].keyword,
                                                          INVALID_KEYWORDS[i].flyux_equiv,
                                                          INVALID_KEYWORDS[i].suggestion);
                        }
                    }
                    
                    // For specific keywords, check their error usage patterns
                    if (strcmp(INVALID_KEYWORDS[i].keyword, "let") == 0 ||
                        strcmp(INVALID_KEYWORDS[i].keyword, "const") == 0 ||
                        strcmp(INVALID_KEYWORDS[i].keyword, "var") == 0) {
                        // "let x" / "const x" / "var x" pattern - followed by identifier
                        if (*after && (isalpha(*after) || *after == '_')) {
                            should_error = 1;
                        }
                    } else if (strcmp(INVALID_KEYWORDS[i].keyword, "return") == 0) {
                        // "return x" 模式 - 后面跟着表达式（非 :=）
                        if (*after && (isalpha(*after) || *after == '_' || isdigit(*after) || 
                                       *after == '"' || *after == '\'' || *after == '!' || *after == '-')) {
                            should_error = 1;
                        }
                    } else if (strcmp(INVALID_KEYWORDS[i].keyword, "function") == 0) {
                        // "function name" 或 "function(" 模式
                        if (*after && (isalpha(*after) || *after == '_')) {
                            should_error = 1;
                        }
                    } else if (strcmp(INVALID_KEYWORDS[i].keyword, "for") == 0 ||
                               strcmp(INVALID_KEYWORDS[i].keyword, "while") == 0 ||
                               strcmp(INVALID_KEYWORDS[i].keyword, "do") == 0) {
                        // "for (" / "while (" / "do {" 模式
                        // 但如果关键字之前被定义为变量（如 for:=），则允许调用 for(...)
                        if (*after == '(' || *after == '{') {
                            // 检查该关键字是否之前被定义
                            if (!is_keyword_defined_before(source, start, 
                                                           INVALID_KEYWORDS[i].keyword, kw_len)) {
                                should_error = 1;
                            }
                        }
                    } else if (strcmp(INVALID_KEYWORDS[i].keyword, "else") == 0) {
                        // "else" 后面直接跟 { 或 if
                        if (*after == '{' || (strncmp(after, "if", 2) == 0)) {
                            should_error = 1;
                        }
                    } else if (strcmp(INVALID_KEYWORDS[i].keyword, "try") == 0) {
                        // "try {" 模式
                        if (*after == '{') {
                            should_error = 1;
                        }
                    } else if (strcmp(INVALID_KEYWORDS[i].keyword, "catch") == 0) {
                        // "catch (" 模式
                        if (*after == '(') {
                            should_error = 1;
                        }
                    } else if (strcmp(INVALID_KEYWORDS[i].keyword, "class") == 0 ||
                               strcmp(INVALID_KEYWORDS[i].keyword, "import") == 0 ||
                               strcmp(INVALID_KEYWORDS[i].keyword, "export") == 0) {
                        // "class X" / "import X" / "export X" pattern
                        if (*after && (isalpha(*after) || *after == '_' || *after == '{' || *after == '*')) {
                            should_error = 1;
                        }
                    } else if (strcmp(INVALID_KEYWORDS[i].keyword, "and") == 0 ||
                               strcmp(INVALID_KEYWORDS[i].keyword, "or") == 0) {
                        // "x and y" / "x or y" - used between expressions
                        // Error when followed by identifier/number/expression start
                        if (*after && (isalpha(*after) || *after == '_' || isdigit(*after) ||
                                      *after == '(' || *after == '!' || *after == '"' || *after == '\'')) {
                            should_error = 1;
                        }
                    } else if (strcmp(INVALID_KEYWORDS[i].keyword, "not") == 0) {
                        // "not x" - used before expression
                        if (*after && (isalpha(*after) || *after == '_' || isdigit(*after) ||
                                      *after == '(' || *after == '!' || *after == '"' || *after == '\'')) {
                            should_error = 1;
                        }
                    } else if (strcmp(INVALID_KEYWORDS[i].keyword, "pass") == 0) {
                        // "pass" alone on a line or followed by nothing
                        if (*after == '\0' || *after == '\n' || *after == ';' || *after == '}') {
                            should_error = 1;
                        }
                    } else if (strcmp(INVALID_KEYWORDS[i].keyword, "int") == 0 ||
                               strcmp(INVALID_KEYWORDS[i].keyword, "float") == 0 ||
                               strcmp(INVALID_KEYWORDS[i].keyword, "double") == 0 ||
                               strcmp(INVALID_KEYWORDS[i].keyword, "boolean") == 0 ||
                               strcmp(INVALID_KEYWORDS[i].keyword, "string") == 0 ||
                               strcmp(INVALID_KEYWORDS[i].keyword, "char") == 0 ||
                               strcmp(INVALID_KEYWORDS[i].keyword, "void") == 0) {
                        // "int x" / "float x" etc. - type followed by identifier
                        if (*after && (isalpha(*after) || *after == '_')) {
                            should_error = 1;
                        }
                    } else if (strcmp(INVALID_KEYWORDS[i].keyword, "None") == 0 ||
                               strcmp(INVALID_KEYWORDS[i].keyword, "nil") == 0 ||
                               strcmp(INVALID_KEYWORDS[i].keyword, "undefined") == 0) {
                        // Python/Ruby null values - used in expressions/assignments
                        // Error when used as value (after := or = or in expression)
                        // Check: is there a := or = before this on the same line?
                        const char* line_start = start;
                        while (line_start > source && *(line_start - 1) != '\n') {
                            line_start--;
                        }
                        // Check for := or = before this keyword
                        const char* checker = line_start;
                        int found_assign = 0;
                        while (checker < start) {
                            if (*checker == ':' && *(checker + 1) == '=') {
                                found_assign = 1;
                                break;
                            } else if (*checker == '=' && (checker == source || *(checker - 1) != ':') && *(checker + 1) != '=') {
                                found_assign = 1;
                                break;
                            }
                            checker++;
                        }
                        if (found_assign) {
                            should_error = 1;
                        }
                    } else if (strcmp(INVALID_KEYWORDS[i].keyword, "True") == 0 ||
                               strcmp(INVALID_KEYWORDS[i].keyword, "False") == 0) {
                        // Python True/False - capital letters should be lowercase
                        // Always error when used (suggest lowercase)
                        should_error = 1;
                    } else if (strcmp(INVALID_KEYWORDS[i].keyword, "yield") == 0) {
                        // "yield x" - followed by expression
                        if (*after && (isalpha(*after) || *after == '_' || isdigit(*after) ||
                                      *after == '(' || *after == '"' || *after == '\'')) {
                            should_error = 1;
                        }
                    } else if (strcmp(INVALID_KEYWORDS[i].keyword, "async") == 0) {
                        // "async function" or "async def"
                        if (*after && (isalpha(*after) || *after == '_')) {
                            should_error = 1;
                        }
                    } else if (strcmp(INVALID_KEYWORDS[i].keyword, "await") == 0) {
                        // "await x" - followed by expression
                        if (*after && (isalpha(*after) || *after == '_' || isdigit(*after) || *after == '(')) {
                            should_error = 1;
                        }
                    } else if (strcmp(INVALID_KEYWORDS[i].keyword, "def") == 0) {
                        // "def name" - Python function definition
                        if (*after && (isalpha(*after) || *after == '_')) {
                            should_error = 1;
                        }
                    } else if (strcmp(INVALID_KEYWORDS[i].keyword, "unless") == 0 ||
                               strcmp(INVALID_KEYWORDS[i].keyword, "until") == 0) {
                        // "unless (" / "until (" - Ruby style conditionals/loops
                        if (*after == '(' || *after == '{') {
                            should_error = 1;
                        }
                    } else if (strcmp(INVALID_KEYWORDS[i].keyword, "pass") == 0) {
                        // "pass" alone - Python placeholder
                        if (*after == '\0' || *after == '\n' || *after == ';' || *after == '}') {
                            should_error = 1;
                        }
                    } else if (strcmp(INVALID_KEYWORDS[i].keyword, "switch") == 0) {
                        // "switch (" - switch statement
                        if (*after == '(' || *after == '{') {
                            should_error = 1;
                        }
                    } else if (strcmp(INVALID_KEYWORDS[i].keyword, "case") == 0) {
                        // "case x:" - case label
                        if (*after && (isalpha(*after) || *after == '_' || isdigit(*after) || *after == '"' || *after == '\'')) {
                            should_error = 1;
                        }
                    }
                    // Other keywords not restricted for now, allow as variable names
                    
                    if (should_error) {
                        return create_formatted_error(display_source, start_line, start_column,
                                                      INVALID_KEYWORDS[i].keyword,
                                                      INVALID_KEYWORDS[i].flyux_equiv,
                                                      INVALID_KEYWORDS[i].suggestion);
                    }
                }
            }
            continue;
        }
        
        // 跳过其他字符
        if (*p == '\n') { line++; column = 1; } else { column++; }
        p++;
    }
    
    return NULL;  // 没有错误
}

/**
 * 释放语句数组
 */
static void free_statements(Statement* statements, int count) {
    if (!statements) return;
    for (int i = 0; i < count; i++) {
        if (statements[i].content) {
            free(statements[i].content);
        }
    }
    free(statements);
}

/**
 * 检查这个 } 是否对应一个函数体
 * 方法：找到匹配的 {，看它前面是否是 )
 */
static int is_closing_function_body(const char* text, int closing_brace_idx) {
    // 从给定索引向后扫描，找到匹配的 {
    int brace_count = 1;
    
    // text[closing_brace_idx] 后面应该是 }，但在调用时可能还没写入
    // 我们需要从 closing_brace_idx - 1 开始向后扫描
    for (int i = closing_brace_idx - 1; i >= 0; i--) {
        if (text[i] == '}') {
            brace_count++;
        } else if (text[i] == '{') {
            brace_count--;
            if (brace_count == 0) {
                // 找到了匹配的 {，它在索引 i 处
                // 检查它前面是什么（跳过空白）
                int j = i - 1;
                while (j >= 0 && (text[j] == ' ' || text[j] == '\t' || text[j] == '\n')) {
                    j--;
                }
                
                // 如果前面是 )，说明这是函数体
                if (j >= 0 && text[j] == ')') {
                    return 1;
                }
                return 0;
            }
        }
    }
    return 0;
}

/**
 * 规范化块内部的换行符为分号（重构版）
 * 目标：
 *  1) 在代码块(如 if/else/L>/函数体)闭合时，若块内最后一条语句未以 ';' 结束，则在 '}' 之前补 ';'
 *  2) 保持原有：在块内（不在 () / []）的换行，若后续非 '}'，则将换行替换为 ';'
 *  3) 严格区分“代码块 { ... }”与“对象字面量 { ... }”，对象字面量结尾不自动补 ';'
 */
static char* normalize_internal_newlines(const char* stmt) {
    if (!stmt) return NULL;

    size_t len = strlen(stmt);
    // 结果缓冲：保守放大一倍空间
    char* result = (char*)malloc(len * 2 + 4);
    if (!result) return NULL;

    // brace 类型栈：1 表示代码块，0 表示对象字面量
    unsigned char* brace_is_block = (unsigned char*)calloc(len + 2, 1);
    if (!brace_is_block) {
        free(result);
        return NULL;
    }
    
    // 为每个大括号层追踪进入时的圆括号和方括号深度
    // 这样在代码块内部判断换行时，可以相对于当前层计算深度
    int* brace_entry_paren = (int*)calloc(len + 2, sizeof(int));
    int* brace_entry_bracket = (int*)calloc(len + 2, sizeof(int));
    if (!brace_entry_paren || !brace_entry_bracket) {
        free(result);
        free(brace_is_block);
        if (brace_entry_paren) free(brace_entry_paren);
        if (brace_entry_bracket) free(brace_entry_bracket);
        return NULL;
    }

    int out_idx = 0;
    int in_str = 0;
    int escape = 0;
    int brace_depth = 0;
    int paren_depth = 0;
    int bracket_depth = 0;

    for (int i = 0; i < (int)len; i++) {
        char ch = stmt[i];

        /* 转义处理 */
        if (escape) {
            result[out_idx++] = ch;
            escape = 0;
            continue;
        }
        if (ch == '\\') {
            result[out_idx++] = ch;
            escape = 1;
            continue;
        }

        /* 字符串处理（与原实现一致：简单切换，不区分单双引号配对） */
        if (ch == '"' || ch == '\'') {
            in_str = !in_str;
            result[out_idx++] = ch;
            continue;
        }
        if (in_str) {
            result[out_idx++] = ch;
            continue;
        }

        /* 左大括号：判定这是代码块还是对象字面量，并入栈 */
        if (ch == '{') {
            // 回看输出缓冲中最后一个非空白字符
            int last = out_idx - 1;
            while (last >= 0 && (result[last] == ' ' || result[last] == '\t' || result[last] == '\n')) {
                last--;
            }
            unsigned char is_block = 0;
            if (last >= 0) {
                char prev = result[last];
                // 经验规则：出现在 ) / } / ] 之后的 { 是代码块
                // 出现在 = / := / , / : / [ / ( 之后的 { 是对象字面量
                // 特殊处理：R> 后面的 { 是对象字面量（返回值）
                if (prev == ')' || prev == '}' || prev == ']') {
                    is_block = 1;
                } else if (prev == '=' || prev == ',' || prev == ':' || prev == '[' || prev == '(') {
                    is_block = 0;
                } else if (prev == '>') {
                    // 检查是否是 R> 或 .>
                    // R> 后面是对象字面量（返回值）
                    // .> 是方法链调用，后面不会直接跟 {
                    int prev2_idx = last - 1;
                    while (prev2_idx >= 0 && (result[prev2_idx] == ' ' || result[prev2_idx] == '\t')) {
                        prev2_idx--;
                    }
                    if (prev2_idx >= 0 && result[prev2_idx] == 'R') {
                        is_block = 0;  // R> { ... } 是返回对象字面量
                    } else {
                        is_block = 1;  // 其他情况默认为代码块
                    }
                } else {
                    // 其他情况（如操作符、标识符后），默认为代码块
                    is_block = 1;
                }
            } else {
                // 行首或文件开头的 { ，视为代码块（顶层代码块）
                is_block = 1;
            }

            // 入栈（下一个深度），同时记录当前的括号深度
            brace_is_block[brace_depth + 1] = is_block;
            brace_entry_paren[brace_depth + 1] = paren_depth;
            brace_entry_bracket[brace_depth + 1] = bracket_depth;
            brace_depth++;
            result[out_idx++] = ch;
            continue;
        }

        /* 右大括号：若当前层是代码块，必要时在 '}' 前补 ';' */
        if (ch == '}') {
            if (brace_depth > 0) {
                unsigned char is_block = brace_is_block[brace_depth];
                if (is_block) {
                    // 找到 '}' 之前最后一个非空白字符
                    int last = out_idx - 1;
                    while (last >= 0 && (result[last] == ' ' || result[last] == '\t' || result[last] == '\n')) {
                        last--;
                    }
                    // 若块非空，且最后不是 ';' 且也不是 '{'（排除空块 {}），则在最后实字符之后插入 ';'
                    if (last >= 0 && result[last] != ';' && result[last] != '{') {
                        // 将 [last+1, out_idx) 右移一位，准备插入 ';'
                        for (int j = out_idx; j > last + 1; j--) {
                            result[j] = result[j - 1];
                        }
                        result[last + 1] = ';';
                        out_idx++;
                    }
                }
                // 退出当前层
                brace_depth--;
            }
            result[out_idx++] = ch;
            continue;
        }

        /* 圆/方括号深度追踪（用于判定换行→分号） */
        if (ch == '(') {
            paren_depth++;
        } else if (ch == ')') {
            paren_depth--;
        } else if (ch == '[') {
            bracket_depth++;
        } else if (ch == ']') {
            bracket_depth--;
        }

        /* 块内换行 → 分号
         * 条件：
         * 1. 在代码块内（brace_depth > 0）
         * 2. 相对于当前代码块的圆括号/方括号深度为0
         *    （使用进入代码块时记录的深度作为基准）
         * 3. 换行后第一个非空不是 '}'
         * 4. 当前层是代码块而非对象字面量
         */
        if (ch == '\n' && brace_depth > 0) {
            // 计算相对于当前层的括号深度
            int relative_paren = paren_depth - brace_entry_paren[brace_depth];
            int relative_bracket = bracket_depth - brace_entry_bracket[brace_depth];
            
            // 检查当前是否在代码块中（而不是对象字面量中）
            unsigned char is_block = brace_is_block[brace_depth];
            
            if (getenv("DEBUG_NORM")) {
                fprintf(stderr, "[DEBUG] Newline at i=%d, brace_depth=%d, is_block=%d, rel_paren=%d, rel_bracket=%d\n", 
                        i, brace_depth, is_block, relative_paren, relative_bracket);
            }
            
            // 只在代码块中，且不在相对括号内时添加分号
            if (is_block && relative_paren == 0 && relative_bracket == 0) {
                int j = i + 1;
                while (j < (int)len && (stmt[j] == ' ' || stmt[j] == '\t' || stmt[j] == '\n')) {
                    j++;
                }
                if (j < (int)len && stmt[j] != '}') {
                    // 检查当前输出末尾（去除空格后）的最后字符
                    int last = out_idx - 1;
                    while (last >= 0 && (result[last] == ' ' || result[last] == '\t')) {
                        last--;
                    }
                    
                    // 获取下一行第一个非空白字符
                    char next_char = stmt[j];
                    
                    if (getenv("DEBUG_NORM")) {
                        fprintf(stderr, "[DEBUG] last=%d, last_char='%c', next_char='%c'\n", 
                                last, last >= 0 ? result[last] : '?', next_char);
                    }
                    
                    // 如果已经有分号、或者是开括号，则不添加
                    if (last >= 0 && (result[last] == ';' || result[last] == '{' || result[last] == '(')) {
                        // 跳过换行
                        if (getenv("DEBUG_NORM")) {
                            fprintf(stderr, "[DEBUG] Skipping semicolon (already have delimiter)\n");
                        }
                        continue;
                    }
                    
                    // 如果行末是运算符，说明表达式未完成，不添加分号
                    // 支持: + - * / % & | ^ < > = ! ~ , :
                    // 也包括复合运算符的第一个字符如 && || >= <= != ==
                    if (last >= 0) {
                        char lc = result[last];
                        if (lc == '+' || lc == '-' || lc == '*' || lc == '/' || lc == '%' ||
                            lc == '&' || lc == '|' || lc == '^' || lc == '<' || lc == '>' ||
                            lc == '=' || lc == '!' || lc == '~' || lc == ',' || lc == ':' ||
                            lc == '[' || lc == '?') {
                            // 行末是运算符，跳过分号
                            if (getenv("DEBUG_NORM")) {
                                fprintf(stderr, "[DEBUG] Skipping semicolon (line ends with operator '%c')\n", lc);
                            }
                            continue;
                        }
                    }
                    
                    // 如果下一行开头是运算符，说明表达式未完成，不添加分号
                    // 支持: + - * / % & | ^ . ?
                    // 注意：不包括 < > = ! 因为它们可能是比较表达式的开始
                    if (next_char == '+' || next_char == '-' || next_char == '*' || 
                        next_char == '/' || next_char == '%' || next_char == '&' ||
                        next_char == '|' || next_char == '^' || next_char == '.' ||
                        next_char == '?') {
                        // 下一行以运算符开头，跳过分号
                        if (getenv("DEBUG_NORM")) {
                            fprintf(stderr, "[DEBUG] Skipping semicolon (next line starts with operator '%c')\n", next_char);
                        }
                        continue;
                    }
                    
                    // 否则添加分号
                    if (getenv("DEBUG_NORM")) {
                        fprintf(stderr, "[DEBUG] Adding semicolon\n");
                    }
                    result[out_idx++] = ';';
                    // 跳过该换行
                    continue;
                } else {
                    // 块的结尾（后续是 '}'），跳过该换行，交由 '}' 分支统一补 ';'
                    if (getenv("DEBUG_NORM")) {
                        fprintf(stderr, "[DEBUG] Skipping newline before '}'\n");
                    }
                    continue;
                }
            } else if (is_block) {
                // 在代码块内但在相对括号内，跳过换行（不添加分号）
                continue;
            } else {
                // 对象字面量中的换行，转换为空格以便解析
                result[out_idx++] = ' ';
                continue;
            }
        }

        // 其他字符，原样写入
        result[out_idx++] = ch;
    }

    result[out_idx] = '\0';
    free(brace_is_block);
    free(brace_entry_paren);
    free(brace_entry_bracket);
    return result;
}

/**
 * 主规范化函数
 */
NormalizeResult flyux_normalize(const char* source_code) {
    NormalizeResult result = {NULL, NULL, 0, NULL, 0};
    
    if (!source_code) {
        result.error_msg = "Source code is null";
        result.error_code = -1;
        return result;
    }
    
    // Step 1: 移除注释
    char* no_comments = normalize_remove_comments(source_code);
    if (!no_comments) {
        result.error_msg = "Failed to remove comments";
        result.error_code = -1;
        return result;
    }
    
    // DEBUG
    if (getenv("DEBUG_NORM")) {
        fprintf(stderr, "=== AFTER REMOVE COMMENTS ===\n%s\n=== END ===\n", no_comments);
    }
    
    // Step 1.3: 验证变量声明（检查 := 左侧是否为有效标识符）
    char* validation_error = validate_variable_declarations(no_comments);
    if (validation_error) {
        result.error_msg = validation_error;
        result.error_code = -1;
        free(no_comments);
        return result;
    }
    
    // Step 1.4: 检测非法关键字（如 let, const, var, return 等）
    // 传入原始源代码用于显示，去注释后的代码用于检测
    char* keyword_error = check_invalid_keywords(no_comments, source_code);
    if (keyword_error) {
        result.error_msg = keyword_error;
        result.error_code = -1;
        free(no_comments);
        return result;
    }
    
    // Step 1.5: 规范化块内的换行符为分号
    char* normalized_newlines = normalize_internal_newlines(no_comments);
    free(no_comments);
    if (!normalized_newlines) {
        result.error_msg = "Failed to normalize newlines";
        result.error_code = -1;
        return result;
    }
    
    // DEBUG
    if (getenv("DEBUG_NORM")) {
        fprintf(stderr, "=== AFTER NORMALIZE NEWLINES ===\n%s\n=== END ===\n", normalized_newlines);
    }
    
    // Step 2: 分割语句
    int stmt_count = 0;
    Statement* statements = normalize_split_statements(normalized_newlines, &stmt_count);
    free(normalized_newlines);
    
    if (!statements || stmt_count == 0) {
        result.error_msg = "Failed to split statements";
        result.error_code = -1;
        return result;
    }
    
    // Step 3: 过滤表达式（如果有main函数）
    normalize_filter_expressions(statements, &stmt_count);
    
    // Step 4: 规范化每个语句并构建最终代码
    size_t total_len = 0;
    char** normalized_stmts = malloc(stmt_count * sizeof(char*));
    if (!normalized_stmts) {
        free_statements(statements, stmt_count);
        result.error_msg = "Memory allocation failed";
        result.error_code = -1;
        return result;
    }
    
    for (int i = 0; i < stmt_count; i++) {
        normalized_stmts[i] = normalize_statement_content(statements[i].content);
        if (!normalized_stmts[i]) {
            normalized_stmts[i] = malloc(1);
            normalized_stmts[i][0] = '\0';
        }
        total_len += strlen(normalized_stmts[i]) + 1;  // +1 for semicolon
    }
    
    // Step 5: 组合所有语句，添加分号
    char* normalized = malloc(total_len + 1);
    if (!normalized) {
        for (int i = 0; i < stmt_count; i++) {
            free(normalized_stmts[i]);
        }
        free(normalized_stmts);
        free_statements(statements, stmt_count);
        result.error_msg = "Memory allocation failed";
        result.error_code = -1;
        return result;
    }
    
    int out_idx = 0;
    for (int i = 0; i < stmt_count; i++) {
        int len = strlen(normalized_stmts[i]);
        if (len > 0) {
            strcpy(normalized + out_idx, normalized_stmts[i]);
            out_idx += len;
            normalized[out_idx++] = ';';
        }
        free(normalized_stmts[i]);
    }
    normalized[out_idx] = '\0';
    
    free(normalized_stmts);
    free_statements(statements, stmt_count);
    
    // 构建源码位置映射：记录每个规范化字符对应的原始位置
    size_t norm_len = strlen(normalized);
    SourceLocation* source_map = malloc(norm_len * sizeof(SourceLocation));
    if (!source_map) {
        free(normalized);
        result.error_msg = "Memory allocation failed for source_map";
        result.error_code = -1;
        return result;
    }
    
    // 构建字符级映射
    int orig_line = 1, orig_col = 1;
    size_t src_idx = 0, norm_idx = 0;
    size_t src_len = strlen(source_code);
    int src_in_string = 0;  // 跟踪原始源码是否在字符串内
    char src_string_quote = '\0';  // 字符串的引号类型
    
    // 主循环：扫描normalized的每个字符，找到其在原始源码中的位置
    while (norm_idx < norm_len) {
        // 内循环：跳过原始源码中的注释、换行、空白，找到下一个有效字符
        while (src_idx < src_len) {
            char src_ch = source_code[src_idx];
            
            // 处理字符串状态
            if (src_ch == '"' || src_ch == '\'') {
                if (!src_in_string) {
                    src_in_string = 1;
                    src_string_quote = src_ch;
                } else if (src_ch == src_string_quote) {
                    // 检查是否是转义的引号
                    int is_escaped = 0;
                    if (src_idx > 0 && source_code[src_idx - 1] == '\\') {
                        // 简单检测：前一个是反斜杠（可能需要更复杂的转义检测）
                        is_escaped = 1;
                    }
                    if (!is_escaped) {
                        src_in_string = 0;
                        src_string_quote = '\0';
                    }
                }
            }
            
            // 只在字符串外跳过注释
            if (!src_in_string) {
                // 跳过块注释 /* ... */
                if (src_ch == '/' && src_idx + 1 < src_len && source_code[src_idx + 1] == '*') {
                    src_idx += 2;
                    orig_col += 2;
                    while (src_idx + 1 < src_len) {
                        if (source_code[src_idx] == '\n') {
                            orig_line++;
                            orig_col = 1;
                            src_idx++;
                        } else {
                            // 按UTF-8字符计数列号
                            unsigned char byte = (unsigned char)source_code[src_idx];
                            int bytes_to_skip = 1;
                            if (byte >= 0xF0) bytes_to_skip = 4;
                            else if (byte >= 0xE0) bytes_to_skip = 3;
                            else if (byte >= 0xC0) bytes_to_skip = 2;
                            
                            src_idx += bytes_to_skip;
                            orig_col++;  // 一个UTF-8字符只加1列
                        }
                        
                        if (src_idx + 1 < src_len && source_code[src_idx] == '*' && source_code[src_idx + 1] == '/') {
                            src_idx += 2;
                            orig_col += 2;
                            break;
                        }
                    }
                    continue;
                }
                
                // 跳过行注释 //...
                if (src_ch == '/' && src_idx + 1 < src_len && source_code[src_idx + 1] == '/') {
                    src_idx += 2;
                    orig_col += 2;  // 跳过 //
                    while (src_idx < src_len && source_code[src_idx] != '\n') {
                        // 按UTF-8字符计数列号
                        unsigned char byte = (unsigned char)source_code[src_idx];
                        int bytes_to_skip = 1;
                        if (byte >= 0xF0) bytes_to_skip = 4;
                        else if (byte >= 0xE0) bytes_to_skip = 3;
                        else if (byte >= 0xC0) bytes_to_skip = 2;
                        
                        src_idx += bytes_to_skip;
                        orig_col++;  // 一个UTF-8字符只加1列
                    }
                    if (src_idx < src_len && source_code[src_idx] == '\n') {
                        src_idx++;
                        orig_line++;
                        orig_col = 1;
                    }
                    continue;
                }
                
                // 跳过换行
                if (src_ch == '\n') {
                    orig_line++;
                    orig_col = 1;
                    src_idx++;
                    continue;
                }
                
                // 跳过空白 - 但如果 normalized 中当前字符也是空格，则不跳过
                // 这样可以正确匹配保留在 normalized 中的空格
                if (src_ch == ' ' || src_ch == '\t' || src_ch == '\r') {
                    // 检查 normalized 中是否也是空格
                    if (normalized[norm_idx] != ' ') {
                        // normalized 中不是空格，跳过源码中的空格
                        orig_col++;
                        src_idx++;
                        continue;
                    }
                    // 否则，需要匹配这个空格，不跳过
                }
            } else {
                // 在字符串内，不跳过注释和空白，只跳过字符串外的换行（不应该有）
                // 字符串内的所有字符都需要匹配
            }
            
            // 找到有效字符，跳出内循环
            break;
        }
        
        // 现在src_idx指向下一个有效字符（或已到文件末尾）
        // 尝试匹配normalized[norm_idx]
        
        if (src_idx >= src_len) {
            // 原始源码已结束，normalized还有字符 → 必定是synthetic
            if (normalized[norm_idx] == ';') {
                source_map[norm_idx].orig_line = 0;
                source_map[norm_idx].orig_column = 0;
                source_map[norm_idx].orig_length = 0;
                source_map[norm_idx].is_synthetic = 1;
                norm_idx++;
                continue;
            }
            // 其他情况（不应该发生）
            norm_idx++;
            continue;
        }
        
        char src_ch = source_code[src_idx];
        
        // 尝试匹配
        if (src_ch == normalized[norm_idx]) {
            // 匹配成功！
            source_map[norm_idx].orig_line = orig_line;
            source_map[norm_idx].orig_column = orig_col;
            source_map[norm_idx].is_synthetic = 0;
            
            // 判断UTF-8字符长度
            int char_bytes = 1;
            if ((unsigned char)src_ch >= 0x80) {
                if ((unsigned char)src_ch >= 0xF0) char_bytes = 4;
                else if ((unsigned char)src_ch >= 0xE0) char_bytes = 3;
                else if ((unsigned char)src_ch >= 0xC0) char_bytes = 2;
                else char_bytes = 0;  // UTF-8后续字节
            }
            
            if (char_bytes > 0) {
                // 字符首字节
                source_map[norm_idx].orig_length = char_bytes;
                
                // 处理多字节字符的后续字节
                for (int b = 1; b < char_bytes && (norm_idx + b) < norm_len && (src_idx + b) < src_len; b++) {
                    if (source_code[src_idx + b] == normalized[norm_idx + b]) {
                        source_map[norm_idx + b].orig_line = orig_line;
                        source_map[norm_idx + b].orig_column = orig_col;
                        source_map[norm_idx + b].orig_length = 0;
                        source_map[norm_idx + b].is_synthetic = 0;
                    }
                }
                
                // 前进
                norm_idx += char_bytes;
                src_idx += char_bytes;
                orig_col++;
            } else {
                // UTF-8后续字节，已处理
                src_idx++;
            }
        } else {
            // 不匹配：可能是synthetic分号
            if (normalized[norm_idx] == ';') {
                source_map[norm_idx].orig_line = 0;
                source_map[norm_idx].orig_column = 0;
                source_map[norm_idx].orig_length = 0;
                source_map[norm_idx].is_synthetic = 1;
                norm_idx++;
                // 不前进src_idx，继续用当前原始位置
            } else {
                // 其他不匹配：跳过原始字符（不应该发生）
                src_idx++;
                orig_col++;
            }
        }
    }
    
    // 标记合成字符（normalize添加的分号等）
    while (norm_idx < norm_len) {
        source_map[norm_idx].orig_line = 0;  // 0表示合成
        source_map[norm_idx].orig_column = 0;
        source_map[norm_idx].orig_length = 0;
        source_map[norm_idx].is_synthetic = 1;
        norm_idx++;
    }
    
    result.normalized = normalized;
    result.source_map = source_map;
    result.source_map_size = norm_len;
    result.error_code = 0;
    return result;
}

/**
 * 释放规范化结果
 */
void normalize_result_free(NormalizeResult* result) {
    if (!result) return;
    if (result->normalized) {
        free(result->normalized);
        result->normalized = NULL;
    }
    if (result->source_map) {
        free(result->source_map);
        result->source_map = NULL;
    }
    if (result->error_msg) {
        free(result->error_msg);
        result->error_msg = NULL;
    }
}