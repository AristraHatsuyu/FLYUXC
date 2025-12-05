/*
 * Auto-generated fragment from value_runtime.c
 * Module: value_runtime_math.c
 */

/* ============================================
 * 数学函数 (Math Functions)
 * ============================================ */

/* abs(num) -> num - 绝对值 */
Value* value_abs(Value* num) {
    if (!num || num->type != VALUE_NUMBER) {
        set_runtime_status(FLYUX_TYPE_ERROR, "(abs) argument must be a number");
        return box_null_typed(VALUE_NUMBER);
    }
    
    set_runtime_status(FLYUX_OK, NULL);
    return box_number(fabs(num->data.number));
}

/* floor(num) -> num - 向下取整 */
Value* value_floor(Value* num) {
    if (!num || num->type != VALUE_NUMBER) {
        set_runtime_status(FLYUX_TYPE_ERROR, "(floor) argument must be a number");
        return box_null_typed(VALUE_NUMBER);
    }
    
    set_runtime_status(FLYUX_OK, NULL);
    return box_number(floor(num->data.number));
}

/* ceil(num) -> num - 向上取整 */
Value* value_ceil(Value* num) {
    if (!num || num->type != VALUE_NUMBER) {
        set_runtime_status(FLYUX_TYPE_ERROR, "(ceil) argument must be a number");
        return box_null_typed(VALUE_NUMBER);
    }
    
    set_runtime_status(FLYUX_OK, NULL);
    return box_number(ceil(num->data.number));
}

/* round(num, digits?) -> num - 四舍五入，可选指定小数位数 */
Value* value_round(Value* num) {
    if (!num || num->type != VALUE_NUMBER) {
        set_runtime_status(FLYUX_TYPE_ERROR, "(round) argument must be a number");
        return box_null_typed(VALUE_NUMBER);
    }
    
    set_runtime_status(FLYUX_OK, NULL);
    return box_number(round(num->data.number));
}

/* round(num, digits) -> num - 四舍五入到指定小数位数 */
Value* value_round2(Value* num, Value* digits) {
    if (!num || num->type != VALUE_NUMBER) {
        set_runtime_status(FLYUX_TYPE_ERROR, "(round) first argument must be a number");
        return box_null_typed(VALUE_NUMBER);
    }
    if (!digits || digits->type != VALUE_NUMBER) {
        set_runtime_status(FLYUX_TYPE_ERROR, "(round) second argument must be a number");
        return box_null_typed(VALUE_NUMBER);
    }
    
    set_runtime_status(FLYUX_OK, NULL);
    int d = (int)digits->data.number;
    double multiplier = pow(10.0, d);
    return box_number(round(num->data.number * multiplier) / multiplier);
}

/* sqrt(num) -> num - 平方根 */
Value* value_sqrt(Value* num) {
    if (!num || num->type != VALUE_NUMBER) {
        set_runtime_status(FLYUX_TYPE_ERROR, "(sqrt) argument must be a number");
        return box_null_typed(VALUE_NUMBER);
    }
    
    if (num->data.number < 0) {
        set_runtime_status(FLYUX_MATH_ERROR, "(sqrt) negative number");
        return box_null_typed(VALUE_NUMBER);
    }
    
    set_runtime_status(FLYUX_OK, NULL);
    return box_number(sqrt(num->data.number));
}

/* pow(base, exp) -> num - 幂运算 */
Value* value_pow(Value* base, Value* exp) {
    if (!base || base->type != VALUE_NUMBER) {
        set_runtime_status(FLYUX_TYPE_ERROR, "(pow) base must be a number");
        return box_null_typed(VALUE_NUMBER);
    }
    if (!exp || exp->type != VALUE_NUMBER) {
        set_runtime_status(FLYUX_TYPE_ERROR, "(pow) exponent must be a number");
        return box_null_typed(VALUE_NUMBER);
    }
    
    set_runtime_status(FLYUX_OK, NULL);
    return box_number(pow(base->data.number, exp->data.number));
}

/* min(a, b) -> num - 最小值 */
Value* value_min(Value* a, Value* b) {
    if (!a || a->type != VALUE_NUMBER) {
        set_runtime_status(FLYUX_TYPE_ERROR, "(min) first argument must be a number");
        return box_null_typed(VALUE_NUMBER);
    }
    if (!b || b->type != VALUE_NUMBER) {
        set_runtime_status(FLYUX_TYPE_ERROR, "(min) second argument must be a number");
        return box_null_typed(VALUE_NUMBER);
    }
    
    set_runtime_status(FLYUX_OK, NULL);
    double result = (a->data.number < b->data.number) ? a->data.number : b->data.number;
    return box_number(result);
}

