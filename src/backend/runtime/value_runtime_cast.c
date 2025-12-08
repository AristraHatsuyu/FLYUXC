/*
 * Auto-generated fragment from value_runtime.c
 * Module: value_runtime_cast.c
 */

/* 前向声明 */
Value* value_delete_field(Value *obj, Value *field_name);
void value_fatal_error(void);    // 致命错误处理（终止程序）

/* ============================================================================
 * 哈希表支持 - 用于对象字段的快速查找
 * 使用线性探测开放寻址法
 * ============================================================================
 */

/* FNV-1a 哈希函数 - 简单高效 */
static inline unsigned long hash_string(const char *str) {
    unsigned long hash = 14695981039346656037UL;  // FNV offset basis
    while (*str) {
        hash ^= (unsigned char)*str++;
        hash *= 1099511628211UL;  // FNV prime
    }
    return hash;
}

/* 
 * 对象哈希表结构：
 * - data.pointer 指向 ObjectEntry 数组
 * - array_size 存储实际字段数量
 * - string_length 存储哈希表容量（如果为0表示线性模式）
 * 
 * 当 string_length > 0 时，使用开放寻址哈希表：
 * - entries[i].key == NULL 表示空槽
 * - entries[i].key == (char*)-1 表示已删除的槽（墓碑）
 */
#define OBJECT_HASH_THRESHOLD 8
#define OBJECT_INITIAL_HASH_CAPACITY 32
#define OBJECT_TOMBSTONE ((char*)(intptr_t)-1)

/* 检查对象是否使用哈希模式 */
static inline int object_is_hash_mode(Value *obj) {
    return obj->string_length > 0;
}

/* 在哈希表中查找键，返回槽索引（找到或空槽），-1表示表满 */
static inline long object_hash_find_slot(ObjectEntry *entries, size_t capacity, const char *key, unsigned long hash) {
    size_t idx = hash % capacity;
    size_t start = idx;
    long first_tombstone = -1;
    
    do {
        if (entries[idx].key == NULL) {
            // 空槽：如果有墓碑返回墓碑位置，否则返回当前位置
            return first_tombstone >= 0 ? first_tombstone : (long)idx;
        }
        if (entries[idx].key == OBJECT_TOMBSTONE) {
            // 记录第一个墓碑位置
            if (first_tombstone < 0) first_tombstone = (long)idx;
        } else if (strcmp(entries[idx].key, key) == 0) {
            // 找到匹配的键
            return (long)idx;
        }
        idx = (idx + 1) % capacity;
    } while (idx != start);
    
    // 表满，返回墓碑位置或-1
    return first_tombstone;
}

/* 将对象从线性模式转换为哈希模式 */
static void object_convert_to_hash(Value *obj) {
    ObjectEntry *old_entries = (ObjectEntry*)obj->data.pointer;
    size_t old_count = obj->array_size;
    
    // 计算新容量（至少是当前数量的2倍）
    size_t new_capacity = OBJECT_INITIAL_HASH_CAPACITY;
    while (new_capacity < old_count * 2) {
        new_capacity *= 2;
    }
    
    // 分配新的哈希表
    ObjectEntry *new_entries = (ObjectEntry*)calloc(new_capacity, sizeof(ObjectEntry));
    if (!new_entries) return;  // 内存分配失败，保持线性模式
    
    // 重新插入所有条目
    for (size_t i = 0; i < old_count; i++) {
        if (old_entries[i].key && old_entries[i].key != OBJECT_TOMBSTONE) {
            unsigned long hash = hash_string(old_entries[i].key);
            long slot = object_hash_find_slot(new_entries, new_capacity, old_entries[i].key, hash);
            if (slot >= 0) {
                new_entries[slot].key = old_entries[i].key;
                new_entries[slot].value = old_entries[i].value;
            }
        }
    }
    
    // 释放旧数组
    if (old_entries) free(old_entries);
    
    // 更新对象
    obj->data.pointer = new_entries;
    obj->string_length = new_capacity;  // 标记为哈希模式并存储容量
}

/* 哈希表扩容 */
static void object_hash_resize(Value *obj, size_t new_capacity) {
    ObjectEntry *old_entries = (ObjectEntry*)obj->data.pointer;
    size_t old_capacity = obj->string_length;
    
    // 分配新的哈希表
    ObjectEntry *new_entries = (ObjectEntry*)calloc(new_capacity, sizeof(ObjectEntry));
    if (!new_entries) return;  // 内存分配失败
    
    // 重新插入所有条目
    for (size_t i = 0; i < old_capacity; i++) {
        if (old_entries[i].key && old_entries[i].key != OBJECT_TOMBSTONE) {
            unsigned long hash = hash_string(old_entries[i].key);
            long slot = object_hash_find_slot(new_entries, new_capacity, old_entries[i].key, hash);
            if (slot >= 0) {
                new_entries[slot].key = old_entries[i].key;
                new_entries[slot].value = old_entries[i].value;
            }
        }
    }
    
    // 释放旧数组
    free(old_entries);
    
    // 更新对象
    obj->data.pointer = new_entries;
    obj->string_length = new_capacity;
}

