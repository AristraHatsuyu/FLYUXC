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
 * 注意：创建新数组，不修改原数组
 */
Value* value_push(Value *arr, Value *val) {
    set_runtime_status(FLYUX_OK, NULL);
    
    if (!arr || arr->type != VALUE_ARRAY) {
        set_runtime_status(FLYUX_TYPE_ERROR, "(push) requires array");
        return box_null_typed(VALUE_OBJECT);
    }
    
    size_t new_size = arr->array_size + 1;
    Value **new_elements = (Value**)malloc(new_size * sizeof(Value*));
    
    // 复制旧元素
    Value **old_elements = (Value**)arr->data.pointer;
    for (size_t i = 0; i < arr->array_size; i++) {
        new_elements[i] = old_elements[i];
    }
    new_elements[arr->array_size] = val;
    
    // 创建新数组
    Value *result = (Value*)malloc(sizeof(Value));
    result->type = VALUE_ARRAY;
    result->declared_type = VALUE_ARRAY;
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
    }
    
    Value *result = (Value*)malloc(sizeof(Value));
    result->type = VALUE_ARRAY;
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
    }
    
    Value *result = (Value*)malloc(sizeof(Value));
    result->type = VALUE_ARRAY;
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
    
    Value **old_elements = (Value**)arr->data.pointer;
    for (size_t i = 0; i < arr->array_size; i++) {
        new_elements[i+1] = old_elements[i];
    }
    
    // 创建新数组
    Value *result = (Value*)malloc(sizeof(Value));
    result->type = VALUE_ARRAY;
    result->declared_type = VALUE_ARRAY;
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
        empty->data.pointer = NULL;
        empty->array_size = 0;
        return empty;
    }
    
    size_t new_size = end_idx - start_idx;
    Value **new_elements = (Value**)malloc(new_size * sizeof(Value*));
    Value **old_elements = (Value**)arr->data.pointer;
    
    for (size_t i = 0; i < new_size; i++) {
        new_elements[i] = old_elements[start_idx + i];
    }
    
    Value *result = (Value*)malloc(sizeof(Value));
    result->type = VALUE_ARRAY;
    result->declared_type = VALUE_ARRAY;
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
    }
    for (size_t i = 0; i < arr2->array_size; i++) {
        new_elements[arr1->array_size + i] = elem2[i];
    }
    
    Value *result = (Value*)malloc(sizeof(Value));
    result->type = VALUE_ARRAY;
    result->declared_type = VALUE_ARRAY;
    result->ext_type = EXT_TYPE_NONE;
    result->data.pointer = new_elements;
    result->array_size = new_size;
    
    return result;
}

