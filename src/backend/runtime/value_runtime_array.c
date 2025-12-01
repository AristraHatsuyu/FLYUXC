/*
 * Auto-generated fragment from value_runtime.c
 * Module: value_runtime_array.c
 */

/* ============================================================================
 * 数组操作函数
 * ============================================================================
 */

/*
 * push(array, value) - 在数组末尾添加元素
 * 返回新数组，不修改原数组
 */
Value* value_push(Value *arr, Value *val) {
    set_runtime_status(FLYUX_OK, NULL);
    
    if (!arr || arr->type != VALUE_ARRAY) {
        set_runtime_status(FLYUX_TYPE_ERROR, "(push) requires array");
        return box_null_typed(VALUE_ARRAY);
    }
    
    size_t old_size = arr->array_size;
    size_t new_size = old_size + 1;
    Value **old_elements = (Value**)arr->data.pointer;
    
    // 创建新数组（不能用 realloc，因为原数组可能不是 malloc 分配的）
    Value **new_elements = (Value**)malloc(new_size * sizeof(Value*));
    if (!new_elements) {
        set_runtime_status(FLYUX_ERROR, "(push) memory allocation failed");
        return box_null_typed(VALUE_ARRAY);
    }
    
    // 复制旧元素（需要 retain，因为新数组也持有引用）
    for (size_t i = 0; i < old_size; i++) {
        new_elements[i] = old_elements[i];
        if (new_elements[i]) {
            value_retain(new_elements[i]);
        }
    }
    
    // 添加新元素（也需要 retain）
    new_elements[old_size] = val;
    if (val) {
        value_retain(val);
    }
    
    // 创建新的 Value 对象
    Value *result = (Value*)malloc(sizeof(Value));
    result->type = VALUE_ARRAY;
    result->declared_type = VALUE_ARRAY;
    result->ext_type = EXT_TYPE_NONE;
    result->refcount = 1;
    result->flags = VALUE_FLAG_NONE;
    result->data.pointer = new_elements;
    result->array_size = new_size;
    
    return result;
}

/*
 * pop(array) - 移除并返回数组最后一个元素
 * 返回新数组（不包含最后一个元素）
 */
Value* value_pop(Value *arr) {
    set_runtime_status(FLYUX_OK, NULL);
    
    if (!arr || arr->type != VALUE_ARRAY) {
        set_runtime_status(FLYUX_TYPE_ERROR, "(pop) requires array");
        return box_null_typed(VALUE_OBJECT);
    }
    
    if (arr->array_size == 0) {
        set_runtime_status(FLYUX_OUT_OF_BOUNDS, "(pop) cannot pop from empty array");
        return box_null_typed(VALUE_OBJECT);
    }
    
    Value **old_elements = (Value**)arr->data.pointer;
    size_t new_size = arr->array_size - 1;
    
    // 创建新数组，不包含最后一个元素
    Value **new_elements = (Value**)malloc(new_size * sizeof(Value*));
    for (size_t i = 0; i < new_size; i++) {
        new_elements[i] = old_elements[i];
        if (new_elements[i]) {
            value_retain(new_elements[i]);
        }
    }
    
    Value *result = (Value*)malloc(sizeof(Value));
    result->type = VALUE_ARRAY;
    result->declared_type = VALUE_ARRAY;
    result->refcount = 1;
    result->flags = VALUE_FLAG_NONE;
    result->ext_type = EXT_TYPE_NONE;
    result->data.pointer = new_elements;
    result->array_size = new_size;
    
    return result;
}

/*
 * shift(array) - 移除并返回数组第一个元素
 * 返回新数组（不包含第一个元素）
 */