/* ============================================================================
 * 类型转换函数
 * ============================================================================
 */

/*
 * toNum(value) - 转换为数字
 * 
 * 支持的转换:
 *   - 数字 → 数字 (直接返回)
 *   - 字符串 → 数字 (解析，失败返回 0 并设置错误)
 *   - 布尔值 → 数字 (true=1, false=0)
 *   - null/undef → 0
 *   - 其他 → 0 (并设置类型错误)
 * 
 * 状态码:
 *   FLYUX_OK: 转换成功
 *   FLYUX_TYPE_ERROR: 无法转换的类型或格式错误
 */
Value* value_to_num(Value *v) {
    set_runtime_status(FLYUX_OK, NULL);
    
    if (!v || v->type == VALUE_UNDEF) {
        return box_number(0.0);
    }
    
    switch (v->type) {
        case VALUE_NUMBER:
            return box_number(v->data.number);
            
        case VALUE_STRING: {
            if (!v->data.string || v->string_length == 0) {
                set_runtime_status(FLYUX_TYPE_ERROR, "(toNum) Empty string cannot be converted to number");
                return box_null_typed(VALUE_NUMBER);
            }
            
            char *endptr;
            double result = strtod(v->data.string, &endptr);
            
            // 检查是否有无效字符
            if (endptr == v->data.string || *endptr != '\0') {
                set_runtime_status(FLYUX_TYPE_ERROR, "(toNum) Invalid number format");
                return box_null_typed(VALUE_NUMBER);
            }
            
            return box_number(result);
        }
        
        case VALUE_BOOL:
            return box_number(v->data.number != 0 ? 1.0 : 0.0);
            
        case VALUE_NULL:
            // null 传递：toNum(null) 返回 null，不设置错误
            return box_null_typed(VALUE_NUMBER);
            
        default:
            set_runtime_status(FLYUX_TYPE_ERROR, "(toNum) Cannot convert array/object to number");
            return box_null_typed(VALUE_NUMBER);
    }
}

/*
 * toStr(value) - 转换为字符串
 * 
 * 支持的转换:
 *   - 字符串 → 字符串 (直接返回)
 *   - 数字 → 字符串 (格式化)
 *   - 布尔值 → "true" / "false"
 *   - null → "null"
 *   - undef → "undef"
 *   - 数组/对象 → JSON 格式字符串
 * 
 * 状态码:
 *   FLYUX_OK: 转换成功
 */
Value* value_to_str(Value *v) {
    set_runtime_status(FLYUX_OK, NULL);
    
    if (!v || v->type == VALUE_UNDEF) {
        char *str = strdup("undef");
        return box_string_owned(str);
    }
    
    char buffer[256];
    
    switch (v->type) {
        case VALUE_STRING: {
            // 复制字符串
            char *str = (char*)malloc(v->string_length + 1);
            memcpy(str, v->data.string, v->string_length);
            str[v->string_length] = '\0';
            return box_string_owned(str);
        }
        
        case VALUE_NUMBER: {
            // 格式化数字，去除不必要的小数点
            double num = v->data.number;
            if (floor(num) == num && fabs(num) < 1e15) {
                // 整数
                snprintf(buffer, sizeof(buffer), "%.0f", num);
            } else {
                // 浮点数
                snprintf(buffer, sizeof(buffer), "%.16g", num);
            }
            char *str = strdup(buffer);
            return box_string_owned(str);
        }
        
        case VALUE_BOOL:
            return box_string_owned(strdup(v->data.number != 0 ? "true" : "false"));
            
        case VALUE_NULL:
            return box_string_owned(strdup("null"));
            
        case VALUE_ARRAY:
        case VALUE_OBJECT: {
            // 使用 JSON 格式
            value_to_string(v, buffer, sizeof(buffer));
            char *str = strdup(buffer);
            return box_string_owned(str);
        }

        case VALUE_FUNCTION:
            return box_string_owned(strdup("[Function]"));
        
        default:
            return box_string_owned(strdup("unknown"));
    }
}

/*
 * toBl(value) - 转换为布尔值
 * 
 * 支持的转换:
 *   - 布尔值 → 布尔值 (直接返回)
 *   - 数字 → 布尔值 (0=false, 其他=true)
 *   - 字符串 → 布尔值 (空串=false, 其他=true)
 *   - null/undef → false
 *   - 数组/对象 → true
 * 
 * 状态码:
 *   FLYUX_OK: 转换成功
 */