/* max(a, b) -> num - 最大值 */
Value* value_max(Value* a, Value* b) {
    if (!a || a->type != VALUE_NUMBER) {
        set_runtime_status(FLYUX_TYPE_ERROR, "(max) first argument must be a number");
        return box_null_typed(VALUE_NUMBER);
    }
    if (!b || b->type != VALUE_NUMBER) {
        set_runtime_status(FLYUX_TYPE_ERROR, "(max) second argument must be a number");
        return box_null_typed(VALUE_NUMBER);
    }
    
    set_runtime_status(FLYUX_OK, NULL);
    double result = (a->data.number > b->data.number) ? a->data.number : b->data.number;
    return box_number(result);
}

/* random() -> num - 返回 [0,1) 之间的随机数 */
Value* value_random() {
    static int initialized = 0;
    if (!initialized) {
        srand((unsigned)time(NULL));
        initialized = 1;
    }
    
    set_runtime_status(FLYUX_OK, NULL);
    return box_number((double)rand() / RAND_MAX);
}

/* startsWith(str, prefix) -> bl - 判断字符串是否以指定前缀开头 */
Value* value_starts_with(Value* str, Value* prefix) {
    if (!str || str->type != VALUE_STRING) {
        set_runtime_status(FLYUX_TYPE_ERROR, "(startsWith) first argument must be a string");
        return box_bool(0);
    }
    if (!prefix || prefix->type != VALUE_STRING) {
        set_runtime_status(FLYUX_TYPE_ERROR, "(startsWith) second argument must be a string");
        return box_bool(0);
    }
    
    const char* s = (const char*)str->data.pointer;
    const char* p = (const char*)prefix->data.pointer;
    
    set_runtime_status(FLYUX_OK, NULL);
    return box_bool(strncmp(s, p, strlen(p)) == 0);
}

/* endsWith(str, suffix) -> bl - 判断字符串是否以指定后缀结尾 */
Value* value_ends_with(Value* str, Value* suffix) {
    if (!str || str->type != VALUE_STRING) {
        set_runtime_status(FLYUX_TYPE_ERROR, "(endsWith) first argument must be a string");
        return box_bool(0);
    }
    if (!suffix || suffix->type != VALUE_STRING) {
        set_runtime_status(FLYUX_TYPE_ERROR, "(endsWith) second argument must be a string");
        return box_bool(0);
    }
    
    const char* s = (const char*)str->data.pointer;
    const char* p = (const char*)suffix->data.pointer;
    size_t s_len = strlen(s);
    size_t p_len = strlen(p);
    
    if (p_len > s_len) {
        set_runtime_status(FLYUX_OK, NULL);
        return box_bool(0);
    }
    
    set_runtime_status(FLYUX_OK, NULL);
    return box_bool(strcmp(s + s_len - p_len, p) == 0);
}

/* contains(str, substr) -> bl - 判断字符串是否包含子串 */
Value* value_contains(Value* str, Value* substr) {
    if (!str || str->type != VALUE_STRING) {
        set_runtime_status(FLYUX_TYPE_ERROR, "(contains) first argument must be a string");
        return box_bool(0);
    }
    if (!substr || substr->type != VALUE_STRING) {
        set_runtime_status(FLYUX_TYPE_ERROR, "(contains) second argument must be a string");
        return box_bool(0);
    }
    
    const char* s = (const char*)str->data.pointer;
    const char* p = (const char*)substr->data.pointer;
    
    set_runtime_status(FLYUX_OK, NULL);
    return box_bool(strstr(s, p) != NULL);
}

// ========================================
// 时间函数 (Time Functions)
// ========================================

// now() - 获取当前时间戳(毫秒)
Value* value_now() {
    set_runtime_status(FLYUX_OK, NULL);
    
#ifdef _WIN32
    // Windows: 使用 GetSystemTimePreciseAsFileTime
    FILETIME ft;
    GetSystemTimePreciseAsFileTime(&ft);
    ULARGE_INTEGER uli;
    uli.LowPart = ft.dwLowDateTime;
    uli.HighPart = ft.dwHighDateTime;
    // FILETIME 是从 1601-01-01 开始的 100 纳秒间隔
    // 转换为 Unix 毫秒时间戳
    double ms = (double)((uli.QuadPart - 116444736000000000ULL) / 10000);
    return box_number(ms);
#else
    // Unix: 使用 gettimeofday
    struct timeval tv;
    gettimeofday(&tv, NULL);
    double ms = (double)tv.tv_sec * 1000.0 + (double)tv.tv_usec / 1000.0;
    return box_number(ms);
#endif
}

// time() - 获取当前Unix时间戳(秒)
Value* value_time() {
    set_runtime_status(FLYUX_OK, NULL);
    return box_number((double)time(NULL));
}

// sleep(seconds) - 休眠指定秒数
Value* value_sleep(Value* seconds) {
    if (!seconds || seconds->type != VALUE_NUMBER) {
        set_runtime_status(FLYUX_TYPE_ERROR, "(sleep) argument must be a number");
        return box_null();
    }
    
    double sec = seconds->data.number;
    if (sec < 0) {
        set_runtime_status(FLYUX_TYPE_ERROR, "(sleep) argument must be non-negative");
        return box_null();
    }
    
    // 使用 usleep (微秒)
    usleep((unsigned int)(sec * 1000000));
    
    set_runtime_status(FLYUX_OK, NULL);
    return box_null();
}

