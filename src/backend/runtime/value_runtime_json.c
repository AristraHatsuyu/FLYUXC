/*
 * Auto-generated fragment from value_runtime.c
 * Module: value_runtime_json.c
 */

/* ============================================================================
 * JSON 函数
 * ============================================================================ */

/* JSON 解析辅助函数 */
static const char* skip_whitespace(const char* str) {
    while (*str && (*str == ' ' || *str == '\t' || *str == '\n' || *str == '\r')) {
        str++;
    }
    return str;
}

/* 解析 JSON 字符串 */
static Value* parse_json_string(const char** ptr) {
    const char* p = *ptr;
    if (*p != '"') return NULL;
    p++; // 跳过开头的 "
    
    // 找到字符串结尾
    char buffer[4096];
    int i = 0;
    while (*p && *p != '"' && i < 4095) {
        if (*p == '\\' && *(p+1)) {
            p++; // 跳过反斜杠
            switch (*p) {
                case 'n': buffer[i++] = '\n'; break;
                case 't': buffer[i++] = '\t'; break;
                case 'r': buffer[i++] = '\r'; break;
                case '"': buffer[i++] = '"'; break;
                case '\\': buffer[i++] = '\\'; break;
                default: buffer[i++] = *p; break;
            }
            p++;
        } else {
            buffer[i++] = *p++;
        }
    }
    buffer[i] = '\0';
    
    if (*p == '"') p++; // 跳过结尾的 "
    *ptr = p;
    
    return box_string_owned(strdup(buffer));
}

/* 解析 JSON 数字 */
static Value* parse_json_number(const char** ptr) {
    const char* p = *ptr;
    char* endptr;
    double num = strtod(p, &endptr);
    *ptr = endptr;
    return box_number(num);
}

/* 前向声明 */
static Value* parse_json_value(const char** ptr);

/* 解析 JSON 数组 */
static Value* parse_json_array(const char** ptr) {
    const char* p = *ptr;
    if (*p != '[') return NULL;
    p++; // 跳过 [
    
    // 动态数组
    size_t capacity = 8;
    size_t count = 0;
    Value** elements = (Value**)malloc(capacity * sizeof(Value*));
    
    p = skip_whitespace(p);
    
    if (*p != ']') {
        while (1) {
            p = skip_whitespace(p);
            
            Value* elem = parse_json_value(&p);
            if (!elem) {
                // 解析元素失败，清理并返回 NULL
                free(elements);
                return NULL;
            }
            
            if (count >= capacity) {
                capacity *= 2;
                Value** new_elements = (Value**)realloc(elements, capacity * sizeof(Value*));
                if (!new_elements) {
                    free(elements);
                    return box_array(NULL, 0);
                }
                elements = new_elements;
            }
            
            elements[count++] = elem;
            
            p = skip_whitespace(p);
            if (*p == ',') {
                p++;
            } else {
                break;
            }
        }
    }
    
    p = skip_whitespace(p);
    if (*p == ']') p++; // 跳过 ]
    *ptr = p;
    
    return box_array((char*)elements, count);
}

/* 解析 JSON 对象 */
static Value* parse_json_object(const char** ptr) {
    const char* p = *ptr;
    if (*p != '{') return NULL;
    p++; // 跳过 {
    
    // 动态数组存储键值对
    size_t capacity = 8;
    size_t count = 0;
    ObjectEntry* entries = (ObjectEntry*)malloc(capacity * sizeof(ObjectEntry));
    
    p = skip_whitespace(p);
    
    if (*p != '}') {
        while (1) {
            p = skip_whitespace(p);
            
            // 解析键
            if (*p != '"') break;
            Value* key_val = parse_json_string(&p);
            if (!key_val) break;
            
            char* key = strdup((const char*)key_val->data.pointer);
            free(key_val);
            
            p = skip_whitespace(p);
            if (*p != ':') {
                free(key);
                break;
            }
            p++; // 跳过 :
            
            p = skip_whitespace(p);
            Value* value = parse_json_value(&p);
            if (!value) {
                // 解析值失败，清理并返回 NULL
                free(key);
                for (size_t i = 0; i < count; i++) {
                    free(entries[i].key);
                }
                free(entries);
                return NULL;
            }
            
            if (count >= capacity) {
                capacity *= 2;
                ObjectEntry* new_entries = (ObjectEntry*)realloc(entries, capacity * sizeof(ObjectEntry));
                if (!new_entries) {
                    free(key);
                    free(entries);
                    return box_object(NULL, 0);
                }
                entries = new_entries;
            }
            
            entries[count].key = key;
            entries[count].value = value;
            count++;
            
            p = skip_whitespace(p);
            if (*p == ',') {
                p++;
            } else {
                break;
            }
        }
    }
    
    p = skip_whitespace(p);
    if (*p == '}') p++; // 跳过 }
    *ptr = p;
    
    return box_object((char*)entries, count);
}