Value* value_shift(Value *arr) {
    set_runtime_status(FLYUX_OK, NULL);
    
    if (!arr || arr->type != VALUE_ARRAY) {
        set_runtime_status(FLYUX_TYPE_ERROR, "(shift) requires array");
        return box_null_typed(VALUE_OBJECT);
    }
    
    if (arr->array_size == 0) {
        set_runtime_status(FLYUX_OUT_OF_BOUNDS, "(shift) cannot shift from empty array");
        return box_null_typed(VALUE_OBJECT);
    }
    
    Value **old_elements = (Value**)arr->data.pointer;
    size_t new_size = arr->array_size - 1;
    
    // 创建新数组，不包含第一个元素
    Value **new_elements = (Value**)malloc(new_size * sizeof(Value*));
    for (size_t i = 0; i < new_size; i++) {
        new_elements[i] = old_elements[i + 1];
        if (new_elements[i]) {
            value_retain(new_elements[i]);
        }
    }
    
    Value *result = (Value*)malloc(sizeof(Value));
    result->type = VALUE_ARRAY;
    result->declared_type = VALUE_ARRAY;
    result->refcount = 1;
    result->flags = VALUE_FLAG_NONE;
    result->ext_type = EXT_TYPE_NONE;
    result->data.pointer = new_elements;
    result->array_size = new_size;
    
    return result;
}

/*
 * unshift(array, value) - 在数组开头添加元素
 * 注意：创建新数组
 */
Value* value_unshift(Value *arr, Value *val) {
    set_runtime_status(FLYUX_OK, NULL);
    
    if (!arr || arr->type != VALUE_ARRAY) {
        set_runtime_status(FLYUX_TYPE_ERROR, "(unshift) requires array");
        return box_null_typed(VALUE_OBJECT);
    }
    
    size_t new_size = arr->array_size + 1;
    Value **new_elements = (Value**)malloc(new_size * sizeof(Value*));
    new_elements[0] = val;
    if (val) {
        value_retain(val);
    }
    
    Value **old_elements = (Value**)arr->data.pointer;
    for (size_t i = 0; i < arr->array_size; i++) {
        new_elements[i+1] = old_elements[i];
        if (new_elements[i+1]) {
            value_retain(new_elements[i+1]);
        }
    }
    
    // 创建新数组
    Value *result = (Value*)malloc(sizeof(Value));
    result->type = VALUE_ARRAY;
    result->declared_type = VALUE_ARRAY;
    result->refcount = 1;
    result->flags = VALUE_FLAG_NONE;
    result->ext_type = EXT_TYPE_NONE;
    result->data.pointer = new_elements;
    result->array_size = new_size;
    
    return result;
}

/*
 * slice(array, start, end) - 获取数组片段
 */
Value* value_slice(Value *arr, Value *start, Value *end) {
    set_runtime_status(FLYUX_OK, NULL);
    
    if (!arr || arr->type != VALUE_ARRAY) {
        set_runtime_status(FLYUX_TYPE_ERROR, "(slice) requires array");
        return box_null_typed(VALUE_OBJECT);
    }
    
    int start_idx = (start && start->type == VALUE_NUMBER) ? (int)start->data.number : 0;
    int end_idx = (end && end->type == VALUE_NUMBER) ? (int)end->data.number : arr->array_size;
    
    if (start_idx < 0) start_idx = 0;
    if (end_idx > (int)arr->array_size) end_idx = arr->array_size;
    if (start_idx >= end_idx) {
        Value *empty = (Value*)malloc(sizeof(Value));
        empty->type = VALUE_ARRAY;
        empty->declared_type = VALUE_ARRAY;
        empty->refcount = 1;
        empty->flags = VALUE_FLAG_NONE;
        empty->ext_type = EXT_TYPE_NONE;
        empty->data.pointer = NULL;
        empty->array_size = 0;
        return empty;
    }
    
    size_t new_size = end_idx - start_idx;
    Value **new_elements = (Value**)malloc(new_size * sizeof(Value*));
    Value **old_elements = (Value**)arr->data.pointer;
    
    for (size_t i = 0; i < new_size; i++) {
        new_elements[i] = old_elements[start_idx + i];
        if (new_elements[i]) {
            value_retain(new_elements[i]);
        }
    }
    
    Value *result = (Value*)malloc(sizeof(Value));
    result->type = VALUE_ARRAY;
    result->declared_type = VALUE_ARRAY;
    result->refcount = 1;
    result->flags = VALUE_FLAG_NONE;
    result->ext_type = EXT_TYPE_NONE;
    result->data.pointer = new_elements;
    result->array_size = new_size;
    
    return result;
}

/*
 * concat(array1, array2) - 连接两个数组
 */
