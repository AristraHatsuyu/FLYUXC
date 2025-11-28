/*
 * Auto-generated fragment from value_runtime.c
 * Module: value_runtime_state_check.c
 */

/* ============================================================================
 * 内部状态检查函数（仅供try-catch使用）
 * ============================================================================
 */

// 内部函数：检查当前状态是否OK（供try-catch使用）
Value* value_is_ok() {
    return box_bool(g_runtime_state.last_status == FLYUX_OK);
}

// 内部函数：获取错误消息（供try-catch使用）
Value* value_last_error() {
    size_t len = strlen(g_runtime_state.error_msg);
    if (len == 0) {
        return box_string("");
    }
    
    char *msg = (char*)malloc(len + 1);
    strcpy(msg, g_runtime_state.error_msg);
    
    Value *result = (Value*)malloc(sizeof(Value));
    result->type = VALUE_STRING;
    result->declared_type = VALUE_STRING;
    result->data.string = msg;
    result->string_length = len;
    
    return result;
}

// 内部函数：获取错误状态码（供try-catch使用）
Value* value_last_status() {
    return box_number((double)g_runtime_state.last_status);
}

// 内部函数：清除错误状态（供try-catch使用）
Value* value_clear_error() {
    flyux_clear_error();
    return box_null(); // ignore
}

