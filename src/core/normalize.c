#include "flyuxc/normalize.h"
#include "flyuxc/mapper.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>

// Helper: check if character is part of identifier continuation (alphanumeric, underscore, or high-bit UTF-8)
static bool is_ident_char(unsigned char c) {
    return isalnum(c) || c == '_' || c >= 128;
}

// 简单状态机来去注释并保留字符串字面量
int normalize_file_to_stdout(const char *path) {
    if (!path) {
        fprintf(stderr, "No path provided\n");
        return 1;
    }

    FILE *f = fopen(path, "rb");
    if (!f) {
        perror(path);
        return 1;
    }

    // 读取整个文件到内存
    if (fseek(f, 0, SEEK_END) != 0) {
        perror("fseek");
        fclose(f);
        return 1;
    }
    long size = ftell(f);
    if (size < 0) size = 0;
    rewind(f);

    char *src = malloc((size_t)size + 1);
    if (!src) {
        perror("malloc");
        fclose(f);
        return 1;
    }
    size_t read = fread(src, 1, (size_t)size, f);
    src[read] = '\0';
    fclose(f);

    // 输出缓冲区（动态增长）
    size_t out_cap = read + 1;
    char *out = malloc(out_cap);
    if (!out) {
        perror("malloc");
        free(src);
        return 1;
    }
    size_t out_len = 0;

    enum { ST_NORMAL, ST_SLASH, ST_LINE_COMMENT, ST_BLOCK_COMMENT, ST_STRING, ST_CHAR, ST_STRING_ESC, ST_CHAR_ESC } state = ST_NORMAL;

    for (size_t i = 0; i < read; ++i) {
        char c = src[i];
        switch (state) {
            case ST_NORMAL:
                if (c == '/') {
                    state = ST_SLASH;
                } else if (c == '"') {
                    // 开始字符串，保留
                    if (out_len + 1 >= out_cap) {
                        out_cap = out_cap * 2 + 16;
                        out = realloc(out, out_cap);
                        if (!out) { perror("realloc"); free(src); return 1; }
                    }
                    out[out_len++] = c;
                    state = ST_STRING;
                } else if (c == '\'') {
                    // 开始字符字面量
                    if (out_len + 1 >= out_cap) {
                        out_cap = out_cap * 2 + 16;
                        out = realloc(out, out_cap);
                        if (!out) { perror("realloc"); free(src); return 1; }
                    }
                    out[out_len++] = c;
                    state = ST_CHAR;
                } else {
                    // 规范化：去掉函数名与 '(' 之间的空格
                    // 如果遇到空白字符，检查后续是否为 '('，如果是则跳过空白
                    if ((c == ' ' || c == '\t') && i + 1 < read) {
                        // lookahead to find next non-space
                        size_t j = i + 1;
                        while (j < read && (src[j] == ' ' || src[j] == '\t')) j++;
                        if (j < read && src[j] == '(') {
                            // skip current space (do not output)
                            continue;
                        }
                    }

                    if (out_len + 1 >= out_cap) {
                        out_cap = out_cap * 2 + 16;
                        out = realloc(out, out_cap);
                        if (!out) { perror("realloc"); free(src); return 1; }
                    }
                    out[out_len++] = c;
                }
                break;
            case ST_SLASH:
                if (c == '/') {
                    state = ST_LINE_COMMENT;
                } else if (c == '*') {
                    state = ST_BLOCK_COMMENT;
                } else {
                    // 之前的 '/' 不是注释，输出它，然后处理当前字符
                    if (out_len + 2 >= out_cap) {
                        out_cap = out_cap * 2 + 16;
                        out = realloc(out, out_cap);
                        if (!out) { perror("realloc"); free(src); return 1; }
                    }
                    out[out_len++] = '/';
                    // handle current char
                    if (c == '"') {
                        out[out_len++] = c;
                        state = ST_STRING;
                    } else if (c == '\'') {
                        out[out_len++] = c;
                        state = ST_CHAR;
                    } else {
                        out[out_len++] = c;
                        state = ST_NORMAL;
                    }
                }
                break;
            case ST_LINE_COMMENT:
                if (c == '\n') {
                    // end of line comment: output the newline to preserve line breaks
                    if (out_len + 1 >= out_cap) {
                        out_cap = out_cap * 2 + 16;
                        out = realloc(out, out_cap);
                        if (!out) { perror("realloc"); free(src); return 1; }
                    }
                    out[out_len++] = c;
                    state = ST_NORMAL;
                } else {
                    // skip comment chars
                }
                break;
            case ST_BLOCK_COMMENT:
                if (c == '*') {
                    // possible end
                    if (i + 1 < read && src[i+1] == '/') {
                        i++; // consume '/'
                        state = ST_NORMAL;
                    }
                } else {
                    // skip
                }
                break;
            case ST_STRING:
                if (c == '\\') {
                    // escape in string
                    if (out_len + 1 >= out_cap) {
                        out_cap = out_cap * 2 + 16;
                        out = realloc(out, out_cap);
                        if (!out) { perror("realloc"); free(src); return 1; }
                    }
                    out[out_len++] = c;
                    state = ST_STRING_ESC;
                } else if (c == '"') {
                    if (out_len + 1 >= out_cap) {
                        out_cap = out_cap * 2 + 16;
                        out = realloc(out, out_cap);
                        if (!out) { perror("realloc"); free(src); return 1; }
                    }
                    out[out_len++] = c;
                    state = ST_NORMAL;
                } else {
                    if (out_len + 1 >= out_cap) {
                        out_cap = out_cap * 2 + 16;
                        out = realloc(out, out_cap);
                        if (!out) { perror("realloc"); free(src); return 1; }
                    }
                    out[out_len++] = c;
                }
                break;
            case ST_STRING_ESC:
                // just copy escaped char and go back to string
                if (out_len + 1 >= out_cap) {
                    out_cap = out_cap * 2 + 16;
                    out = realloc(out, out_cap);
                    if (!out) { perror("realloc"); free(src); return 1; }
                }
                out[out_len++] = c;
                state = ST_STRING;
                break;
            case ST_CHAR:
                if (c == '\\') {
                    if (out_len + 1 >= out_cap) {
                        out_cap = out_cap * 2 + 16;
                        out = realloc(out, out_cap);
                        if (!out) { perror("realloc"); free(src); return 1; }
                    }
                    out[out_len++] = c;
                    state = ST_CHAR_ESC;
                } else if (c == '\'') {
                    if (out_len + 1 >= out_cap) {
                        out_cap = out_cap * 2 + 16;
                        out = realloc(out, out_cap);
                        if (!out) { perror("realloc"); free(src); return 1; }
                    }
                    out[out_len++] = c;
                    state = ST_NORMAL;
                } else {
                    if (out_len + 1 >= out_cap) {
                        out_cap = out_cap * 2 + 16;
                        out = realloc(out, out_cap);
                        if (!out) { perror("realloc"); free(src); return 1; }
                    }
                    out[out_len++] = c;
                }
                break;
            case ST_CHAR_ESC:
                if (out_len + 1 >= out_cap) {
                    out_cap = out_cap * 2 + 16;
                    out = realloc(out, out_cap);
                    if (!out) { perror("realloc"); free(src); return 1; }
                }
                out[out_len++] = c;
                state = ST_CHAR;
                break;
        }
    }

    // 输出缓冲区
    if (out_len > 0) {
        // 新的后处理：
        // 1) 修剪每行行尾空白
        // 2) 在逻辑行结束处（非以 '{', '(', '[' 开始，且不在未闭合的括号/花括号/方括号内）补充分号（如果缺失）
        // 3) 将所有内容合并为单行输出（用单个空格分隔原有行边界），避免产生连续分号

        // final buffer
        size_t fin_cap = out_len + 64;
        char *fin = malloc(fin_cap);
        if (!fin) { perror("malloc"); free(src); free(out); return 1; }
        size_t fin_len = 0;

        // 状态机，注意 out 已经保留了字符串和字符字面量
        enum { S_NORMAL, S_STRING, S_CHAR, S_ESC } sstate = S_NORMAL;
        int paren_depth = 0, brace_depth = 0, bracket_depth = 0;

        // We'll not use lambda in C; implement inline

        for (size_t i = 0; i < out_len; ++i) {
            char c = out[i];

            // handle state transitions for strings/chars to avoid injecting semicolons inside them
            if (sstate == S_STRING) {
                if (c == '\\') {
                    // copy escape and next char if any
                    if (fin_len + 1 >= fin_cap) { fin_cap = fin_cap * 2 + 16; fin = realloc(fin, fin_cap); if (!fin) { perror("realloc"); free(src); free(out); return 1; } }
                    fin[fin_len++] = c;
                    if (i + 1 < out_len) {
                        c = out[++i];
                        if (fin_len + 1 >= fin_cap) { fin_cap = fin_cap * 2 + 16; fin = realloc(fin, fin_cap); if (!fin) { perror("realloc"); free(src); free(out); return 1; } }
                        fin[fin_len++] = c;
                    }
                    continue;
                } else if (c == '"') {
                    if (fin_len + 1 >= fin_cap) { fin_cap = fin_cap * 2 + 16; fin = realloc(fin, fin_cap); if (!fin) { perror("realloc"); free(src); free(out); return 1; } }
                    fin[fin_len++] = c;
                    sstate = S_NORMAL;
                    continue;
                } else {
                    if (fin_len + 1 >= fin_cap) { fin_cap = fin_cap * 2 + 16; fin = realloc(fin, fin_cap); if (!fin) { perror("realloc"); free(src); free(out); return 1; } }
                    fin[fin_len++] = c;
                    continue;
                }
            }

            if (sstate == S_CHAR) {
                if (c == '\\') {
                    if (fin_len + 1 >= fin_cap) { fin_cap = fin_cap * 2 + 16; fin = realloc(fin, fin_cap); if (!fin) { perror("realloc"); free(src); free(out); return 1; } }
                    fin[fin_len++] = c;
                    if (i + 1 < out_len) {
                        c = out[++i];
                        if (fin_len + 1 >= fin_cap) { fin_cap = fin_cap * 2 + 16; fin = realloc(fin, fin_cap); if (!fin) { perror("realloc"); free(src); free(out); return 1; } }
                        fin[fin_len++] = c;
                    }
                    continue;
                } else if (c == '\'') {
                    if (fin_len + 1 >= fin_cap) { fin_cap = fin_cap * 2 + 16; fin = realloc(fin, fin_cap); if (!fin) { perror("realloc"); free(src); free(out); return 1; } }
                    fin[fin_len++] = c;
                    sstate = S_NORMAL;
                    continue;
                } else {
                    if (fin_len + 1 >= fin_cap) { fin_cap = fin_cap * 2 + 16; fin = realloc(fin, fin_cap); if (!fin) { perror("realloc"); free(src); free(out); return 1; } }
                    fin[fin_len++] = c;
                    continue;
                }
            }

            // NORMAL state
            if (c == '"') {
                if (fin_len + 1 >= fin_cap) { fin_cap = fin_cap * 2 + 16; fin = realloc(fin, fin_cap); if (!fin) { perror("realloc"); free(src); free(out); return 1; } }
                fin[fin_len++] = c;
                sstate = S_STRING;
                continue;
            }
            if (c == '\'') {
                if (fin_len + 1 >= fin_cap) { fin_cap = fin_cap * 2 + 16; fin = realloc(fin, fin_cap); if (!fin) { perror("realloc"); free(src); free(out); return 1; } }
                fin[fin_len++] = c;
                sstate = S_CHAR;
                continue;
            }

            // track nesting
            if (c == '(') { paren_depth++; }
            else if (c == ')') { if (paren_depth > 0) paren_depth--; }
            else if (c == '{') { brace_depth++; }
            else if (c == '}') { if (brace_depth > 0) brace_depth--; }
            else if (c == '[') { bracket_depth++; }
            else if (c == ']') { if (bracket_depth > 0) bracket_depth--; }

            if (c == '\n') {
                // on logical line break: decide whether to insert semicolon
                // find last non-space char in fin
                ssize_t j = (ssize_t)fin_len - 1;
                while (j >= 0 && (fin[j] == ' ' || fin[j] == '\t' || fin[j] == '\r')) j--;

                char last = (j >= 0) ? fin[j] : '\0';

                bool need_semi = false;
                if (last != '\0') {
                    if (last == ';' || last == '{' || last == '(' || last == '[' || last == ':' ) {
                        need_semi = false;
                    } else {
                        // if inside any unclosed paren/brace/bracket, avoid adding semicolon
                        if (paren_depth > 0 || brace_depth > 0 || bracket_depth > 0) {
                            need_semi = false;
                        } else {
                            need_semi = true;
                        }
                    }
                }

                if (need_semi) {
                    // avoid duplicate semicolon
                    if (!(last == ';')) {
                        if (fin_len + 1 >= fin_cap) { fin_cap = fin_cap * 2 + 16; fin = realloc(fin, fin_cap); if (!fin) { perror("realloc"); free(src); free(out); return 1; } }
                        fin[fin_len++] = ';';
                    }
                }

                // replace newline with a single space separator (but avoid duplicate spaces)
                if (fin_len > 0 && fin[fin_len-1] != ' ') {
                    if (fin_len + 1 >= fin_cap) { fin_cap = fin_cap * 2 + 16; fin = realloc(fin, fin_cap); if (!fin) { perror("realloc"); free(src); free(out); return 1; } }
                    fin[fin_len++] = ' ';
                }
                // skip emitting the newline itself
                continue;
            }

            // normal character: append
            if (fin_len + 1 >= fin_cap) { fin_cap = fin_cap * 2 + 16; fin = realloc(fin, fin_cap); if (!fin) { perror("realloc"); free(src); free(out); return 1; } }
            fin[fin_len++] = c;
        }

        // trim trailing spaces
        while (fin_len > 0 && (fin[fin_len-1] == ' ' || fin[fin_len-1] == '\t' || fin[fin_len-1] == '\r')) fin_len--;

        // 第二次规范化：去掉运算符（如 + - * / % = < > & | ^）和逗号周围的多余空格，保留字符串/字符字面量
        size_t norm_cap = fin_len + 16;
        char *norm = malloc(norm_cap);
        if (!norm) { perror("malloc"); free(src); free(out); free(fin); return 1; }
        size_t norm_len = 0;

        enum { N_NORMAL, N_STRING, N_CHAR } nstate = N_NORMAL;

        for (size_t i = 0; i < fin_len; ++i) {
            char c = fin[i];

            if (nstate == N_STRING) {
                // copy escapes and closing quote
                if (c == '\\') {
                    if (norm_len + 1 >= norm_cap) { norm_cap = norm_cap * 2 + 16; norm = realloc(norm, norm_cap); if (!norm) { perror("realloc"); free(src); free(out); free(fin); return 1; } }
                    norm[norm_len++] = c;
                    if (i + 1 < fin_len) {
                        c = fin[++i];
                        if (norm_len + 1 >= norm_cap) { norm_cap = norm_cap * 2 + 16; norm = realloc(norm, norm_cap); if (!norm) { perror("realloc"); free(src); free(out); free(fin); return 1; } }
                        norm[norm_len++] = c;
                    }
                    continue;
                } else if (c == '"') {
                    if (norm_len + 1 >= norm_cap) { norm_cap = norm_cap * 2 + 16; norm = realloc(norm, norm_cap); if (!norm) { perror("realloc"); free(src); free(out); free(fin); return 1; } }
                    norm[norm_len++] = c;
                    nstate = N_NORMAL;
                    continue;
                } else {
                    if (norm_len + 1 >= norm_cap) { norm_cap = norm_cap * 2 + 16; norm = realloc(norm, norm_cap); if (!norm) { perror("realloc"); free(src); free(out); free(fin); return 1; } }
                    norm[norm_len++] = c;
                    continue;
                }
            }

            if (nstate == N_CHAR) {
                if (c == '\\') {
                    if (norm_len + 1 >= norm_cap) { norm_cap = norm_cap * 2 + 16; norm = realloc(norm, norm_cap); if (!norm) { perror("realloc"); free(src); free(out); free(fin); return 1; } }
                    norm[norm_len++] = c;
                    if (i + 1 < fin_len) {
                        c = fin[++i];
                        if (norm_len + 1 >= norm_cap) { norm_cap = norm_cap * 2 + 16; norm = realloc(norm, norm_cap); if (!norm) { perror("realloc"); free(src); free(out); free(fin); return 1; } }
                        norm[norm_len++] = c;
                    }
                    continue;
                } else if (c == '\'') {
                    if (norm_len + 1 >= norm_cap) { norm_cap = norm_cap * 2 + 16; norm = realloc(norm, norm_cap); if (!norm) { perror("realloc"); free(src); free(out); free(fin); return 1; } }
                    norm[norm_len++] = c;
                    nstate = N_NORMAL;
                    continue;
                } else {
                    if (norm_len + 1 >= norm_cap) { norm_cap = norm_cap * 2 + 16; norm = realloc(norm, norm_cap); if (!norm) { perror("realloc"); free(src); free(out); free(fin); return 1; } }
                    norm[norm_len++] = c;
                    continue;
                }
            }

            // NORMAL state
            if (c == '"') {
                if (norm_len + 1 >= norm_cap) { norm_cap = norm_cap * 2 + 16; norm = realloc(norm, norm_cap); if (!norm) { perror("realloc"); free(src); free(out); free(fin); return 1; } }
                norm[norm_len++] = c;
                nstate = N_STRING;
                continue;
            }
            if (c == '\'') {
                if (norm_len + 1 >= norm_cap) { norm_cap = norm_cap * 2 + 16; norm = realloc(norm, norm_cap); if (!norm) { perror("realloc"); free(src); free(out); free(fin); return 1; } }
                norm[norm_len++] = c;
                nstate = N_CHAR;
                continue;
            }

            // Remove explicit newlines if any (ensure single-line)
            if (c == '\n' || c == '\r') {
                continue;
            }

            // Bracket/parenthesis handling: remove spaces after '(' and before ')', '{', '}', '[' ,']'
            if (c == '(') {
                // remove trailing space before '('
                while (norm_len > 0 && norm[norm_len-1] == ' ') norm_len--;
                if (norm_len + 1 >= norm_cap) { norm_cap = norm_cap * 2 + 16; norm = realloc(norm, norm_cap); if (!norm) { perror("realloc"); free(src); free(out); free(fin); return 1; } }
                norm[norm_len++] = '(';
                // skip spaces after '('
                size_t j = i + 1; while (j < fin_len && fin[j] == ' ') j++; i = j - 1;
                continue;
            }
            if (c == ')' || c == '}' || c == ']') {
                // remove trailing spaces before closing delimiter
                while (norm_len > 0 && norm[norm_len-1] == ' ') norm_len--;
                if (norm_len + 1 >= norm_cap) { norm_cap = norm_cap * 2 + 16; norm = realloc(norm, norm_cap); if (!norm) { perror("realloc"); free(src); free(out); free(fin); return 1; } }
                norm[norm_len++] = c;
                continue;
            }

            // Comma handling: remove spaces before, ensure single space after (unless next is closing delimiter or semicolon)
            if (c == ',') {
                // remove trailing spaces in norm
                while (norm_len > 0 && norm[norm_len-1] == ' ') norm_len--;
                if (norm_len + 1 >= norm_cap) { norm_cap = norm_cap * 2 + 16; norm = realloc(norm, norm_cap); if (!norm) { perror("realloc"); free(src); free(out); free(fin); return 1; } }
                norm[norm_len++] = ',';
                // skip spaces after comma in fin
                size_t j = i + 1;
                while (j < fin_len && fin[j] == ' ') j++;
                // if next non-space is not a closing delimiter or semicolon or end, add single space
                if (j < fin_len && fin[j] != ')' && fin[j] != ']' && fin[j] != '}' && fin[j] != ';' && fin[j] != ',') {
                    if (norm_len + 1 >= norm_cap) { norm_cap = norm_cap * 2 + 16; norm = realloc(norm, norm_cap); if (!norm) { perror("realloc"); free(src); free(out); free(fin); return 1; } }
                    norm[norm_len++] = ' ';
                }
                i = j - 1;
                continue;
            }

            // Operators: remove spaces around common operators
            if (c == '+' || c == '-' || c == '*' || c == '/' || c == '%' || c == '=' || c == '<' || c == '>' || c == '&' || c == '|' || c == '^' || c == '!') {
                // remove trailing spaces before operator
                while (norm_len > 0 && norm[norm_len-1] == ' ') norm_len--;
                // append operator
                if (norm_len + 1 >= norm_cap) { norm_cap = norm_cap * 2 + 16; norm = realloc(norm, norm_cap); if (!norm) { perror("realloc"); free(src); free(out); free(fin); return 1; } }
                norm[norm_len++] = c;
                // skip spaces after operator
                size_t j = i + 1;
                while (j < fin_len && fin[j] == ' ') j++;
                // append following token immediately (no space)
                if (j < fin_len) {
                    // do nothing here; next loop iteration will append fin[j]
                }
                i = j - 1;
                continue;
            }

            // default: copy character, collapse multiple spaces to single
            if (c == ' ') {
                if (norm_len == 0) continue; // skip leading space
                if (norm[norm_len-1] == ' ') continue; // collapse
                // else append single space
                if (norm_len + 1 >= norm_cap) { norm_cap = norm_cap * 2 + 16; norm = realloc(norm, norm_cap); if (!norm) { perror("realloc"); free(src); free(out); free(fin); return 1; } }
                norm[norm_len++] = ' ';
                continue;
            }

            if (norm_len + 1 >= norm_cap) { norm_cap = norm_cap * 2 + 16; norm = realloc(norm, norm_cap); if (!norm) { perror("realloc"); free(src); free(out); free(fin); return 1; } }
            norm[norm_len++] = c;
        }

        // trim trailing spaces again
        while (norm_len > 0 && (norm[norm_len-1] == ' ' || norm[norm_len-1] == '\t' || norm[norm_len-1] == '\r')) norm_len--;

        // 最后一次更强力的规范化：
        // - 移除标点前多余空格
        // - 移除 '(' 后的空格
        // - 逗号后不保留空格
        // - 如果在闭合分隔符 ']','}','')' 之后紧跟标识符/字面量，则补充分号
        size_t final_cap = norm_len + 16;
        char *final = malloc(final_cap);
        if (!final) { perror("malloc"); free(src); free(out); free(fin); free(norm); return 1; }
        size_t final_len = 0;

        enum { F_NORMAL, F_STRING, F_CHAR } fstate = F_NORMAL;

        for (size_t i = 0; i < norm_len; ++i) {
            char c = norm[i];

            if (fstate == F_STRING) {
                if (c == '\\') {
                    if (final_len + 1 >= final_cap) { final_cap = final_cap * 2 + 16; final = realloc(final, final_cap); if (!final) { perror("realloc"); free(src); free(out); free(fin); free(norm); return 1; } }
                    final[final_len++] = c;
                    if (i + 1 < norm_len) {
                        c = norm[++i];
                        if (final_len + 1 >= final_cap) { final_cap = final_cap * 2 + 16; final = realloc(final, final_cap); if (!final) { perror("realloc"); free(src); free(out); free(fin); free(norm); return 1; } }
                        final[final_len++] = c;
                    }
                    continue;
                } else if (c == '"') {
                    if (final_len + 1 >= final_cap) { final_cap = final_cap * 2 + 16; final = realloc(final, final_cap); if (!final) { perror("realloc"); free(src); free(out); free(fin); free(norm); return 1; } }
                    final[final_len++] = c;
                    fstate = F_NORMAL;
                    continue;
                } else {
                    if (final_len + 1 >= final_cap) { final_cap = final_cap * 2 + 16; final = realloc(final, final_cap); if (!final) { perror("realloc"); free(src); free(out); free(fin); free(norm); return 1; } }
                    final[final_len++] = c;
                    continue;
                }
            }

            if (fstate == F_CHAR) {
                if (c == '\\') {
                    if (final_len + 1 >= final_cap) { final_cap = final_cap * 2 + 16; final = realloc(final, final_cap); if (!final) { perror("realloc"); free(src); free(out); free(fin); free(norm); return 1; } }
                    final[final_len++] = c;
                    if (i + 1 < norm_len) {
                        c = norm[++i];
                        if (final_len + 1 >= final_cap) { final_cap = final_cap * 2 + 16; final = realloc(final, final_cap); if (!final) { perror("realloc"); free(src); free(out); free(fin); free(norm); return 1; } }
                        final[final_len++] = c;
                    }
                    continue;
                } else if (c == '\'') {
                    if (final_len + 1 >= final_cap) { final_cap = final_cap * 2 + 16; final = realloc(final, final_cap); if (!final) { perror("realloc"); free(src); free(out); free(fin); free(norm); return 1; } }
                    final[final_len++] = c;
                    fstate = F_NORMAL;
                    continue;
                } else {
                    if (final_len + 1 >= final_cap) { final_cap = final_cap * 2 + 16; final = realloc(final, final_cap); if (!final) { perror("realloc"); free(src); free(out); free(fin); free(norm); return 1; } }
                    final[final_len++] = c;
                    continue;
                }
            }

            if (c == '"') {
                if (final_len + 1 >= final_cap) { final_cap = final_cap * 2 + 16; final = realloc(final, final_cap); if (!final) { perror("realloc"); free(src); free(out); free(fin); free(norm); return 1; } }
                final[final_len++] = c;
                fstate = F_STRING;
                continue;
            }
            if (c == '\'') {
                if (final_len + 1 >= final_cap) { final_cap = final_cap * 2 + 16; final = realloc(final, final_cap); if (!final) { perror("realloc"); free(src); free(out); free(fin); free(norm); return 1; } }
                final[final_len++] = c;
                fstate = F_CHAR;
                continue;
            }

            // Comma in final pass: remove spaces before and skip spaces after (keep compact form)
            if (c == ',') {
                while (final_len > 0 && final[final_len-1] == ' ') final_len--;
                if (final_len + 1 >= final_cap) { final_cap = final_cap * 2 + 16; final = realloc(final, final_cap); if (!final) { perror("realloc"); free(src); free(out); free(fin); free(norm); return 1; } }
                final[final_len++] = ',';
                size_t j = i + 1; while (j < norm_len && norm[j] == ' ') j++; i = j - 1;
                continue;
            }

            // if current is space, examine prev non-space and next non-space to decide
            // but first: if previous char is '(' then drop following spaces
            if (final_len > 0 && final[final_len-1] == '(' && c == ' ') {
                size_t j = i + 1; while (j < norm_len && norm[j] == ' ') j++; i = j - 1; continue;
            }

            if (c == ' ') {
                size_t j = i + 1; while (j < norm_len && norm[j] == ' ') j++;
                char next = (j < norm_len) ? norm[j] : '\0';
                ssize_t p = (ssize_t)final_len - 1; while (p >= 0 && final[p] == ' ') p--;
                char prev = (p >= 0) ? final[p] : '\0';

                // if previous is closing delimiter and next looks like identifier/literal, insert semicolon
                if ((prev == ']' || prev == '}' || prev == ')') && next != '\0' && next != ')' && next != ']' && next != '}' && next != ',' && next != ';' && next != ':' && next != '.' ) {
                    if (final_len + 1 >= final_cap) { final_cap = final_cap * 2 + 16; final = realloc(final, final_cap); if (!final) { perror("realloc"); free(src); free(out); free(fin); free(norm); return 1; } }
                    final[final_len++] = ';';
                    i = j - 1; // skip spaces
                    continue;
                }

                // remove space before punctuation (if next is punctuation), else collapse to single space
                if (next == ')' || next == '}' || next == ']' || next == ',' || next == ';' || next == ':') {
                    i = j - 1; // skip spaces
                    continue;
                }
                // else keep single space if not duplicate
                if (final_len == 0) { i = j - 1; continue; }
                if (final[final_len-1] == ' ') { i = j - 1; continue; }
                if (final_len + 1 >= final_cap) { final_cap = final_cap * 2 + 16; final = realloc(final, final_cap); if (!final) { perror("realloc"); free(src); free(out); free(fin); free(norm); return 1; } }
                final[final_len++] = ' ';
                i = j - 1;
                continue;
            }

            // remove space after '(' if any (already handled earlier but ensure no leftover)
            if (final_len > 0 && final[final_len-1] == '(') {
                // skip spaces after '('
                if (c == ' ') continue;
            }

            // punctuation handling: remove trailing spaces before punctuation and skip spaces after
            if (c == ':' || c == ')' || c == ']' || c == ',' || c == ';') {
                while (final_len > 0 && final[final_len-1] == ' ') final_len--;
                if (final_len + 1 >= final_cap) { final_cap = final_cap * 2 + 16; final = realloc(final, final_cap); if (!final) { perror("realloc"); free(src); free(out); free(fin); free(norm); return 1; } }
                final[final_len++] = c;
                // skip following spaces
                size_t j = i + 1; while (j < norm_len && norm[j] == ' ') j++; i = j - 1;
                continue;
            }

            // closing brace: insert semicolon before it if needed (last non-space char is not ; or {)
            if (c == '}') {
                // Check if last non-space is something that needs a semicolon after
                ssize_t last_idx = (ssize_t)final_len - 1;
                while (last_idx >= 0 && final[last_idx] == ' ') last_idx--;
                char last_ch = (last_idx >= 0) ? final[last_idx] : '\0';
                
                // If previous char is not ; or { or [ or ( or : or comma, we should insert semicolon
                if (last_ch != '\0' && last_ch != ';' && last_ch != '{' && last_ch != '[' && last_ch != '(' && last_ch != ':' && last_ch != ',') {
                    if (final_len + 1 >= final_cap) { final_cap = final_cap * 2 + 16; final = realloc(final, final_cap); if (!final) { perror("realloc"); free(src); free(out); free(fin); free(norm); return 1; } }
                    final[final_len++] = ';';
                }
                
                while (final_len > 0 && final[final_len-1] == ' ') final_len--;
                if (final_len + 1 >= final_cap) { final_cap = final_cap * 2 + 16; final = realloc(final, final_cap); if (!final) { perror("realloc"); free(src); free(out); free(fin); free(norm); return 1; } }
                final[final_len++] = '}';
                size_t j = i + 1; while (j < norm_len && norm[j] == ' ') j++; i = j - 1;
                continue;
            }

            // opening delimiters: remove trailing spaces before and skip spaces after
            if (c == '{' || c == '(' || c == '[') {
                while (final_len > 0 && final[final_len-1] == ' ') final_len--;
                if (final_len + 1 >= final_cap) { final_cap = final_cap * 2 + 16; final = realloc(final, final_cap); if (!final) { perror("realloc"); free(src); free(out); free(fin); free(norm); return 1; } }
                final[final_len++] = c;
                size_t j = i + 1; while (j < norm_len && norm[j] == ' ') j++; i = j - 1;
                continue;
            }

            // default copy
            if (final_len + 1 >= final_cap) { final_cap = final_cap * 2 + 16; final = realloc(final, final_cap); if (!final) { perror("realloc"); free(src); free(out); free(fin); free(norm); return 1; } }
            final[final_len++] = c;
        }

        // final trim and output
        while (final_len > 0 && (final[final_len-1] == ' ' || final[final_len-1] == '\t' || final[final_len-1] == '\r')) final_len--;

        // Additional pass: 在闭合分隔符之后直接接标识符/字面量的情况（例如 '}' 之后紧跟 emoji/identifier），
        // 插入缺失的分号以明确语句边界，方便后续 lexer/parser。
        size_t final2_cap = final_len + 16;
        char *final2 = malloc(final2_cap);
        if (!final2) { perror("malloc"); free(src); free(out); free(fin); free(norm); free(final); return 1; }
        size_t final2_len = 0;

        for (size_t i = 0; i < final_len; ++i) {
            unsigned char ch = (unsigned char)final[i];
            // append current char
            if (final2_len + 1 >= final2_cap) { final2_cap = final2_cap * 2 + 16; final2 = realloc(final2, final2_cap); if (!final2) { perror("realloc"); free(src); free(out); free(fin); free(norm); free(final); return 1; } }
            final2[final2_len++] = ch;

            // if current is a closing delimiter, and next exists and is not punctuation/operator/space, insert semicolon
            if ((ch == ')' || ch == ']' || ch == '}') && i + 1 < final_len) {
                unsigned char next = (unsigned char)final[i+1];
                // characters considered as NOT starting a new statement: space, punctuation, closing/opening delimiters, operators, semicolon, colon, dot, comma
                bool next_is_split = (next == ' ' || next == '\t' || next == '\r' || next == '\n'
                                      || next == ')' || next == ']' || next == '}' || next == '(' || next == '[' || next == '{'
                                      || next == ',' || next == ';' || next == ':' || next == '.'
                                      || next == '+' || next == '-' || next == '*' || next == '/' || next == '%' || next == '=' || next == '<' || next == '>' || next == '&' || next == '|' || next == '^' || next == '!');
                if (!next_is_split) {
                    // insert semicolon if not already present
                    if (final2_len == 0 || final2[final2_len-1] != ';') {
                        if (final2_len + 1 >= final2_cap) { final2_cap = final2_cap * 2 + 16; final2 = realloc(final2, final2_cap); if (!final2) { perror("realloc"); free(src); free(out); free(fin); free(norm); free(final); return 1; } }
                        final2[final2_len++] = ';';
                    }
                }
            }
        }

        // trim and output final2
        while (final2_len > 0 && (final2[final2_len-1] == ' ' || final2[final2_len-1] == '\t' || final2[final2_len-1] == '\r')) final2_len--;

        // PART 1: Output normalized code (with Unicode identifiers)
        printf("=== NORMALIZED CODE ===\n");
        if (final2_len == 0) putchar('\n');
        else { fwrite(final2, 1, final2_len, stdout); putchar('\n'); }

        // PART 2: Extract identifiers, create mapper, output identifier mapping JSON
        printf("\n=== IDENTIFIER MAPPING ===\n");
        IdentifierMapper *mapper = mapper_create();
        if (!mapper) { perror("mapper_create"); free(final2); free(final); free(norm); free(fin); free(src); free(out); return 1; }

        // Extract all identifiers from final2 (skipping strings and chars) and build mapping
        char current_ident[256];
        size_t ident_len = 0;
        enum { EXT_NORMAL, EXT_STRING, EXT_CHAR, EXT_ESC } ext_state = EXT_NORMAL;
        
        for (size_t i = 0; i <= final2_len; ++i) {
            unsigned char c = (i < final2_len) ? (unsigned char)final2[i] : 0;
            
            // State machine to skip strings/chars
            if (ext_state == EXT_STRING) {
                if (c == '\\') { ext_state = EXT_ESC; continue; }
                else if (c == '"') { ext_state = EXT_NORMAL; continue; }
                else { continue; }
            }
            if (ext_state == EXT_CHAR) {
                if (c == '\\') { ext_state = EXT_ESC; continue; }
                else if (c == '\'') { ext_state = EXT_NORMAL; continue; }
                else { continue; }
            }
            if (ext_state == EXT_ESC) {
                ext_state = (i > 0 && final2[i-1] == '"') ? EXT_STRING : (i > 0 && final2[i-1] == '\'') ? EXT_CHAR : EXT_NORMAL;
                continue;
            }

            // In normal state, check for strings/chars
            if (c == '"') { ext_state = EXT_STRING; continue; }
            if (c == '\'') { ext_state = EXT_CHAR; continue; }

            // Extract identifiers
            if (is_ident_char(c)) {
                if (ident_len < sizeof(current_ident) - 1) {
                    current_ident[ident_len++] = c;
                }
            } else {
                if (ident_len > 0) {
                    current_ident[ident_len] = '\0';
                    mapper_get_or_alloc(mapper, current_ident);
                    ident_len = 0;
                }
            }
        }
        mapper_output_json(mapper);

        // PART 3: Apply mapping to generate intermediate representation (all Unicode -> ASCII)
        printf("\n=== INTERMEDIATE REPRESENTATION ===\n");
        // Use a larger buffer for IR
        size_t ir_cap = final2_len * 2 + 256;
        char *ir = malloc(ir_cap);
        if (!ir) { perror("malloc"); mapper_free(mapper); free(final2); free(final); free(norm); free(fin); free(src); free(out); return 1; }
        size_t ir_len = 0;
        
        ident_len = 0;
        ext_state = EXT_NORMAL;
        
        for (size_t i = 0; i <= final2_len; ++i) {
            unsigned char c = (i < final2_len) ? (unsigned char)final2[i] : 0;
            
            // Handle strings/chars: copy as-is
            if (ext_state == EXT_STRING) {
                if (ir_len < ir_cap) ir[ir_len++] = c;
                if (c == '\\') { ext_state = EXT_ESC; continue; }
                else if (c == '"') { ext_state = EXT_NORMAL; }
                continue;
            }
            if (ext_state == EXT_CHAR) {
                if (ir_len < ir_cap) ir[ir_len++] = c;
                if (c == '\\') { ext_state = EXT_ESC; continue; }
                else if (c == '\'') { ext_state = EXT_NORMAL; }
                continue;
            }
            if (ext_state == EXT_ESC) {
                if (ir_len < ir_cap) ir[ir_len++] = c;
                ext_state = (i > 0 && final2[i-1] == '"') ? EXT_STRING : (i > 0 && final2[i-1] == '\'') ? EXT_CHAR : EXT_NORMAL;
                continue;
            }

            // In normal state
            if (c == '"') { if (ir_len < ir_cap) ir[ir_len++] = c; ext_state = EXT_STRING; continue; }
            if (c == '\'') { if (ir_len < ir_cap) ir[ir_len++] = c; ext_state = EXT_CHAR; continue; }

            // Extract and map identifiers
            if (is_ident_char(c)) {
                if (ident_len < sizeof(current_ident) - 1) {
                    current_ident[ident_len++] = c;
                }
            } else {
                if (ident_len > 0) {
                    current_ident[ident_len] = '\0';
                    const char *mapped = mapper_get_or_alloc(mapper, current_ident);
                    size_t mapped_len = strlen(mapped);
                    if (ir_len + mapped_len < ir_cap) {
                        memcpy(ir + ir_len, mapped, mapped_len);
                        ir_len += mapped_len;
                    }
                    ident_len = 0;
                }
                // Append non-identifier character
                if (c != 0 && ir_len < ir_cap - 1) {
                    ir[ir_len++] = c;
                }
            }
        }
        if (ir_len > 0) fwrite(ir, 1, ir_len, stdout);
        putchar('\n');

        free(ir);
        mapper_free(mapper);
        free(final2);
        free(final);
        free(norm);
        free(fin);
    }

    free(src);
    free(out);
    return 0;
}