Value* value_concat(Value *arr1, Value *arr2) {
    set_runtime_status(FLYUX_OK, NULL);
    
    if (!arr1 || arr1->type != VALUE_ARRAY || !arr2 || arr2->type != VALUE_ARRAY) {
        set_runtime_status(FLYUX_TYPE_ERROR, "(concat) requires two arrays");
        return box_null_typed(VALUE_OBJECT);
    }
    
    size_t new_size = arr1->array_size + arr2->array_size;
    Value **new_elements = (Value**)malloc(new_size * sizeof(Value*));
    
    Value **elem1 = (Value**)arr1->data.pointer;
    Value **elem2 = (Value**)arr2->data.pointer;
    
    for (size_t i = 0; i < arr1->array_size; i++) {
        new_elements[i] = elem1[i];
        if (new_elements[i]) {
            value_retain(new_elements[i]);
        }
    }
    for (size_t i = 0; i < arr2->array_size; i++) {
        new_elements[arr1->array_size + i] = elem2[i];
        if (new_elements[arr1->array_size + i]) {
            value_retain(new_elements[arr1->array_size + i]);
        }
    }
    
    Value *result = (Value*)malloc(sizeof(Value));
    result->type = VALUE_ARRAY;
    result->declared_type = VALUE_ARRAY;
    result->refcount = 1;
    result->flags = VALUE_FLAG_NONE;
    result->ext_type = EXT_TYPE_NONE;
    result->data.pointer = new_elements;
    result->array_size = new_size;
    
    return result;
}

/*
 * reverse(array|string) - 反转数组或字符串
 * 返回新数组/字符串，不修改原值
 */
Value* value_reverse(Value *val) {
    set_runtime_status(FLYUX_OK, NULL);
    
    if (!val) {
        set_runtime_status(FLYUX_TYPE_ERROR, "(reverse) requires array or string");
        return box_null();
    }
    
    // 处理字符串反转
    if (val->type == VALUE_STRING) {
        const char *str = (const char*)val->data.pointer;
        if (!str) {
            return box_string("");
        }
        
        size_t len = strlen(str);
        char *reversed = (char*)malloc(len + 1);
        
        for (size_t i = 0; i < len; i++) {
            reversed[i] = str[len - 1 - i];
        }
        reversed[len] = '\0';
        
        return box_string_owned(reversed);
    }
    
    // 处理数组反转
    if (val->type != VALUE_ARRAY) {
        set_runtime_status(FLYUX_TYPE_ERROR, "(reverse) requires array or string");
        return box_null_typed(VALUE_ARRAY);
    }
    
    size_t size = val->array_size;
    Value **new_elements = (Value**)malloc(size * sizeof(Value*));
    Value **old_elements = (Value**)val->data.pointer;
    
    for (size_t i = 0; i < size; i++) {
        new_elements[i] = value_retain(old_elements[size - 1 - i]);  // 增加引用计数
    }
    
    Value *result = (Value*)malloc(sizeof(Value));
    result->type = VALUE_ARRAY;
    result->declared_type = VALUE_ARRAY;
    result->refcount = 1;
    result->flags = VALUE_FLAG_NONE;
    result->ext_type = EXT_TYPE_NONE;
    result->data.pointer = new_elements;
    result->array_size = size;
    result->string_length = 0;
    
    return result;
}

/*
 * indexOf(array, value) - 查找元素在数组中的索引
 * 返回索引（从0开始），未找到返回 -1
 */
Value* value_index_of_array(Value *arr, Value *val) {
    set_runtime_status(FLYUX_OK, NULL);
    
    if (!arr || arr->type != VALUE_ARRAY) {
        set_runtime_status(FLYUX_TYPE_ERROR, "(indexOf) requires array");
        return box_number(-1);
    }
    
    Value **elements = (Value**)arr->data.pointer;
    
    for (size_t i = 0; i < arr->array_size; i++) {
        Value *eq = value_equals(elements[i], val);
        if (eq && eq->type == VALUE_BOOL && eq->data.number != 0) {
            return box_number((double)i);
        }
    }
    
    return box_number(-1);
}

/*
 * includes(array, value) - 检查数组是否包含某元素
 * 返回 true/false
 */
