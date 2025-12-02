/*
 * Auto-generated fragment from value_runtime.c
 * Module: value_runtime_string.c
 */

/* ============================================================================
 * UTF-8 辅助函数
 * ============================================================================
 */

/*
 * 计算 UTF-8 字符串的字符数（不是字节数）
 */
static size_t utf8_strlen(const char *s) {
    size_t count = 0;
    while (*s) {
        // UTF-8 continuation bytes 以 10xxxxxx 开头，跳过它们
        if ((*s & 0xC0) != 0x80) {
            count++;
        }
        s++;
    }
    return count;
}

/*
 * 获取第 n 个 UTF-8 字符的字节偏移量
 * 返回 -1 表示索引超出范围
 */
static int utf8_char_to_byte_offset(const char *s, int char_index) {
    int byte_offset = 0;
    int char_count = 0;
    
    while (s[byte_offset]) {
        if (char_count == char_index) {
            return byte_offset;
        }
        // 跳过当前字符的所有字节
        if ((s[byte_offset] & 0x80) == 0) {
            // ASCII 字符（1 字节）
            byte_offset += 1;
        } else if ((s[byte_offset] & 0xE0) == 0xC0) {
            // 2 字节字符
            byte_offset += 2;
        } else if ((s[byte_offset] & 0xF0) == 0xE0) {
            // 3 字节字符
            byte_offset += 3;
        } else if ((s[byte_offset] & 0xF8) == 0xF0) {
            // 4 字节字符
            byte_offset += 4;
        } else {
            // 无效的 UTF-8，当作单字节处理
            byte_offset += 1;
        }
        char_count++;
    }
    
    // 如果 char_index 正好等于字符数，返回字符串末尾偏移量
    if (char_count == char_index) {
        return byte_offset;
    }
    
    return -1;  // 索引超出范围
}

/*
 * 获取单个 UTF-8 字符的字节长度
 */
static int utf8_char_byte_length(const char *s) {
    if (!s || !*s) return 0;
    
    unsigned char c = (unsigned char)*s;
    if ((c & 0x80) == 0) return 1;       // ASCII
    if ((c & 0xE0) == 0xC0) return 2;    // 2 字节
    if ((c & 0xF0) == 0xE0) return 3;    // 3 字节
    if ((c & 0xF8) == 0xF0) return 4;    // 4 字节
    return 1;  // 无效的 UTF-8
}

/* ============================================================================
 * 字符串处理函数
 * ============================================================================
 */

/*
 * len(value) - 获取长度
 * 字符串：返回字符数（UTF-8 字符）
 * 数组：返回元素个数
 * 对象：返回字段个数
 * 其他：返回0
 */
Value* value_len(Value *v) {
    set_runtime_status(FLYUX_OK, NULL);
    
    if (!v || v->type == VALUE_NULL) {
        set_runtime_status(FLYUX_TYPE_ERROR, "(len) argument must be a string, array, or object");
        return box_null_typed(VALUE_NUMBER);
    }
    
    switch (v->type) {
        case VALUE_STRING:
            // 返回 UTF-8 字符数，而不是字节数
            return box_number((double)utf8_strlen((const char*)v->data.pointer));
        case VALUE_ARRAY:
        case VALUE_OBJECT:
            return box_number((double)v->array_size);
        default:
            set_runtime_status(FLYUX_TYPE_ERROR, "(len) argument must be a string, array, or object");
            return box_null_typed(VALUE_NUMBER);
    }
}

/*
 * charAt(str, index) - 获取指定位置的字符或数组元素
 * 支持字符串和数组，字符串索引是 UTF-8 字符索引
 */