Value* value_to_bl(Value *v) {
    set_runtime_status(FLYUX_OK, NULL);
    
    if (!v || v->type == VALUE_UNDEF || v->type == VALUE_NULL) {
        return box_bool(0);
    }
    
    switch (v->type) {
        case VALUE_BOOL:
            return box_bool(v->data.number != 0);
            
        case VALUE_NUMBER:
            // 0, NaN → false, 其他 → true
            return box_bool(v->data.number != 0 && !isnan(v->data.number));
            
        case VALUE_STRING:
            // 空字符串 → false, 其他 → true
            return box_bool(v->string_length > 0);
            
        case VALUE_ARRAY:
        case VALUE_OBJECT:
            // 数组和对象总是 true
            return box_bool(1);
            
        default:
            return box_bool(0);
    }
}

/*
 * create_error_object - 创建详细的错误对象
 * 
 * 参数：
 *   message: 错误消息（Value字符串）
 *   code: 错误代码（Value数字）
 *   type: 错误类型名称（Value字符串）
 * 
 * 返回：
 *   包含 {message, code, type} 三个字段的对象
 * 
 * 示例：
 *   error := create_error_object("Invalid number", 3, "TypeError")
 *   println(error.message)  // "Invalid number"
 *   println(error.code)     // 3
 *   println(error.type)     // "TypeError"
 */
Value* create_error_object(Value *message, Value *code, Value *type) {
    // 创建3个键值对
    ObjectEntry *entries = (ObjectEntry*)malloc(3 * sizeof(ObjectEntry));
    
    // message字段
    entries[0].key = strdup("message");
    entries[0].value = message;
    
    // code字段
    entries[1].key = strdup("code");
    entries[1].value = code;
    
    // type字段
    entries[2].key = strdup("type");
    entries[2].value = type;
    
    // 创建对象Value
    Value *obj = (Value*)malloc(sizeof(Value));
    obj->type = VALUE_OBJECT;
    obj->declared_type = VALUE_OBJECT;
    obj->data.pointer = entries;
    obj->array_size = 3;  // 3个键值对
    
    return obj;
}

/*
 * value_get_field - 从对象中获取指定字段的值
 * 
 * 参数：
 *   obj: 对象Value（必须是VALUE_OBJECT类型）
 *   field_name: 字段名（Value字符串）
 * 
 * 返回：
 *   字段的Value，如果找不到则返回null
 * 
 * 示例：
 *   msg := value_get_field(error, "message")
 */
Value* value_get_field(Value *obj, Value *field_name) {
    if (!obj || !field_name) {
        set_runtime_status(FLYUX_TYPE_ERROR, "(getField) Null argument");
        return box_null();
    }
    
    // 检查obj是否为对象类型
    if (obj->type != VALUE_OBJECT) {
        set_runtime_status(FLYUX_TYPE_ERROR, "(getField) Not an object");
        return box_null();
    }
    
    // 检查field_name是否为字符串
    if (field_name->type != VALUE_STRING) {
        set_runtime_status(FLYUX_TYPE_ERROR, "(getField) Field name must be a string");
        return box_null();
    }
    
    // 获取字段名
    const char *key = (const char*)field_name->data.pointer;
    
    // 如果是扩展类型对象，先检查虚拟属性
    if (obj->ext_type != EXT_TYPE_NONE) {
        switch (obj->ext_type) {
            case EXT_TYPE_BUFFER: {
                BufferObject *buf = (BufferObject*)obj->data.pointer;
                if (strcmp(key, "size") == 0) {
                    return box_number((double)buf->size);
                }
                if (strcmp(key, "capacity") == 0) {
                    return box_number((double)buf->capacity);
                }
                if (strcmp(key, "type") == 0) {
                    return box_string("Buffer");
                }
                break;
            }
            case EXT_TYPE_FILE: {
                FileHandleObject *file = (FileHandleObject*)obj->data.pointer;
                if (strcmp(key, "path") == 0) {
                    return box_string(file->path);
                }
                if (strcmp(key, "mode") == 0) {
                    return box_string(file->mode);
                }
                if (strcmp(key, "isOpen") == 0) {
                    return box_bool(file->is_open);
                }
                if (strcmp(key, "position") == 0) {
                    return box_number((double)file->position);
                }
                if (strcmp(key, "type") == 0) {
                    return box_string("FileHandle");
                }
                break;
            }
            case EXT_TYPE_ERROR: {
                ErrorObject *err = (ErrorObject*)obj->data.pointer;
                if (strcmp(key, "message") == 0) {
                    return box_string(err->message);
                }
                if (strcmp(key, "code") == 0) {
                    return box_number((double)err->code);
                }
                if (strcmp(key, "errorType") == 0) {
                    return box_string(err->error_type);
                }
                if (strcmp(key, "type") == 0) {
                    return box_string("Error");
                }
                break;
            }
        }
    }
    
    // 遍历对象的所有字段（普通对象或扩展类型未匹配虚拟属性）
    ObjectEntry *entries = (ObjectEntry*)obj->data.pointer;
    
    // 检查是否是哈希模式
    if (object_is_hash_mode(obj)) {
        // 哈希模式：使用哈希查找
        unsigned long hash = hash_string(key);
        size_t capacity = obj->string_length;
        size_t idx = hash % capacity;
        size_t start = idx;
        
        do {
            if (entries[idx].key == NULL) {
                // 空槽，键不存在
                break;
            }
            if (entries[idx].key != OBJECT_TOMBSTONE && strcmp(entries[idx].key, key) == 0) {
                return entries[idx].value;
            }
            idx = (idx + 1) % capacity;
        } while (idx != start);
    } else {
        // 线性模式：遍历所有字段
        for (size_t i = 0; i < obj->array_size; i++) {
            if (strcmp(entries[i].key, key) == 0) {
                return entries[i].value;
            }
        }
    }
    
    // 字段不存在 - 只设置错误状态，由调用方决定是否终止
    set_runtime_status(FLYUX_TYPE_ERROR, "(getField) Field not found");
    return box_null();
}