// date() - 获取当前日期时间字符串 (格式: "YYYY-MM-DD HH:MM:SS")
Value* value_date() {
    time_t now = time(NULL);
    struct tm* tm_info = localtime(&now);
    
    char buffer[32];
    strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", tm_info);
    
    // 需要复制字符串，因为buffer是栈上的
    char* date_str = (char*)malloc(strlen(buffer) + 1);
    strcpy(date_str, buffer);
    
    set_runtime_status(FLYUX_OK, NULL);
    return box_string_owned(date_str);
}

// ========================================
// 系统操作函数 (System Functions)
// ========================================

// exit(code) - 退出程序
Value* value_exit(Value* code) {
    int exit_code = 0;
    
    if (code && code->type == VALUE_NUMBER) {
        exit_code = (int)code->data.number;
    }
    
    exit(exit_code);
    // 不会执行到这里
    return box_null();
}

// getEnv(name) - 获取环境变量
Value* value_get_env(Value* name) {
    if (!name || name->type != VALUE_STRING) {
        set_runtime_status(FLYUX_TYPE_ERROR, "(getEnv) argument must be a string");
        return box_null_typed(VALUE_STRING);
    }
    
    const char* var_name = (const char*)name->data.pointer;
    const char* value = getenv(var_name);
    
    if (value == NULL) {
        set_runtime_status(FLYUX_OK, NULL);
        return box_null_typed(VALUE_STRING);
    }
    
    // 需要复制环境变量的值
    char* env_value = (char*)malloc(strlen(value) + 1);
    strcpy(env_value, value);
    
    set_runtime_status(FLYUX_OK, NULL);
    return box_string_owned(env_value);
}

// setEnv(name, value) - 设置环境变量
Value* value_set_env(Value* name, Value* value) {
    if (!name || name->type != VALUE_STRING) {
        set_runtime_status(FLYUX_TYPE_ERROR, "(setEnv) first argument must be a string");
        return box_bool(0);
    }
    if (!value || value->type != VALUE_STRING) {
        set_runtime_status(FLYUX_TYPE_ERROR, "(setEnv) second argument must be a string");
        return box_bool(0);
    }
    
    const char* var_name = (const char*)name->data.pointer;
    const char* var_value = (const char*)value->data.pointer;
    
    int result = setenv(var_name, var_value, 1);
    
    if (result != 0) {
        set_runtime_status(FLYUX_IO_ERROR, "(setEnv) failed to set environment variable");
        return box_bool(0);
    }
    
    set_runtime_status(FLYUX_OK, NULL);
    return box_bool(1);
}

// ========================================
// 实用工具函数 (Utility Functions)
// ========================================

// isNaN(value) - 判断是否为NaN
Value* value_is_nan(Value* val) {
    if (!val || val->type != VALUE_NUMBER) {
        set_runtime_status(FLYUX_OK, NULL);
        return box_bool(0);
    }
    
    set_runtime_status(FLYUX_OK, NULL);
    return box_bool(isnan(val->data.number));
}

// isFinite(value) - 判断是否为有限数
Value* value_is_finite(Value* val) {
    if (!val || val->type != VALUE_NUMBER) {
        set_runtime_status(FLYUX_OK, NULL);
        return box_bool(0);
    }
    
    set_runtime_status(FLYUX_OK, NULL);
    return box_bool(isfinite(val->data.number));
}

// clamp(value, min, max) - 限制数值在范围内
Value* value_clamp(Value* val, Value* min_val, Value* max_val) {
    if (!val || val->type != VALUE_NUMBER) {
        set_runtime_status(FLYUX_TYPE_ERROR, "(clamp) first argument must be a number");
        return box_null_typed(VALUE_NUMBER);
    }
    if (!min_val || min_val->type != VALUE_NUMBER) {
        set_runtime_status(FLYUX_TYPE_ERROR, "(clamp) second argument must be a number");
        return box_null_typed(VALUE_NUMBER);
    }
    if (!max_val || max_val->type != VALUE_NUMBER) {
        set_runtime_status(FLYUX_TYPE_ERROR, "(clamp) third argument must be a number");
        return box_null_typed(VALUE_NUMBER);
    }
    
    double value = val->data.number;
    double min = min_val->data.number;
    double max = max_val->data.number;
    
    if (min > max) {
        set_runtime_status(FLYUX_TYPE_ERROR, "(clamp) min must be less than or equal to max");
        return box_null_typed(VALUE_NUMBER);
    }
    
    if (value < min) value = min;
    if (value > max) value = max;
    
    set_runtime_status(FLYUX_OK, NULL);
    return box_number(value);
}