Value* value_char_at(Value *str, Value *index) {
    set_runtime_status(FLYUX_OK, NULL);
    
    // 支持数组访问
    if (str && str->type == VALUE_ARRAY) {
        if (!index || index->type != VALUE_NUMBER) {
            set_runtime_status(FLYUX_TYPE_ERROR, "(charAt) requires numeric index");
            return box_null();
        }
        
        int idx = (int)index->data.number;
        if (idx < 0 || idx >= (int)str->array_size) {
            set_runtime_status(FLYUX_OUT_OF_BOUNDS, "(charAt) array index out of range");
            return box_null();
        }
        
        Value **elements = (Value**)str->data.pointer;
        return elements[idx];
    }
    
    // 支持字符串访问
    if (!str || str->type != VALUE_STRING) {
        set_runtime_status(FLYUX_TYPE_ERROR, "(charAt) requires string or array");
        return box_null_typed(VALUE_STRING);
    }
    
    if (!index || index->type != VALUE_NUMBER) {
        set_runtime_status(FLYUX_TYPE_ERROR, "(charAt) requires numeric index");
        return box_null_typed(VALUE_STRING);
    }
    
    const char *s = (const char*)str->data.pointer;
    int idx = (int)index->data.number;
    size_t char_count = utf8_strlen(s);
    
    if (idx < 0 || idx >= (int)char_count) {
        set_runtime_status(FLYUX_OUT_OF_BOUNDS, "(charAt) index out of range");
        return box_null_typed(VALUE_STRING);
    }
    
    // 找到第 idx 个 UTF-8 字符的字节偏移量
    int byte_offset = utf8_char_to_byte_offset(s, idx);
    if (byte_offset < 0) {
        return box_string("");
    }
    
    // 获取该字符的字节长度
    int char_len = utf8_char_byte_length(s + byte_offset);
    
    // 复制该字符
    char *result = (char*)malloc(char_len + 1);
    memcpy(result, s + byte_offset, char_len);
    result[char_len] = '\0';
    
    return box_string_owned(result);
}

/*
 * substr(str, start, length) - 获取子字符串
 * start 和 length 都是 UTF-8 字符索引，不是字节索引
 */
Value* value_substr(Value *str, Value *start, Value *length) {
    set_runtime_status(FLYUX_OK, NULL);
    
    if (!str || str->type != VALUE_STRING) {
        set_runtime_status(FLYUX_TYPE_ERROR, "(substr) requires string");
        return box_string("");
    }
    
    if (!start || start->type != VALUE_NUMBER) {
        set_runtime_status(FLYUX_TYPE_ERROR, "(substr) requires numeric start");
        return box_string("");
    }
    
    const char *s = (const char*)str->data.pointer;
    size_t char_count = utf8_strlen(s);
    int start_idx = (int)start->data.number;
    int len = (length && length->type == VALUE_NUMBER) 
              ? (int)length->data.number 
              : (int)char_count - start_idx;
    
    if (start_idx < 0 || start_idx >= (int)char_count) {
        return box_string("");
    }
    
    if (len < 0) len = 0;
    if (start_idx + len > (int)char_count) {
        len = (int)char_count - start_idx;
    }
    
    // 找到起始字符的字节偏移量
    int start_byte = utf8_char_to_byte_offset(s, start_idx);
    if (start_byte < 0) {
        return box_string("");
    }
    
    // 找到结束字符后一个位置的字节偏移量
    int end_byte = utf8_char_to_byte_offset(s, start_idx + len);
    if (end_byte < 0) {
        // 如果超出范围，使用字符串末尾
        end_byte = (int)str->string_length;
    }
    
    int byte_len = end_byte - start_byte;
    char *result = (char*)malloc(byte_len + 1);
    memcpy(result, s + start_byte, byte_len);
    result[byte_len] = '\0';
    
    return box_string_owned(result);
}

/*
 * indexOf(str/arr, substr/value) - 查找子字符串位置或数组元素索引
 * 支持字符串和数组两种类型
 * 对于字符串，返回 UTF-8 字符索引
 */
Value* value_index_of(Value *str, Value *substr) {
    set_runtime_status(FLYUX_OK, NULL);
    
    // 如果是数组，查找元素索引
    if (str && str->type == VALUE_ARRAY) {
        Value **elements = (Value**)str->data.pointer;
        for (size_t i = 0; i < str->array_size; i++) {
            Value *eq = value_equals(elements[i], substr);
            int is_equal = (eq && eq->type == VALUE_BOOL && eq->data.number != 0);
            value_release(eq);  // 释放 value_equals 返回的临时值
            if (is_equal) {
                return box_number((double)i);
            }
        }
        return box_number(-1);
    }
    
    // 否则作为字符串处理
    if (!str || str->type != VALUE_STRING || !substr || substr->type != VALUE_STRING) {
        set_runtime_status(FLYUX_TYPE_ERROR, "(indexOf) requires two strings or array and value");
        return box_number(-1);
    }
    
    const char *haystack = (const char*)str->data.pointer;
    const char *needle = (const char*)substr->data.pointer;
    const char *pos = strstr(haystack, needle);
    
    if (pos) {
        // 计算字节偏移量对应的字符索引
        int byte_offset = (int)(pos - haystack);
        // 计算从字符串开头到 pos 位置有多少个 UTF-8 字符
        int char_index = 0;
        for (int i = 0; i < byte_offset; ) {
            if ((haystack[i] & 0x80) == 0) {
                i += 1;  // ASCII
            } else if ((haystack[i] & 0xE0) == 0xC0) {
                i += 2;  // 2 字节
            } else if ((haystack[i] & 0xF0) == 0xE0) {
                i += 3;  // 3 字节
            } else if ((haystack[i] & 0xF8) == 0xF0) {
                i += 4;  // 4 字节
            } else {
                i += 1;  // 无效 UTF-8
            }
            char_index++;
        }
        return box_number((double)char_index);
    }
    return box_number(-1);
}

