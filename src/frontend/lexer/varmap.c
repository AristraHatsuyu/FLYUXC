#include "flyuxc/frontend/varmap.h"
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

    /* break 关键字 */
    if (len == 5 && name[0]=='b' && name[1]=='r' && name[2]=='e' && name[3]=='a' && name[4]=='k') return 1;

    /* L / R / T 用于 L> / R> / T>，简单起见直接视为保留 */
    if (len == 1 && (name[0] == 'L' || name[0] == 'R' || name[0] == 'T')) return 1;

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
    
    /* 字符串操作 (17) */
    "substr", "indexOf", "replace", "split", "join",
    "toUpper", "toLower", "trim", "startsWith", "endsWith", "contains",
    "len", "charAt", "upper", "lower",
    
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
    
    /* 时间函数 (6) */
    "now", "sleep", "dateStr", "time", "date",
    
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

VarMapResult flyux_varmap_process(const char* normalized_source,
                                  const SourceLocation* source_map,
                                  size_t source_map_size) {
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

        if (!in_str && (c == '"' || c == 39)) {  /* 39 is '\'' */
            /* 进入字符串 */
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
            in_str = 1;
            str_quote = c;
            i++;
            continue;
        }

        if (in_str && c == str_quote) {
            /* 退出字符串 */
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
            in_str = 0;
            str_quote = 0;
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
                    /* 区分类型注解/变量定义、对象 key、以及 foreach 循环中的迭代变量 */
                    // 检查是否是 foreach: L> (arr : item) 或 L>arr:item
                    // 向前搜索，看是否有 "L>" 后面跟 "(" 或直接跟标识符
                    int is_foreach = 0;
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
                    
                    if (!is_foreach && !looks_like_typed_definition(normalized_source, len, j)) {
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

    // 构建 offset_map：映射输出偏移到输入偏移
    // offset_map[i] = j 表示输出字符i对应输入字符j
    size_t* offset_map = malloc(out_idx * sizeof(size_t));
    if (!offset_map) {
        free(out);
        for (size_t k = 0; k < entry_count; k++) {
            free(entries[k].original);
            free(entries[k].mapped);
        }
        free(entries);
        result.error_code = -1;
        result.error_msg = str_dup_n("Memory allocation failed for offset_map", 41);
        return result;
    }
    
    // 重新扫描normalized_source和out，建立偏移映射
    size_t norm_i = 0, map_i = 0;
    int in_string = 0, in_escape = 0;
    
    while (norm_i < len && map_i < out_idx) {
        // 处理转义和字符串（与上面逻辑一致）
        if (in_escape) {
            offset_map[map_i++] = norm_i++;
            in_escape = 0;
            continue;
        }
        
        if (normalized_source[norm_i] == '\\') {
            in_escape = 1;
            offset_map[map_i++] = norm_i++;
            continue;
        }
        
        if (normalized_source[norm_i] == '"' || normalized_source[norm_i] == '\'') {
            in_string = !in_string;
            offset_map[map_i++] = norm_i++;
            continue;
        }
        
        if (in_string) {
            offset_map[map_i++] = norm_i++;
            continue;
        }
        
        // 检查是否是标识符开头
        if (is_ident_start(normalized_source[norm_i])) {
            size_t ident_start = norm_i;
            size_t ident_len = 0;
            while (norm_i < len && is_ident_char(normalized_source[norm_i])) {
                ident_len++;
                norm_i++;
            }
            
            // 检查是否是对象key（标识符后面是 : 且不是类型定义）
            int is_object_key = 0;
            if (norm_i < len && normalized_source[norm_i] == ':') {
                if (!looks_like_typed_definition(normalized_source, len, norm_i)) {
                    is_object_key = 1;
                }
            }
            
            // 检查是否被映射（对象key不参与映射）
            int was_mapped = 0;
            if (!is_object_key) {
                for (size_t e = 0; e < entry_count; e++) {
                    size_t orig_len = strlen(entries[e].original);
                    if (orig_len == ident_len && 
                        strncmp(normalized_source + ident_start, entries[e].original, ident_len) == 0) {
                        // 这个标识符被映射了
                        size_t mapped_len = strlen(entries[e].mapped);
                        // 映射后的所有字符都指向原标识符的起始位置
                        for (size_t k = 0; k < mapped_len; k++) {
                            offset_map[map_i++] = ident_start;
                        }
                        was_mapped = 1;
                        break;
                    }
                }
            }
            
            if (!was_mapped) {
                // 未映射，1:1对应
                for (size_t k = 0; k < ident_len; k++) {
                    offset_map[map_i++] = ident_start + k;
                }
            }
        } else {
            // 非标识符，1:1映射
            offset_map[map_i++] = norm_i++;
        }
    }

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