/*
 * value_get_field_safe - 从对象中安全获取指定字段的值（可选链访问）
 * 
 * 参数：
 *   obj: 对象Value（可以是任何类型，非对象返回undef）
 *   field_name: 字段名（Value字符串）
 * 
 * 返回：
 *   字段的Value，如果找不到则返回undef（不设置错误状态）
 * 
 * 说明：
 *   用于实现 ?. 可选链访问，与 value_get_field 不同的是：
 *   - 字段不存在时返回 undef 而不是 null
 *   - 不设置错误状态
 *   - 对非对象类型返回 undef 而不是报错
 * 
 * 示例：
 *   obj?.prop  ->  如果obj是对象且prop存在，返回prop值，否则返回undef
 */
Value* value_get_field_safe(Value *obj, Value *field_name) {
    if (!obj || !field_name) {
        return box_undef();
    }
    
    // 如果obj不是对象类型，返回undef
    if (obj->type != VALUE_OBJECT) {
        return box_undef();
    }
    
    // 如果field_name不是字符串，返回undef
    if (field_name->type != VALUE_STRING) {
        return box_undef();
    }
    
    // 获取字段名
    const char *key = (const char*)field_name->data.pointer;
    
    // 如果是扩展类型对象，先检查虚拟属性
    if (obj->ext_type != EXT_TYPE_NONE) {
        switch (obj->ext_type) {
            case EXT_TYPE_BUFFER: {
                BufferObject *buf = (BufferObject*)obj->data.pointer;
                if (strcmp(key, "size") == 0) {
                    return box_number((double)buf->size);
                }
                if (strcmp(key, "capacity") == 0) {
                    return box_number((double)buf->capacity);
                }
                if (strcmp(key, "type") == 0) {
                    return box_string("Buffer");
                }
                break;
            }
            case EXT_TYPE_FILE: {
                FileHandleObject *file = (FileHandleObject*)obj->data.pointer;
                if (strcmp(key, "path") == 0) {
                    return box_string(file->path);
                }
                if (strcmp(key, "mode") == 0) {
                    return box_string(file->mode);
                }
                if (strcmp(key, "isOpen") == 0) {
                    return box_bool(file->is_open);
                }
                if (strcmp(key, "position") == 0) {
                    return box_number((double)file->position);
                }
                if (strcmp(key, "type") == 0) {
                    return box_string("FileHandle");
                }
                break;
            }
            case EXT_TYPE_ERROR: {
                ErrorObject *err = (ErrorObject*)obj->data.pointer;
                if (strcmp(key, "message") == 0) {
                    return box_string(err->message);
                }
                if (strcmp(key, "code") == 0) {
                    return box_number((double)err->code);
                }
                if (strcmp(key, "errorType") == 0) {
                    return box_string(err->error_type);
                }
                if (strcmp(key, "type") == 0) {
                    return box_string("Error");
                }
                break;
            }
        }
    }
    
    // 遍历对象的所有字段
    ObjectEntry *entries = (ObjectEntry*)obj->data.pointer;
    
    // 检查是否是哈希模式
    if (object_is_hash_mode(obj)) {
        // 哈希模式：使用哈希查找
        unsigned long hash = hash_string(key);
        size_t capacity = obj->string_length;
        size_t idx = hash % capacity;
        size_t start = idx;
        
        do {
            if (entries[idx].key == NULL) {
                // 空槽，键不存在
                break;
            }
            if (entries[idx].key != OBJECT_TOMBSTONE && strcmp(entries[idx].key, key) == 0) {
                return entries[idx].value;
            }
            idx = (idx + 1) % capacity;
        } while (idx != start);
    } else {
        // 线性模式：遍历所有字段
        for (size_t i = 0; i < obj->array_size; i++) {
            if (strcmp(entries[i].key, key) == 0) {
                return entries[i].value;
            }
        }
    }
    
    // 字段不存在，返回undef（不设置错误状态）
    return box_undef();
}