/*
 * replace(str, old, new) - 替换字符串（替换第一个匹配）
 */
Value* value_replace(Value *str, Value *old_str, Value *new_str) {
    set_runtime_status(FLYUX_OK, NULL);
    
    if (!str || str->type != VALUE_STRING || 
        !old_str || old_str->type != VALUE_STRING ||
        !new_str || new_str->type != VALUE_STRING) {
        set_runtime_status(FLYUX_TYPE_ERROR, "(replace) requires three strings");
        return box_string("");
    }
    
    const char *source = (const char*)str->data.pointer;
    const char *old = (const char*)old_str->data.pointer;
    const char *new = (const char*)new_str->data.pointer;
    
    const char *pos = strstr(source, old);
    if (!pos) {
        return box_string_owned(strdup(source));
    }
    
    size_t old_len = strlen(old);
    size_t new_len = strlen(new);
    size_t prefix_len = pos - source;
    size_t suffix_len = strlen(pos + old_len);
    size_t result_len = prefix_len + new_len + suffix_len;
    
    char *result = (char*)malloc(result_len + 1);
    memcpy(result, source, prefix_len);
    memcpy(result + prefix_len, new, new_len);
    memcpy(result + prefix_len + new_len, pos + old_len, suffix_len);
    result[result_len] = '\0';
    
    return box_string_owned(result);
}

/*
 * split(str, delimiter) - 分割字符串为数组
 */
Value* value_split(Value *str, Value *delimiter) {
    set_runtime_status(FLYUX_OK, NULL);
    
    if (!str || str->type != VALUE_STRING) {
        set_runtime_status(FLYUX_TYPE_ERROR, "(split) requires string");
        return box_null_typed(VALUE_OBJECT);
    }
    
    const char *source = (const char*)str->data.pointer;
    const char *delim = (delimiter && delimiter->type == VALUE_STRING) 
                        ? (const char*)delimiter->data.pointer : " ";
    
    // 计算分段数量
    size_t count = 1;
    const char *p = source;
    size_t delim_len = strlen(delim);
    
    if (delim_len == 0) {
        // 空分隔符：每个字符一个元素
        count = strlen(source);
    } else {
        while ((p = strstr(p, delim)) != NULL) {
            count++;
            p += delim_len;
        }
    }
    
    // 创建数组
    Value **elements = (Value**)malloc(count * sizeof(Value*));
    size_t idx = 0;
    
    if (delim_len == 0) {
        // 每个字符一个元素
        for (size_t i = 0; i < strlen(source); i++) {
            char temp[2] = {source[i], '\0'};
            elements[idx++] = box_string_owned(strdup(temp));
        }
    } else {
        p = source;
        const char *next;
        while ((next = strstr(p, delim)) != NULL) {
            size_t len = next - p;
            char *part = (char*)malloc(len + 1);
            memcpy(part, p, len);
            part[len] = '\0';
            elements[idx++] = box_string_owned(part);
            p = next + delim_len;
        }
        // 最后一段
        elements[idx++] = box_string_owned(strdup(p));
    }
    
    Value *result = (Value*)malloc(sizeof(Value));
    result->type = VALUE_ARRAY;
    result->declared_type = VALUE_ARRAY;
    result->refcount = 1;
    result->flags = VALUE_FLAG_NONE;
    result->ext_type = EXT_TYPE_NONE;
    result->data.pointer = elements;
    result->array_size = count;
    result->string_length = 0;
    
    return result;
}

