#include "flyuxc/varmap.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>

/* ========== 工具函数 ========== */

static int is_space_c(int c) {
    return c==' ' || c=='\t' || c=='\n' || c=='\r' || c=='\v' || c=='\f';
}

static int is_ident_char(int c) {
    /* 允许 ASCII 字母、数字、_，以及所有非 ASCII 字节（emoji 等） */
    return (c == '_' || isalnum((unsigned char)c) || (unsigned char)c >= 0x80);
}

static int is_ident_start(int c) {
    /* 允许以字母、_、非 ASCII 开头，数字开头暂不当作标识符起点 */
    return (c == '_' || isalpha((unsigned char)c) || (unsigned char)c >= 0x80);
}

/* 简单字符串复制，出错返回 NULL */
static char* str_dup_n(const char* s, size_t len) {
    char* out = (char*)malloc(len + 1);
    if (!out) return NULL;
    memcpy(out, s, len);
    out[len] = '\0';
    return out;
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

    /* L / R 用于 L> / R>，简单起见直接视为保留 */
    if (len == 1 && (name[0] == 'L' || name[0] == 'R')) return 1;

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
static int is_builtin_identifier(const char* name, size_t len) {
    /* 内置函数 print */
    if (len == 5 &&
        name[0]=='p' && name[1]=='r' && name[2]=='i' && name[3]=='n' && name[4]=='t') {
        return 1;
    }

    /* 如需扩展更多内置，在这里追加即可 */

    return 0;
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

/* ========== 主映射逻辑 ========== */

VarMapResult flyux_varmap_process(const char* normalized_source) {
    VarMapResult result;
    result.mapped_source = NULL;
    result.entries = NULL;
    result.count = 0;
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

    VarMapEntry* entries = NULL;
    size_t entry_count = 0;
    size_t entry_cap = 0;
    size_t next_index = 1;  /* 用于生成 _00001, _00002, ... */

    int in_str = 0;
    int escape = 0;
    size_t i = 0;
    size_t out_idx = 0;

    while (i < len) {
        char c = normalized_source[i];

        /* 字符串内照抄 */
        if (escape) {
            if (out_idx + 1 >= out_cap) {
                out_cap *= 2;
                char* new_out = (char*)realloc(out, out_cap);
                if (!new_out) {
                    result.error_code = -1;
                    result.error_msg = str_dup_n("Memory allocation failed", strlen("Memory allocation failed"));
                    free(out);
                    varmap_result_free(&result);
                    return result;
                }
                out = new_out;
            }
            out[out_idx++] = c;
            escape = 0;
            i++;
            continue;
        }

        if (c == '\\') {
            if (out_idx + 1 >= out_cap) {
                out_cap *= 2;
                char* new_out = (char*)realloc(out, out_cap);
                if (!new_out) {
                    result.error_code = -1;
                    result.error_msg = str_dup_n("Memory allocation failed", strlen("Memory allocation failed"));
                    free(out);
                    varmap_result_free(&result);
                    return result;
                }
                out = new_out;
            }
            out[out_idx++] = c;
            escape = 1;
            i++;
            continue;
        }

        if (c == '"' || c == '\'') {
            /* 进入或退出字符串 */
            if (out_idx + 1 >= out_cap) {
                out_cap *= 2;
                char* new_out = (char*)realloc(out, out_cap);
                if (!new_out) {
                    result.error_code = -1;
                    result.error_msg = str_dup_n("Memory allocation failed", strlen("Memory allocation failed"));
                    free(out);
                    varmap_result_free(&result);
                    return result;
                }
                out = new_out;
            }
            out[out_idx++] = c;
            in_str = !in_str;
            i++;
            continue;
        }

        if (in_str) {
            /* 字符串内容照抄 */
            if (out_idx + 1 >= out_cap) {
                out_cap *= 2;
                char* new_out = (char*)realloc(out, out_cap);
                if (!new_out) {
                    result.error_code = -1;
                    result.error_msg = str_dup_n("Memory allocation failed", strlen("Memory allocation failed"));
                    free(out);
                    varmap_result_free(&result);
                    return result;
                }
                out = new_out;
            }
            out[out_idx++] = c;
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

            /* 确认 token 边界：前一个/后一个字符不能是标识符字符 */
            if ( (start == 0 || !is_ident_char((unsigned char)normalized_source[start - 1])) &&
                 (j >= len || !is_ident_char((unsigned char)normalized_source[j])) ) {

                /* 保留/内置标识符：直接不参与映射 */
                int reserved = is_reserved_identifier(ident_start, ident_len) ||
                               is_builtin_identifier(ident_start, ident_len);

                /* 上下文判断：成员访问 / 方法名 / 对象 key */
                char before = (start > 0) ? normalized_source[start - 1] : '\0';
                char after  = (j < len) ? normalized_source[j] : '\0';

                int is_method_after_chain = 0;   /* .>methodName */
                int is_property_access    = 0;   /* obj.property */
                if (before == '>' && start >= 2 && normalized_source[start - 2] == '.') {
                    /* ".>method" 场景 */
                    is_method_after_chain = 1;
                } else if (before == '.') {
                    /* 普通属性访问 obj.prop */
                    is_property_access = 1;
                }

                int is_object_key = 0;
                if (after == ':') {
                    /* 区分类型注解/变量定义 和 对象 key */
                    if (!looks_like_typed_definition(normalized_source, len, j)) {
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

                /* 写出 token（要么映射名，要么原名） */
                if (out_idx + replacement_len + 1 >= out_cap) {
                    while (out_idx + replacement_len + 1 >= out_cap) {
                        out_cap *= 2;
                    }
                    char* new_out = (char*)realloc(out, out_cap);
                    if (!new_out) {
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
                    out = new_out;
                }

                memcpy(out + out_idx, replacement, replacement_len);
                out_idx += replacement_len;

                i = j;
                continue;
            }

            /* 不满足 token 边界，按普通字符抄 */
            if (out_idx + 1 >= out_cap) {
                out_cap *= 2;
                char* new_out = (char*)realloc(out, out_cap);
                if (!new_out) {
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
                out = new_out;
            }
            out[out_idx++] = c;
            i++;
            continue;
        }

        /* 其他字符照抄 */
        if (out_idx + 1 >= out_cap) {
            out_cap *= 2;
            char* new_out = (char*)realloc(out, out_cap);
            if (!new_out) {
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
            out = new_out;
        }
        out[out_idx++] = c;
        i++;
    }

    out[out_idx] = '\0';

    result.mapped_source = out;
    result.entries = entries;
    result.count = entry_count;
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
    if (result->entries) {
        for (size_t i = 0; i < result->count; i++) {
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
    result->count = 0;
    result->error_code = 0;
}

void varmap_print_table(const VarMapResult* result, FILE* out) {
    if (!result || !out) return;
    for (size_t i = 0; i < result->count; i++) {
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