/*
 * value_set_field - 动态设置对象字段的值
 * 
 * 参数：
 *   obj: 对象Value（必须是VALUE_OBJECT类型）
 *   field_name: 字段名（Value字符串）
 *   value: 要设置的值
 * 
 * 返回：
 *   设置成功返回true，失败返回false
 * 
 * 行为：
 *   - 如果字段已存在，更新其值
 *   - 如果字段不存在，动态添加新字段
 *   - 当字段数超过阈值时，自动转换为哈希模式
 * 
 * 示例：
 *   value_set_field(obj, box_string("name"), box_string("Alice"))
 */
Value* value_set_field(Value *obj, Value *field_name, Value *value) {
    if (!obj || !field_name) {
        // 返回value而不是obj，避免与obj指针冲突导致双重释放
        return value ? value_retain(value) : box_undef();
    }
    
    // 检查obj是否为对象类型
    if (obj->type != VALUE_OBJECT) {
        return value ? value_retain(value) : box_undef();
    }
    
    // 检查field_name是否为字符串
    if (field_name->type != VALUE_STRING) {
        return value ? value_retain(value) : box_undef();
    }
    
    // 特殊处理：如果 value 是 undef，则删除该键（类似 JS 的 delete）
    if (!value || value->type == VALUE_UNDEF) {
        value_delete_field(obj, field_name);
        return box_undef();
    }
    
    const char *key = (const char*)field_name->data.pointer;
    ObjectEntry *entries = (ObjectEntry*)obj->data.pointer;
    size_t count = obj->array_size;
    
    // 检查是否是哈希模式
    if (object_is_hash_mode(obj)) {
        // === 哈希模式 ===
        size_t capacity = obj->string_length;
        unsigned long hash = hash_string(key);
        
        // 查找槽位
        long slot = object_hash_find_slot(entries, capacity, key, hash);
        if (slot < 0) {
            // 表满，需要扩容
            object_hash_resize(obj, capacity * 2);
            entries = (ObjectEntry*)obj->data.pointer;
            capacity = obj->string_length;
            slot = object_hash_find_slot(entries, capacity, key, hash);
        }
        
        if (entries[slot].key && entries[slot].key != OBJECT_TOMBSTONE) {
            // 键已存在，更新值
            if (entries[slot].value) value_release(entries[slot].value);
            entries[slot].value = value;
            if (value) value_retain(value);
            return value_retain(value);
        }
        
        // 新键，检查是否需要扩容（负载因子 > 75%）
        if ((count + 1) * 4 > capacity * 3) {
            object_hash_resize(obj, capacity * 2);
            entries = (ObjectEntry*)obj->data.pointer;
            capacity = obj->string_length;
            slot = object_hash_find_slot(entries, capacity, key, hash);
        }
        
        // 插入新键值对
        entries[slot].key = strdup(key);
        entries[slot].value = value;
        if (value) value_retain(value);
        obj->array_size = count + 1;
        
        return value_retain(value);
    }
    
    // === 线性模式 ===
    
    // 查找是否已存在该字段
    for (size_t i = 0; i < count; i++) {
        if (strcmp(entries[i].key, key) == 0) {
            // 字段已存在，更新值
            if (entries[i].value) value_release(entries[i].value);
            entries[i].value = value;
            if (value) value_retain(value);
            return value_retain(value);
        }
    }
    
    // 字段不存在，需要添加新字段
    // 检查是否应该转换为哈希模式
    if (count >= OBJECT_HASH_THRESHOLD) {
        // 转换为哈希模式
        object_convert_to_hash(obj);
        // 递归调用以使用哈希模式插入
        return value_set_field(obj, field_name, value);
    }
    
    // 仍然使用线性模式，分配新数组
    ObjectEntry *new_entries = (ObjectEntry*)malloc(sizeof(ObjectEntry) * (count + 1));
    if (!new_entries) {
        return value ? value_retain(value) : box_undef();
    }
    
    // 复制旧entries
    for (size_t i = 0; i < count; i++) {
        new_entries[i].key = entries[i].key;
        new_entries[i].value = entries[i].value;
    }
    
    // 添加新字段
    new_entries[count].key = strdup(key);
    new_entries[count].value = value;
    if (value) value_retain(value);
    
    // 释放旧entries数组
    if (entries) {
        free(entries);
    }
    
    // 更新对象指针和大小
    obj->data.pointer = new_entries;
    obj->array_size = count + 1;
    
    return value_retain(value);
}

