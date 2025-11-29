/*
 * Auto-generated fragment from value_runtime.c
 * Module: value_runtime_string.c
 */

/* ============================================================================
 * 字符串处理函数
 * ============================================================================
 */

/*
 * len(value) - 获取长度
 * 字符串：返回字符数
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
            return box_number((double)v->string_length);
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
 * 支持字符串和数组
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
        return box_string("");
    }
    
    if (!index || index->type != VALUE_NUMBER) {
        set_runtime_status(FLYUX_TYPE_ERROR, "(charAt) requires numeric index");
        return box_string("");
    }
    
    int idx = (int)index->data.number;
    if (idx < 0 || idx >= (int)str->string_length) {
        set_runtime_status(FLYUX_OUT_OF_BOUNDS, "(charAt) index out of range");
        return box_string("");
    }
    
    char result[2] = {((char*)str->data.pointer)[idx], '\0'};
    return box_string(strdup(result));
}

/*
 * substr(str, start, length) - 获取子字符串
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
    
    int start_idx = (int)start->data.number;
    int len = (length && length->type == VALUE_NUMBER) ? (int)length->data.number : str->string_length;
    
    if (start_idx < 0 || start_idx >= (int)str->string_length) {
        return box_string("");
    }
    
    if (len < 0) len = 0;
    if (start_idx + len > (int)str->string_length) {
        len = str->string_length - start_idx;
    }
    
    char *result = (char*)malloc(len + 1);
    memcpy(result, ((char*)str->data.pointer) + start_idx, len);
    result[len] = '\0';
    
    return box_string(result);
}

/*
 * indexOf(str/arr, substr/value) - 查找子字符串位置或数组元素索引
 * 支持字符串和数组两种类型
 */
Value* value_index_of(Value *str, Value *substr) {
    set_runtime_status(FLYUX_OK, NULL);
    
    // 如果是数组，查找元素索引
    if (str && str->type == VALUE_ARRAY) {
        Value **elements = (Value**)str->data.pointer;
        for (size_t i = 0; i < str->array_size; i++) {
            Value *eq = value_equals(elements[i], substr);
            if (eq && eq->type == VALUE_BOOL && eq->data.number != 0) {
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
        return box_number((double)(pos - haystack));
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
        return box_string(strdup(source));
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
    
    return box_string(result);
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
            elements[idx++] = box_string(strdup(temp));
        }
    } else {
        p = source;
        const char *next;
        while ((next = strstr(p, delim)) != NULL) {
            size_t len = next - p;
            char *part = (char*)malloc(len + 1);
            memcpy(part, p, len);
            part[len] = '\0';
            elements[idx++] = box_string(part);
            p = next + delim_len;
        }
        // 最后一段
        elements[idx++] = box_string(strdup(p));
    }
    
    Value *result = (Value*)malloc(sizeof(Value));
    result->type = VALUE_ARRAY;
    result->declared_type = VALUE_ARRAY;
    result->data.pointer = elements;
    result->array_size = count;
    
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
    
    if (arr->array_size == 0) {
        return box_string("");
    }
    
    // 计算总长度
    size_t total_len = 0;
    Value **elements = (Value**)arr->data.pointer;
    
    for (size_t i = 0; i < arr->array_size; i++) {
        Value *str_val = value_to_str(elements[i]);
        total_len += strlen((char*)str_val->data.pointer);
        if (i < arr->array_size - 1) {
            total_len += strlen(sep);
        }
    }
    
    // 构建结果
    char *result = (char*)malloc(total_len + 1);
    char *ptr = result;
    
    for (size_t i = 0; i < arr->array_size; i++) {
        Value *str_val = value_to_str(elements[i]);
        const char *str = (const char*)str_val->data.pointer;
        size_t len = strlen(str);
        memcpy(ptr, str, len);
        ptr += len;
        
        if (i < arr->array_size - 1) {
            size_t sep_len = strlen(sep);
            memcpy(ptr, sep, sep_len);
            ptr += sep_len;
        }
    }
    *ptr = '\0';
    
    return box_string(result);
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
    
    return box_string(result);
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
    
    return box_string(result);
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
    
    return box_string(result);
}