Value* value_includes(Value *arr, Value *val) {
    set_runtime_status(FLYUX_OK, NULL);
    
    if (!arr || arr->type != VALUE_ARRAY) {
        set_runtime_status(FLYUX_TYPE_ERROR, "(includes) requires array");
        return box_bool(0);
    }
    
    Value **elements = (Value**)arr->data.pointer;
    
    for (size_t i = 0; i < arr->array_size; i++) {
        Value *eq = value_equals(elements[i], val);
        if (eq && eq->type == VALUE_BOOL && eq->data.number != 0) {
            return box_bool(1);
        }
    }
    
    return box_bool(0);
}

/*
 * value_create_array(size) - 创建指定大小的数组
 * 元素初始化为 null
 */
Value* value_create_array(int64_t size) {
    set_runtime_status(FLYUX_OK, NULL);
    
    Value **elements = NULL;
    if (size > 0) {
        elements = (Value**)malloc((size_t)size * sizeof(Value*));
        for (int64_t i = 0; i < size; i++) {
            elements[i] = box_null();
        }
    }
    
    Value *result = (Value*)malloc(sizeof(Value));
    result->type = VALUE_ARRAY;
    result->declared_type = VALUE_ARRAY;
    result->refcount = 1;
    result->flags = VALUE_FLAG_NONE;
    result->ext_type = EXT_TYPE_NONE;
    result->data.pointer = elements;
    result->array_size = (size_t)size;
    
    return result;
}

/*
 * 比较函数类型，用于 sort
 */
typedef Value* (*CompareFunc)(Value*, Value*);

/*
 * 默认比较函数：数字比较或字符串比较
 */
static int default_compare(const void *a, const void *b) {
    Value *va = *(Value**)a;
    Value *vb = *(Value**)b;
    
    // 数字比较
    if (va->type == VALUE_NUMBER && vb->type == VALUE_NUMBER) {
        double diff = va->data.number - vb->data.number;
        if (diff < 0) return -1;
        if (diff > 0) return 1;
        return 0;
    }
    
    // 字符串比较
    if (va->type == VALUE_STRING && vb->type == VALUE_STRING) {
        return strcmp(va->data.string, vb->data.string);
    }
    
    // 类型不同，按类型排序
    return (int)va->type - (int)vb->type;
}

/* 全局比较函数指针，用于自定义排序 */
static CompareFunc g_compare_func = NULL;

static int custom_compare(const void *a, const void *b) {
    Value *va = *(Value**)a;
    Value *vb = *(Value**)b;
    
    Value *result = g_compare_func(va, vb);
    if (result && result->type == VALUE_NUMBER) {
        double r = result->data.number;
        if (r < 0) return -1;
        if (r > 0) return 1;
        return 0;
    }
    return 0;
}

/*
 * sort(array, compare?) - 排序数组
 * compare 是可选的比较函数，返回负数/0/正数
 */
Value* value_sort(Value *arr, CompareFunc compare) {
    set_runtime_status(FLYUX_OK, NULL);
    
    if (!arr || arr->type != VALUE_ARRAY) {
        set_runtime_status(FLYUX_TYPE_ERROR, "(sort) requires array");
        return box_null_typed(VALUE_ARRAY);
    }
    
    size_t size = arr->array_size;
    if (size <= 1) {
        // 返回数组副本
        return value_reverse(value_reverse(arr));  // 简单方式创建副本
    }
    
    // 创建新数组副本
    Value **new_elements = (Value**)malloc(size * sizeof(Value*));
    Value **old_elements = (Value**)arr->data.pointer;
    for (size_t i = 0; i < size; i++) {
        new_elements[i] = old_elements[i];
        if (new_elements[i]) {
            value_retain(new_elements[i]);
        }
    }
    
    // 排序
    if (compare) {
        g_compare_func = compare;
        qsort(new_elements, size, sizeof(Value*), custom_compare);
        g_compare_func = NULL;
    } else {
        qsort(new_elements, size, sizeof(Value*), default_compare);
    }
    
    Value *result = (Value*)malloc(sizeof(Value));
    result->type = VALUE_ARRAY;
    result->declared_type = VALUE_ARRAY;
    result->refcount = 1;
    result->flags = VALUE_FLAG_NONE;
    result->ext_type = EXT_TYPE_NONE;
    result->data.pointer = new_elements;
    result->array_size = size;
    
    return result;
}