/*
 * value_delete_field - 动态删除对象字段
 * 
 * 参数：
 *   obj: 对象Value（必须是VALUE_OBJECT类型）
 *   field_name: 字段名（Value字符串）
 * 
 * 返回：
 *   删除成功返回true，字段不存在或失败返回false
 * 
 * 示例：
 *   value_delete_field(obj, box_string("name"))
 */
Value* value_delete_field(Value *obj, Value *field_name) {
    if (!obj || !field_name) {
        return box_bool(0);
    }
    
    // 检查obj是否为对象类型
    if (obj->type != VALUE_OBJECT) {
        return box_bool(0);
    }
    
    // 检查field_name是否为字符串
    if (field_name->type != VALUE_STRING) {
        return box_bool(0);
    }
    
    const char *key = (const char*)field_name->data.pointer;
    ObjectEntry *entries = (ObjectEntry*)obj->data.pointer;
    size_t count = obj->array_size;
    
    // 检查是否是哈希模式
    if (object_is_hash_mode(obj)) {
        // === 哈希模式：使用墓碑标记删除 ===
        size_t capacity = obj->string_length;
        unsigned long hash = hash_string(key);
        size_t idx = hash % capacity;
        size_t start = idx;
        
        do {
            if (entries[idx].key == NULL) {
                // 空槽，键不存在
                return box_bool(0);
            }
            if (entries[idx].key != OBJECT_TOMBSTONE && strcmp(entries[idx].key, key) == 0) {
                // 找到键，删除
                free(entries[idx].key);
                if (entries[idx].value) value_release(entries[idx].value);
                entries[idx].key = OBJECT_TOMBSTONE;
                entries[idx].value = NULL;
                obj->array_size = count - 1;
                return box_bool(1);
            }
            idx = (idx + 1) % capacity;
        } while (idx != start);
        
        return box_bool(0);
    }
    
    // === 线性模式 ===
    
    // 查找要删除的字段
    int found_index = -1;
    for (size_t i = 0; i < count; i++) {
        if (strcmp(entries[i].key, key) == 0) {
            found_index = (int)i;
            break;
        }
    }
    
    if (found_index < 0) {
        // 字段不存在
        return box_bool(0);
    }
    
    // 如果删除后为空对象
    if (count == 1) {
        // 释放旧数组和被删除字段的key
        if (entries[0].key) free(entries[0].key);
        if (entries[0].value) value_release(entries[0].value);
        free(entries);
        obj->data.pointer = NULL;
        obj->array_size = 0;
        return box_bool(1);
    }
    
    // 分配新数组
    ObjectEntry *new_entries = (ObjectEntry*)malloc(sizeof(ObjectEntry) * (count - 1));
    if (!new_entries) {
        return box_bool(0);
    }
    
    // 复制除了被删除字段外的所有字段
    size_t new_idx = 0;
    for (size_t i = 0; i < count; i++) {
        if ((int)i != found_index) {
            new_entries[new_idx].key = entries[i].key;
            new_entries[new_idx].value = entries[i].value;
            new_idx++;
        }
    }
    
    // 释放被删除字段的key和value
    if (entries[found_index].key) free(entries[found_index].key);
    if (entries[found_index].value) value_release(entries[found_index].value);
    
    // 释放旧数组
    free(entries);
    
    // 更新对象
    obj->data.pointer = new_entries;
    obj->array_size = count - 1;
    
    return box_bool(1);
}

/*
 * value_has_field - 检查对象是否有指定字段
 * 
 * 参数：
 *   obj: 对象Value
 *   field_name: 字段名（Value字符串）
 * 
 * 返回：
 *   存在返回true，不存在返回false
 */
Value* value_has_field(Value *obj, Value *field_name) {
    if (!obj || !field_name) {
        return box_bool(0);
    }
    
    if (obj->type != VALUE_OBJECT || field_name->type != VALUE_STRING) {
        return box_bool(0);
    }
    
    const char *key = (const char*)field_name->data.pointer;
    ObjectEntry *entries = (ObjectEntry*)obj->data.pointer;
    
    // 检查是否是哈希模式
    if (object_is_hash_mode(obj)) {
        size_t capacity = obj->string_length;
        unsigned long hash = hash_string(key);
        size_t idx = hash % capacity;
        size_t start = idx;
        
        do {
            if (entries[idx].key == NULL) {
                return box_bool(0);
            }
            if (entries[idx].key != OBJECT_TOMBSTONE && strcmp(entries[idx].key, key) == 0) {
                return box_bool(1);
            }
            idx = (idx + 1) % capacity;
        } while (idx != start);
        
        return box_bool(0);
    }
    
    // 线性模式
    for (size_t i = 0; i < obj->array_size; i++) {
        if (strcmp(entries[i].key, key) == 0) {
            return box_bool(1);
        }
    }
    
    return box_bool(0);
}