/*
 * join(array, separator) - 将数组元素连接为字符串
 */
Value* value_join(Value *arr, Value *separator) {
    set_runtime_status(FLYUX_OK, NULL);
    
    if (!arr || arr->type != VALUE_ARRAY) {
        set_runtime_status(FLYUX_TYPE_ERROR, "(join) requires array");
        return box_string("");
    }
    
    const char *sep = (separator && separator->type == VALUE_STRING)
                      ? (const char*)separator->data.pointer : ",";
    
    size_t arr_size = arr->array_size;
    if (arr_size == 0) {
        return box_string("");
    }
    
    Value **elements = (Value**)arr->data.pointer;
    size_t sep_len = strlen(sep);
    
    // 一次性转换所有元素为字符串并缓存
    Value **str_vals = (Value**)malloc(arr_size * sizeof(Value*));
    const char **strs = (const char**)malloc(arr_size * sizeof(const char*));
    size_t *lens = (size_t*)malloc(arr_size * sizeof(size_t));
    
    // 计算总长度，同时缓存转换结果
    size_t total_len = 0;
    for (size_t i = 0; i < arr_size; i++) {
        str_vals[i] = value_to_str(elements[i]);
        strs[i] = (const char*)str_vals[i]->data.pointer;
        lens[i] = strlen(strs[i]);
        total_len += lens[i];
        if (i < arr_size - 1) {
            total_len += sep_len;
        }
    }
    
    // 构建结果字符串
    char *result = (char*)malloc(total_len + 1);
    char *ptr = result;
    
    for (size_t i = 0; i < arr_size; i++) {
        memcpy(ptr, strs[i], lens[i]);
        ptr += lens[i];
        
        if (i < arr_size - 1) {
            memcpy(ptr, sep, sep_len);
            ptr += sep_len;
        }
    }
    *ptr = '\0';
    
    // 释放所有临时 Value
    for (size_t i = 0; i < arr_size; i++) {
        value_release(str_vals[i]);
    }
    
    // 释放辅助数组
    free(str_vals);
    free(strs);
    free(lens);
    
    return box_string_owned(result);
}

/*
 * trim(str) - 去除首尾空白字符
 */
Value* value_trim(Value *str) {
    set_runtime_status(FLYUX_OK, NULL);
    
    if (!str || str->type != VALUE_STRING) {
        set_runtime_status(FLYUX_TYPE_ERROR, "(trim) requires string");
        return box_string("");
    }
    
    const char *source = (const char*)str->data.pointer;
    
    // 找到第一个非空白字符
    const char *start = source;
    while (*start && isspace((unsigned char)*start)) start++;
    
    // 找到最后一个非空白字符
    const char *end = source + strlen(source) - 1;
    while (end > start && isspace((unsigned char)*end)) end--;
    
    size_t len = end - start + 1;
    char *result = (char*)malloc(len + 1);
    memcpy(result, start, len);
    result[len] = '\0';
    
    return box_string_owned(result);
}

/*
 * upper(str) - 转换为大写
 */
Value* value_upper(Value *str) {
    set_runtime_status(FLYUX_OK, NULL);
    
    if (!str || str->type != VALUE_STRING) {
        set_runtime_status(FLYUX_TYPE_ERROR, "(upper) requires string");
        return box_string("");
    }
    
    const char *source = (const char*)str->data.pointer;
    size_t len = strlen(source);
    char *result = (char*)malloc(len + 1);
    
    for (size_t i = 0; i < len; i++) {
        result[i] = toupper((unsigned char)source[i]);
    }
    result[len] = '\0';
    
    return box_string_owned(result);
}

/*
 * lower(str) - 转换为小写
 */
Value* value_lower(Value *str) {
    set_runtime_status(FLYUX_OK, NULL);
    
    if (!str || str->type != VALUE_STRING) {
        set_runtime_status(FLYUX_TYPE_ERROR, "(lower) requires string");
        return box_string("");
    }
    
    const char *source = (const char*)str->data.pointer;
    size_t len = strlen(source);
    char *result = (char*)malloc(len + 1);
    
    for (size_t i = 0; i < len; i++) {
        result[i] = tolower((unsigned char)source[i]);
    }
    result[len] = '\0';
    
    return box_string_owned(result);
}