/* 解析 JSON 值 */
static Value* parse_json_value(const char** ptr) {
    const char* p = skip_whitespace(*ptr);
    
    if (*p == '"') {
        // 字符串
        return parse_json_string(ptr);
    } else if (*p == '[') {
        // 数组
        return parse_json_array(ptr);
    } else if (*p == '{') {
        // 对象
        return parse_json_object(ptr);
    } else if (*p == 't' && strncmp(p, "true", 4) == 0) {
        // true
        *ptr = p + 4;
        return box_bool(1);
    } else if (*p == 'f' && strncmp(p, "false", 5) == 0) {
        // false
        *ptr = p + 5;
        return box_bool(0);
    } else if (*p == 'n' && strncmp(p, "null", 4) == 0) {
        // null
        *ptr = p + 4;
        return box_null();
    } else if (*p == '-' || (*p >= '0' && *p <= '9')) {
        // 数字
        return parse_json_number(ptr);
    }
    
    // 无法识别的字符，返回 NULL 表示解析失败
    return NULL;
}

/* parseJSON(str) -> obj - 解析 JSON 字符串 */
Value* value_parse_json(Value* json_str) {
    if (!json_str || json_str->type != VALUE_STRING) {
        set_runtime_status(FLYUX_TYPE_ERROR, "(parseJSON) argument must be a string");
        return box_null_typed(VALUE_OBJECT);  // 返回 obj 类型的 null
    }
    
    const char* str = (const char*)json_str->data.pointer;
    const char* ptr = str;
    ptr = skip_whitespace(ptr);
    
    // 检查是否为空或无效起始字符
    if (*ptr == '\0' || (*ptr != '{' && *ptr != '[' && *ptr != '"' &&
        *ptr != 't' && *ptr != 'f' && *ptr != 'n' && *ptr != '-' &&
        !(*ptr >= '0' && *ptr <= '9'))) {
        set_runtime_status(FLYUX_TYPE_ERROR, "(parseJSON) invalid JSON format");
        return box_null_typed(VALUE_OBJECT);  // 返回 obj 类型的 null
    }
    
    Value* result = parse_json_value(&ptr);
    
    if (!result) {
        set_runtime_status(FLYUX_TYPE_ERROR, "(parseJSON) parse error");
        return box_null_typed(VALUE_OBJECT);  // 返回 obj 类型的 null
    }
    
    set_runtime_status(FLYUX_OK, NULL);
    return result;
}

/* JSON 序列化辅助函数 */
static void append_string(char** buffer, size_t* size, size_t* capacity, const char* str) {
    size_t len = strlen(str);
    while (*size + len >= *capacity) {
        *capacity *= 2;
        char* new_buffer = (char*)realloc(*buffer, *capacity);
        if (!new_buffer) return;
        *buffer = new_buffer;
    }
    strcpy(*buffer + *size, str);
    *size += len;
}

static void append_char(char** buffer, size_t* size, size_t* capacity, char c) {
    if (*size + 1 >= *capacity) {
        *capacity *= 2;
        char* new_buffer = (char*)realloc(*buffer, *capacity);
        if (!new_buffer) return;
        *buffer = new_buffer;
    }
    (*buffer)[*size] = c;
    (*size)++;
    (*buffer)[*size] = '\0';
}

/* 循环引用检测栈 - 用于追踪正在序列化的对象/数组 */
#define MAX_JSON_DEPTH 256

/* 前向声明 */
static void serialize_value_to_json_impl(Value* v, char** buffer, size_t* size, size_t* capacity,
                                         Value** visited, int* visited_count);

/* 检查值是否在访问栈中（循环引用） */
static int is_circular_ref(Value* v, Value** visited, int visited_count) {
    if (v->type != VALUE_ARRAY && v->type != VALUE_OBJECT) {
        return 0;  // 只有数组和对象需要检查循环引用
    }
    for (int i = 0; i < visited_count; i++) {
        if (visited[i] == v) {
            return 1;
        }
    }
    return 0;
}