/*
 * value_keys - 获取对象的所有键名
 * 
 * 参数：
 *   obj: 对象Value
 * 
 * 返回：
 *   包含所有键名的数组Value
 */
Value* value_keys(Value *obj) {
    if (!obj || obj->type != VALUE_OBJECT) {
        return box_array(NULL, 0);  // 返回空数组
    }
    
    ObjectEntry *entries = (ObjectEntry*)obj->data.pointer;
    size_t count = obj->array_size;
    
    // 创建字符串数组
    Value **keys = (Value**)malloc(sizeof(Value*) * count);
    size_t key_idx = 0;
    
    if (object_is_hash_mode(obj)) {
        // 哈希模式：遍历整个表，跳过空槽和墓碑
        size_t capacity = obj->string_length;
        for (size_t i = 0; i < capacity && key_idx < count; i++) {
            if (entries[i].key && entries[i].key != OBJECT_TOMBSTONE) {
                keys[key_idx++] = box_string(entries[i].key);
            }
        }
    } else {
        // 线性模式
        for (size_t i = 0; i < count; i++) {
            keys[key_idx++] = box_string(entries[i].key);
        }
    }
    
    return box_array(keys, count);
}

/*
 * value_values - 获取对象的所有值
 * 
 * 参数：
 *   obj: 对象Value
 * 
 * 返回：
 *   包含所有值的数组Value
 */
Value* value_values(Value *obj) {
    set_runtime_status(FLYUX_OK, NULL);
    
    if (!obj || obj->type != VALUE_OBJECT) {
        set_runtime_status(FLYUX_TYPE_ERROR, "(values) requires object");
        return box_array(NULL, 0);  // 返回空数组
    }
    
    ObjectEntry *entries = (ObjectEntry*)obj->data.pointer;
    size_t count = obj->array_size;
    
    // 创建值数组
    Value **values = (Value**)malloc(sizeof(Value*) * count);
    size_t val_idx = 0;
    
    if (object_is_hash_mode(obj)) {
        // 哈希模式：遍历整个表，跳过空槽和墓碑
        size_t capacity = obj->string_length;
        for (size_t i = 0; i < capacity && val_idx < count; i++) {
            if (entries[i].key && entries[i].key != OBJECT_TOMBSTONE) {
                values[val_idx++] = entries[i].value;
            }
        }
    } else {
        // 线性模式
        for (size_t i = 0; i < count; i++) {
            values[val_idx++] = entries[i].value;
        }
    }
    
    return box_array(values, count);
}

/*
 * value_entries - 获取对象的所有键值对
 * 
 * 参数：
 *   obj: 对象Value
 * 
 * 返回：
 *   包含 [key, value] 数组的数组（二维数组）
 */
Value* value_entries(Value *obj) {
    set_runtime_status(FLYUX_OK, NULL);
    
    if (!obj || obj->type != VALUE_OBJECT) {
        set_runtime_status(FLYUX_TYPE_ERROR, "(entries) requires object");
        return box_array(NULL, 0);  // 返回空数组
    }
    
    ObjectEntry *obj_entries = (ObjectEntry*)obj->data.pointer;
    size_t count = obj->array_size;
    
    // 创建二维数组
    Value **entries_arr = (Value**)malloc(sizeof(Value*) * count);
    size_t entry_idx = 0;
    
    if (object_is_hash_mode(obj)) {
        // 哈希模式：遍历整个表，跳过空槽和墓碑
        size_t capacity = obj->string_length;
        for (size_t i = 0; i < capacity && entry_idx < count; i++) {
            if (obj_entries[i].key && obj_entries[i].key != OBJECT_TOMBSTONE) {
                Value **pair = (Value**)malloc(sizeof(Value*) * 2);
                pair[0] = box_string(obj_entries[i].key);
                pair[1] = obj_entries[i].value;
                entries_arr[entry_idx++] = box_array(pair, 2);
            }
        }
    } else {
        // 线性模式
        for (size_t i = 0; i < count; i++) {
            Value **pair = (Value**)malloc(sizeof(Value*) * 2);
            pair[0] = box_string(obj_entries[i].key);
            pair[1] = obj_entries[i].value;
            entries_arr[entry_idx++] = box_array(pair, 2);
        }
    }
    
    return box_array(entries_arr, count);
}

/*
 * value_set_index - 设置数组或对象的索引值
 * 
 * 参数：
 *   obj: 数组或对象 Value
 *   index: 索引（数字或字符串）
 *   value: 要设置的值
 * 
 * 返回：
 *   设置成功返回 true，失败返回 false
 * 
 * 行为：
 *   - 如果是数组且 index 是数字，设置数组元素
 *   - 如果是对象且 index 是字符串，设置对象字段
 *   - 如果 value 是 undef，对于对象会删除键
 */
Value* value_set_index(Value *obj, Value *index, Value *value) {
    if (!obj || !index) {
        return value ? value_retain(value) : box_undef();
    }
    
    // 如果是数组
    if (obj->type == VALUE_ARRAY && index->type == VALUE_NUMBER) {
        double idx_double = index->data.number;
        if (idx_double < 0) {
            return value ? value_retain(value) : box_undef();  // 负索引不允许
        }
        
        size_t idx = (size_t)idx_double;
        size_t count = obj->array_size;
        Value **elements = (Value**)obj->data.pointer;
        
        // 如果索引超出当前数组大小，需要扩展数组
        if (idx >= count) {
            size_t new_size = idx + 1;
            Value **new_elements = (Value**)malloc(sizeof(Value*) * new_size);
            if (!new_elements) {
                return value ? value_retain(value) : box_undef();  // 内存分配失败
            }
            
            // 复制旧元素
            if (elements) {
                for (size_t i = 0; i < count; i++) {
                    new_elements[i] = elements[i];
                }
                free(elements);
            }
            
            // 新位置填充 undef
            for (size_t i = count; i < new_size; i++) {
                new_elements[i] = box_undef();
            }
            
            // 更新数组
            obj->data.pointer = new_elements;
            obj->array_size = new_size;
            elements = new_elements;
            count = new_size;
        }
        
        // 数组元素赋值：释放旧值，retain新值
        if (elements[idx]) {
            value_release(elements[idx]);
        }
        if (value) {
            value_retain(value);
            elements[idx] = value;
        } else {
            elements[idx] = box_undef();
        }
        // 返回value的引用（与value_set_field保持一致）
        return value ? value_retain(value) : box_undef();
    }
    
    // 如果是对象且索引是字符串
    if (obj->type == VALUE_OBJECT && index->type == VALUE_STRING) {
        // value_set_field 会处理 undef 删除逻辑
        return value_set_field(obj, index, value);
    }
    
    // 类型不匹配，返回value（保持一致的返回语义）
    return value ? value_retain(value) : box_undef();
}

/* ============================================================================
 * 更多类型转换函数
 * ============================================================================
 */

/*
 * toInt(value) - 转换为整数
 * 截断小数部分，与toNum的区别是强制转为整数
 */
Value* value_to_int(Value *v) {
    Value *num = value_to_num(v);
    if (num->type == VALUE_NUMBER) {
        num->data.number = floor(num->data.number);
    }
    return num;
}

/*
 * toFloat(value) - 转换为浮点数（与toNum相同，保留语义清晰）
 */
Value* value_to_float(Value *v) {
    return value_to_num(v);
}

/* ============================================================================
 * 方法访问支持 - self 绑定
 * ============================================================================
 */

/* 外部声明 bind_method 函数（定义在 value_runtime_ext.c 中） */
extern Value* bind_method(Value *func_val, Value *self_obj);

/*
 * value_get_method - 从对象中获取方法（自动绑定 self）
 * 
 * 当访问对象的函数类型字段时，自动将对象绑定为方法的 self。
 * 这使得 obj.method 返回一个绑定了 self 的函数。
 * 
 * 参数：
 *   obj: 对象Value（必须是VALUE_OBJECT类型）
 *   field_name: 字段名（Value字符串）
 * 
 * 返回：
 *   如果字段是函数，返回绑定了 self 的方法
 *   如果字段不是函数，返回字段值本身
 *   如果找不到字段，返回 null
 */
Value* value_get_method(Value *obj, Value *field_name) {
    // 先使用普通的 value_get_field 获取字段值
    Value *field_value = value_get_field(obj, field_name);
    
    if (!field_value) {
        return box_null();
    }
    
    // 如果字段值是函数，绑定 self
    if (field_value->type == VALUE_FUNCTION) {
        return bind_method(field_value, obj);
    }
    
    // 否则直接返回字段值
    return field_value;
}

/*
 * value_get_method_by_index - 通过索引获取方法（自动绑定 self）
 * 
 * 类似于 value_get_method，但使用索引访问（支持数组和对象）。
 * obj[index] 如果获取的是函数，则自动绑定 self。
 * 
 * 参数：
 *   obj: 对象或数组Value
 *   index: 索引（可以是数字或字符串）
 * 
 * 返回：
 *   如果索引处的值是函数，返回绑定了 self 的方法
 *   如果不是函数，返回值本身
 */
Value* value_get_method_by_index(Value *obj, Value *index) {
    // 使用 value_index 获取索引处的值
    Value *indexed_value = value_index(obj, index);
    
    if (!indexed_value) {
        return box_null();
    }
    
    // 如果索引处的值是函数，绑定 self
    if (indexed_value->type == VALUE_FUNCTION) {
        return bind_method(indexed_value, obj);
    }
    
    // 否则直接返回值
    return indexed_value;
}