/* 将值序列化为 JSON（内部实现，带循环引用检测） */
static void serialize_value_to_json_impl(Value* v, char** buffer, size_t* size, size_t* capacity,
                                         Value** visited, int* visited_count) {
    if (!v) {
        append_string(buffer, size, capacity, "null");
        return;
    }
    
    // 检查循环引用
    if (is_circular_ref(v, visited, *visited_count)) {
        append_string(buffer, size, capacity, "\"[Circular]\"");
        return;
    }
    
    // 检查深度限制
    if (*visited_count >= MAX_JSON_DEPTH) {
        append_string(buffer, size, capacity, "\"[Max Depth Exceeded]\"");
        return;
    }
    
    switch (v->type) {
        case VALUE_NUMBER: {
            char num_buf[64];
            if (isinf(v->data.number)) {
                strcpy(num_buf, "null");
            } else if (isnan(v->data.number)) {
                strcpy(num_buf, "null");
            } else {
                snprintf(num_buf, sizeof(num_buf), "%.16g", v->data.number);
            }
            append_string(buffer, size, capacity, num_buf);
            break;
        }
        case VALUE_STRING: {
            append_char(buffer, size, capacity, '"');
            const char* str = (const char*)v->data.pointer;
            while (*str) {
                if (*str == '"') {
                    append_string(buffer, size, capacity, "\\\"");
                } else if (*str == '\\') {
                    append_string(buffer, size, capacity, "\\\\");
                } else if (*str == '\n') {
                    append_string(buffer, size, capacity, "\\n");
                } else if (*str == '\t') {
                    append_string(buffer, size, capacity, "\\t");
                } else if (*str == '\r') {
                    append_string(buffer, size, capacity, "\\r");
                } else {
                    append_char(buffer, size, capacity, *str);
                }
                str++;
            }
            append_char(buffer, size, capacity, '"');
            break;
        }
        case VALUE_BOOL:
            append_string(buffer, size, capacity, v->data.number != 0 ? "true" : "false");
            break;
        case VALUE_NULL:
            append_string(buffer, size, capacity, "null");
            break;
        case VALUE_UNDEF:
            append_string(buffer, size, capacity, "null");
            break;
        case VALUE_ARRAY: {
            // 将当前数组加入访问栈
            visited[*visited_count] = v;
            (*visited_count)++;
            
            append_char(buffer, size, capacity, '[');
            Value** arr = (Value**)v->data.pointer;
            for (long i = 0; i < v->array_size; i++) {
                if (i > 0) append_char(buffer, size, capacity, ',');
                serialize_value_to_json_impl(arr[i], buffer, size, capacity, visited, visited_count);
            }
            append_char(buffer, size, capacity, ']');
            
            // 从访问栈中移除
            (*visited_count)--;
            break;
        }
        case VALUE_OBJECT: {
            // 检查是否为扩展对象类型
            if (v->ext_type != EXT_TYPE_NONE) {
                // 扩展类型显示为类型名字符串
                append_char(buffer, size, capacity, '"');
                switch (v->ext_type) {
                    case EXT_TYPE_BUFFER:
                        append_string(buffer, size, capacity, "Buffer");
                        break;
                    case EXT_TYPE_FILE:
                        append_string(buffer, size, capacity, "FileHandle");
                        break;
                    case EXT_TYPE_ERROR:
                        append_string(buffer, size, capacity, "Error");
                        break;
                    default:
                        append_string(buffer, size, capacity, "ExtendedObject");
                        break;
                }
                append_char(buffer, size, capacity, '"');
            } else {
                // 将当前对象加入访问栈
                visited[*visited_count] = v;
                (*visited_count)++;
                
                // 普通对象
                append_char(buffer, size, capacity, '{');
                ObjectEntry* entries = (ObjectEntry*)v->data.pointer;
                for (long i = 0; i < v->array_size; i++) {
                    if (i > 0) append_char(buffer, size, capacity, ',');
                    
                    // 键
                    append_char(buffer, size, capacity, '"');
                    append_string(buffer, size, capacity, entries[i].key);
                    append_char(buffer, size, capacity, '"');
                    append_char(buffer, size, capacity, ':');
                    
                    // 值
                    serialize_value_to_json_impl(entries[i].value, buffer, size, capacity, visited, visited_count);
                }
                append_char(buffer, size, capacity, '}');
                
                // 从访问栈中移除
                (*visited_count)--;
            }
            break;
        }
        default:
            append_string(buffer, size, capacity, "null");
    }
}

/* 包装函数 - 保持原有接口 */
static void serialize_value_to_json(Value* v, char** buffer, size_t* size, size_t* capacity) {
    Value* visited[MAX_JSON_DEPTH];
    int visited_count = 0;
    serialize_value_to_json_impl(v, buffer, size, capacity, visited, &visited_count);
}

/* toJSON(obj) -> str - 将值转换为 JSON 字符串 */
Value* value_to_json(Value* obj) {
    size_t capacity = 256;
    size_t size = 0;
    char* buffer = (char*)malloc(capacity);
    buffer[0] = '\0';
    
    serialize_value_to_json(obj, &buffer, &size, &capacity);
    
    Value* result = box_string_owned(buffer);
    set_runtime_status(FLYUX_OK, NULL);
    return result;
}

