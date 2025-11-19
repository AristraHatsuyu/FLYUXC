/* Runtime support functions for FLYUX mixed-type system */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <unistd.h>
#include <stdarg.h>
#include <ctype.h>

/* ANSI Color codes (JS console style) */
#define COLOR_NUM      "\033[38;5;151m"      /* 数字 (浅青色) */
#define ANSI_RED_BROWN  "\033[38;5;173m" /* 字符串 (红褐色) */
#define ANSI_BLUE       "\033[34m"      /* 布尔值 */
#define COLOR_GRAY      "\033[90m"      /* null/undefined */
#define COLOR_RESET     "\033[0m"

/* Rainbow bracket colors (like VSCode) */
#define BRACKET_GOLD    "\033[38;5;220m"  /* 金黄色 */
#define BRACKET_PURPLE  "\033[38;5;176m"  /* 紫色 */
#define BRACKET_CYAN    "\033[38;5;111m"   /* 青色 */

static const char* bracket_colors[] = {
    BRACKET_GOLD,
    BRACKET_PURPLE, 
    BRACKET_CYAN
};
#define NUM_BRACKET_COLORS 3

/* Check if we should use colors (TTY detection) */
static int should_use_colors() {
    static int checked = 0;
    static int use_colors = 0;
    
    if (!checked) {
        use_colors = isatty(fileno(stdout));
        checked = 1;
    }
    
    return use_colors;
}

/* Value type tags */
#define VALUE_NUMBER 0
#define VALUE_STRING 1
#define VALUE_ARRAY 2
#define VALUE_OBJECT 3
#define VALUE_BOOL 4
#define VALUE_NULL 5
#define VALUE_UNDEF 6

/* ============================================================================
 * 运行时状态系统 (Runtime State System)
 * ============================================================================
 * 用于追踪函数执行状态，支持错误处理和异常情况
 * 
 * 状态码：
 *   0 - OK: 操作成功
 *   1 - ERROR: 一般错误
 *   2 - EOF: 文件结束/输入结束
 *   3 - TYPE_ERROR: 类型错误
 *   4 - OUT_OF_BOUNDS: 越界错误
 *   5 - IO_ERROR: 输入输出错误
 * ============================================================================
 */

#define FLYUX_OK            0
#define FLYUX_ERROR         1
#define FLYUX_EOF           2
#define FLYUX_TYPE_ERROR    3
#define FLYUX_OUT_OF_BOUNDS 4
#define FLYUX_IO_ERROR      5

/* 全局运行时状态 */
typedef struct {
    int last_status;        /* 最后一次操作的状态码 */
    char error_msg[256];    /* 错误消息 */
    int error_line;         /* 错误行号（供调试用）*/
} RuntimeState;

static RuntimeState g_runtime_state = {
    .last_status = FLYUX_OK,
    .error_msg = "",
    .error_line = 0
};

/* 设置运行时状态 */
static void set_runtime_status(int status, const char *message) {
    g_runtime_state.last_status = status;
    if (message) {
        snprintf(g_runtime_state.error_msg, sizeof(g_runtime_state.error_msg), "%s", message);
    } else {
        g_runtime_state.error_msg[0] = '\0';
    }
}

/* 获取最后的状态码 */
int flyux_get_last_status() {
    return g_runtime_state.last_status;
}

/* 获取最后的错误消息 */
const char* flyux_get_last_error() {
    return g_runtime_state.error_msg;
}

/* 清除错误状态 */
void flyux_clear_error() {
    g_runtime_state.last_status = FLYUX_OK;
    g_runtime_state.error_msg[0] = '\0';
}

/* ============================================================================
 * Value 结构和基础操作
 * ============================================================================
 */

/* Value structure */
typedef struct {
    int type;           /* 当前值的实际类型 */
    int declared_type;  /* 变量的声明类型（用于类型注解）*/
    union {
        double number;
        char *string;
        void *pointer;
    } data;
    long array_size;    /* 数组大小（仅当type==VALUE_ARRAY时有效）*/
    size_t string_length;  /* 字符串长度（支持包含\0的字符串）*/
} Value;

/* Object key-value pair (after Value definition) */
typedef struct ObjectEntry {
    char *key;
    Value *value;
} ObjectEntry;

/* Box a number into a Value */
Value* box_number(double num) {
    Value *v = (Value*)malloc(sizeof(Value));
    v->type = VALUE_NUMBER;
    v->declared_type = VALUE_NUMBER;  /* 默认声明类型等于实际类型 */
    v->data.number = num;
    return v;
}

/* Box a string into a Value */
Value* box_string(char *str) {
    Value *v = (Value*)malloc(sizeof(Value));
    v->type = VALUE_STRING;
    v->declared_type = VALUE_STRING;
    v->data.string = str;  // 不复制，直接使用全局常量
    v->string_length = str ? strlen(str) : 0;  // 保存长度，支持\0字符串
    return v;
}

/* Box a string with explicit length (supports \0 in string) */
Value* box_string_with_length(char *str, size_t len) {
    Value *v = (Value*)malloc(sizeof(Value));
    v->type = VALUE_STRING;
    v->declared_type = VALUE_STRING;
    v->data.string = str;  // 不复制，直接使用全局常量
    v->string_length = len;  // 使用显式长度，支持包含\0的字符串
    return v;
}

/* Box a boolean into a Value */
Value* box_bool(int b) {
    Value *v = (Value*)malloc(sizeof(Value));
    v->type = VALUE_BOOL;
    v->declared_type = VALUE_BOOL;
    v->data.number = b ? 1.0 : 0.0;
    return v;
}

/* Box null */
Value* box_null() {
    Value *v = (Value*)malloc(sizeof(Value));
    v->type = VALUE_NULL;
    v->declared_type = VALUE_NULL;  /* 默认声明类型为null */
    return v;
}

/* Box undef - for undefined variables */
Value* box_undef() {
    Value *v = (Value*)malloc(sizeof(Value));
    v->type = VALUE_UNDEF;
    v->declared_type = VALUE_UNDEF;
    return v;
}

/* Box null with declared type - for typed variables */
Value* box_null_typed(int decl_type) {
    Value *v = (Value*)malloc(sizeof(Value));
    v->type = VALUE_NULL;
    v->declared_type = decl_type;  /* 使用指定的声明类型 */
    return v;
}

/* Create null preserving declared_type from existing value */
Value* box_null_preserve_type(Value *old_val) {
    if (!old_val) return box_null();
    return box_null_typed(old_val->declared_type);
}

/* Box an array */
Value* box_array(void *array_ptr, long size) {
    Value *v = (Value*)malloc(sizeof(Value));
    v->type = VALUE_ARRAY;
    v->declared_type = VALUE_ARRAY;
    v->data.pointer = array_ptr;
    v->array_size = size;
    return v;
}

/* Box an object - takes array of ObjectEntry */
Value* box_object(void *entries_ptr, long count) {
    Value *v = (Value*)malloc(sizeof(Value));
    v->type = VALUE_OBJECT;
    v->declared_type = VALUE_OBJECT;
    v->data.pointer = entries_ptr;  /* ObjectEntry* */
    v->array_size = count;          /* number of properties */
    return v;
}

/* Unbox to number (with type coercion) */
double unbox_number(Value *v) {
    if (!v) return 0.0;
    
    switch (v->type) {
        case VALUE_NUMBER:
        case VALUE_BOOL:
            return v->data.number;
        case VALUE_STRING:
            // 尝试解析字符串为数字
            if (v->data.string) {
                double result = 0.0;
                sscanf(v->data.string, "%lf", &result);
                return result;
            }
            return 0.0;
        case VALUE_NULL:
        case VALUE_UNDEF:
            return 0.0;
        default:
            return 0.0;
    }
}

/* Unbox to string (with type coercion) */
char* unbox_string(Value *v) {
    if (!v) return "(null)";
    
    switch (v->type) {
        case VALUE_STRING:
            return v->data.string ? v->data.string : "(empty)";
        case VALUE_NUMBER:
        case VALUE_BOOL: {
            char *buf = (char*)malloc(32);
            snprintf(buf, 32, "%g", v->data.number);
            return buf;
        }
        case VALUE_NULL:
        case VALUE_UNDEF:
            return "";
        default:
            return "(object)";
    }
}

/* Check if value is truthy */
int value_is_truthy(Value *v) {
    if (!v) return 0;
    
    switch (v->type) {
        case VALUE_NUMBER:
        case VALUE_BOOL:
            return v->data.number != 0.0;
        case VALUE_STRING:
            return v->data.string && v->data.string[0] != '\0';
        case VALUE_NULL:
        case VALUE_UNDEF:
            return 0;
        default:
            return 1;  // objects/arrays are truthy
    }
}

/* Helper: Convert value to JSON string representation */
static void value_to_json_string(Value *v, char *buf, size_t size) {
    if (!v) {
        snprintf(buf, size, "null");
        return;
    }
    
    switch (v->type) {
        case VALUE_NUMBER:
            if (isinf(v->data.number)) {
                snprintf(buf, size, "%s", v->data.number > 0 ? "+Inf" : "-Inf");
            } else if (isnan(v->data.number)) {
                snprintf(buf, size, "NaN");
            } else {
                snprintf(buf, size, "%.16g", v->data.number);
            }
            break;
        case VALUE_STRING:
            snprintf(buf, size, "\"%s\"", v->data.string);
            break;
        case VALUE_BOOL:
            snprintf(buf, size, "%s", v->data.number != 0 ? "true" : "false");
            break;
        case VALUE_NULL:
            snprintf(buf, size, "null");
            break;
        case VALUE_ARRAY:
            snprintf(buf, size, "[...]");
            break;
        case VALUE_OBJECT:
            snprintf(buf, size, "{...}");
            break;
        default:
            snprintf(buf, size, "<unknown>");
    }
}

/* 智能数字格式化 - 自动清理浮点误差 */
static void print_smart_number(double num, int use_colors) {
    // 特殊值检查
    if (isinf(num)) {
        if (use_colors) {
            printf("%s%s%s", COLOR_NUM, num > 0 ? "+Inf" : "-Inf", COLOR_RESET);
        } else {
            printf("%s", num > 0 ? "+Inf" : "-Inf");
        }
        return;
    } else if (isnan(num)) {
        if (use_colors) {
            printf("%sNaN%s", COLOR_NUM, COLOR_RESET);
        } else {
            printf("NaN");
        }
        return;
    }
    
    // 对于非常接近整数的值，显示为整数
    double rounded = round(num);
    if (fabs(num - rounded) < 1e-9 && fabs(rounded) < 1e15) {
        if (use_colors) {
            printf("%s%.0f%s", COLOR_NUM, rounded, COLOR_RESET);
        } else {
            printf("%.0f", rounded);
        }
        return;
    }
    
    // 对于小数，尝试找到合理的精度
    // 使用 %.16g 但去除尾部无意义的 0 和浮点误差
    char buf[64];
    snprintf(buf, sizeof(buf), "%.16g", num);
    
    // 如果数字非常小或非常大，保持科学计数法
    if (fabs(num) < 1e-4 || fabs(num) >= 1e15) {
        if (use_colors) {
            printf("%s%s%s", COLOR_NUM, buf, COLOR_RESET);
        } else {
            printf("%s", buf);
        }
        return;
    }
    
    // 对于普通小数，尝试用更少的精度重新格式化以避免浮点误差
    // 尝试 1-15 位小数，找到最短的精确表示
    // 使用截断而不是四舍五入，避免精度边界问题
    for (int precision = 1; precision <= 15; precision++) {
        // 先用高精度格式化（precision+2 保证有足够的位数）
        char temp_buf[64];
        snprintf(temp_buf, sizeof(temp_buf), "%.*f", precision + 2, num);
        
        // 手动截断到目标精度（不四舍五入）
        char *dot = strchr(temp_buf, '.');
        if (dot && strlen(dot) > precision + 1) {
            dot[precision + 1] = '\0';  // 截断到目标精度
        }
        
        // 检查截断后的值是否足够接近原值
        double reparsed = atof(temp_buf);
        if (fabs(reparsed - num) < 1e-15) {
            // 去除尾部的 0
            size_t len = strlen(temp_buf);
            while (len > 0 && temp_buf[len-1] == '0') {
                temp_buf[--len] = '\0';
            }
            // 如果最后是小数点，也去掉
            if (len > 0 && temp_buf[len-1] == '.') {
                temp_buf[--len] = '\0';
            }
            
            if (use_colors) {
                printf("%s%s%s", COLOR_NUM, temp_buf, COLOR_RESET);
            } else {
                printf("%s", temp_buf);
            }
            return;
        }
    }
    
    // 降级方案：使用 %.16g
    snprintf(buf, sizeof(buf), "%.16g", num);
    if (use_colors) {
        printf("%s%s%s", COLOR_NUM, buf, COLOR_RESET);
    } else {
        printf("%s", buf);
    }
}

/* 递归打印数组内容为JSON格式，支持嵌套层级的彩虹括号 */
static void print_array_json_depth(Value **arr, long size, int depth);
static void print_object_json_depth(ObjectEntry *entries, long count, int depth);

static void print_value_json_depth(Value *v, int depth) {
    int use_colors = should_use_colors();
    
    if (!v) {
        if (use_colors) printf("%snull%s", COLOR_GRAY, COLOR_RESET);
        else printf("null");
        return;
    }
    
    switch (v->type) {
        case VALUE_NUMBER: {
            print_smart_number(v->data.number, use_colors);
            break;
        }
        case VALUE_STRING:
            if (use_colors) {
                printf("%s\"%s\"%s", ANSI_RED_BROWN, v->data.string, COLOR_RESET);
            } else {
                printf("\"%s\"", v->data.string);
            }
            break;
        case VALUE_BOOL:
            if (use_colors) {
                printf("%s%s%s", ANSI_BLUE, v->data.number != 0 ? "true" : "false", COLOR_RESET);
            } else {
                printf("%s", v->data.number != 0 ? "true" : "false");
            }
            break;
        case VALUE_NULL:
            if (use_colors) printf("%snull%s", COLOR_GRAY, COLOR_RESET);
            else printf("null");
            break;
        case VALUE_UNDEF:
            if (use_colors) printf("%sundef%s", COLOR_GRAY, COLOR_RESET);
            else printf("undef");
            break;
        case VALUE_ARRAY: {
            Value **nested = (Value **)v->data.pointer;
            print_array_json_depth(nested, v->array_size, depth);
            break;
        }
        case VALUE_OBJECT: {
            ObjectEntry *entries = (ObjectEntry *)v->data.pointer;
            print_object_json_depth(entries, v->array_size, depth);
            break;
        }
        default:
            if (use_colors) printf("%sunknown%s", COLOR_GRAY, COLOR_RESET);
            else printf("unknown");
    }
}

static void print_array_json_depth(Value **arr, long size, int depth) {
    int use_colors = should_use_colors();
    const char* bracket_color = use_colors ? bracket_colors[depth % NUM_BRACKET_COLORS] : "";
    
    if (use_colors) printf("%s", bracket_color);
    printf("[");
    if (use_colors) printf("%s", COLOR_RESET);
    
    for (long i = 0; i < size; i++) {
        if (i > 0) printf(", ");
        print_value_json_depth(arr[i], depth + 1);
    }
    
    if (use_colors) printf("%s", bracket_color);
    printf("]");
    if (use_colors) printf("%s", COLOR_RESET);
}

/* 递归打印数组内容为JSON格式（兼容接口）*/
static void print_array_json(Value **arr, long size) {
    print_array_json_depth(arr, size, 0);
}

/* 打印对象内容为JSON格式 */
static void print_object_json_depth(ObjectEntry *entries, long count, int depth) {
    int use_colors = should_use_colors();
    const char* bracket_color = use_colors ? bracket_colors[depth % NUM_BRACKET_COLORS] : "";
    
    if (use_colors) printf("%s", bracket_color);
    printf("{ ");
    if (use_colors) printf("%s", COLOR_RESET);
    
    for (long i = 0; i < count; i++) {
        if (i > 0) printf(", ");
        
        // 打印键（使用默认颜色）
        printf("%s: ", entries[i].key);
        
        // 打印值
        print_value_json_depth(entries[i].value, depth + 1);
    }
    
    if (use_colors) printf("%s", bracket_color);
    printf(" }");
    if (use_colors) printf("%s", COLOR_RESET);
}

/* Print a value */
void value_print(Value *v) {
    int use_colors = should_use_colors();
    
    if (!v) {
        if (use_colors) printf("%sundef%s", COLOR_GRAY, COLOR_RESET);
        else printf("undef");
        return;
    }
    
    switch (v->type) {
        case VALUE_NUMBER:
            print_smart_number(v->data.number, use_colors);
            break;
        case VALUE_STRING:
            /* 直接的字符串不变色，保持默认终端颜色 */
            if (v->data.string && v->string_length > 0) {
                fwrite(v->data.string, 1, v->string_length, stdout);
            }
            break;
        case VALUE_BOOL:
            if (use_colors) {
                printf("%s%s%s", ANSI_BLUE, v->data.number != 0 ? "true" : "false", COLOR_RESET);
            } else {
                printf("%s", v->data.number != 0 ? "true" : "false");
            }
            break;
        case VALUE_NULL:
            if (use_colors) printf("%snull%s", COLOR_GRAY, COLOR_RESET);
            else printf("null");
            break;
        case VALUE_UNDEF:
            if (use_colors) printf("%sundef%s", COLOR_GRAY, COLOR_RESET);
            else printf("undef");
            break;
        case VALUE_ARRAY: {
            /* 输出JSON格式的数组 */
            Value **arr = (Value **)v->data.pointer;
            if (!arr || v->array_size == 0) {
                printf("[]");
            } else {
                print_array_json(arr, v->array_size);
            }
            break;
        }
        case VALUE_OBJECT: {
            /* 输出JSON格式的对象 */
            ObjectEntry *entries = (ObjectEntry *)v->data.pointer;
            if (!entries || v->array_size == 0) {
                printf("{}");
            } else {
                print_object_json_depth(entries, v->array_size, 0);
            }
            break;
        }
        default:
            if (use_colors) printf("%sunknown%s", COLOR_GRAY, COLOR_RESET);
            else printf("unknown");
    }
}

/* Print value with newline */
void value_println(Value *v) {
    if (v) {
        value_print(v);
    }
    printf("\n");
}

/* Get type of value as string */
char* value_typeof(Value *v) {
    if (!v) return "undef";
    
    /* 返回声明类型而不是实际类型 */
    switch (v->declared_type) {
        case VALUE_NUMBER: return "num";
        case VALUE_STRING: return "str";
        case VALUE_BOOL: return "bl";
        case VALUE_NULL: return "null";
        case VALUE_ARRAY: return "arr";
        case VALUE_OBJECT: return "obj";
        default: return "unknown";
    }
}

/* Value arithmetic operations */
Value* value_add(Value *a, Value *b) {
    // String concatenation
    if (a->type == VALUE_STRING || b->type == VALUE_STRING) {
        char *sa = unbox_string(a);
        char *sb = unbox_string(b);
        size_t len = strlen(sa) + strlen(sb) + 1;
        char *result = (char*)malloc(len);
        strcpy(result, sa);
        strcat(result, sb);
        return box_string(result);
    }
    
    // Numeric addition
    double na = unbox_number(a);
    double nb = unbox_number(b);
    return box_number(na + nb);
}

Value* value_subtract(Value *a, Value *b) {
    return box_number(unbox_number(a) - unbox_number(b));
}

Value* value_multiply(Value *a, Value *b) {
    return box_number(unbox_number(a) * unbox_number(b));
}

Value* value_divide(Value *a, Value *b) {
    double dividend = unbox_number(a);
    double divisor = unbox_number(b);
    
    if (divisor == 0.0) {
        if (dividend == 0.0) {
            // 0 / 0 = NaN
            return box_number(NAN);
        } else if (dividend > 0.0) {
            // 正数 / 0 = Infinity
            return box_number(INFINITY);
        } else {
            // 负数 / 0 = -Infinity
            return box_number(-INFINITY);
        }
    }
    
    return box_number(dividend / divisor);
}

Value* value_power(Value *a, Value *b) {
    return box_number(pow(unbox_number(a), unbox_number(b)));
}

/* Value comparison */
Value* value_equals(Value *a, Value *b) {
    if (!a || !b) return box_bool(a == b);
    if (a->type != b->type) {
        // Type coercion comparison
        return box_bool(unbox_number(a) == unbox_number(b));
    }
    
    switch (a->type) {
        case VALUE_NUMBER:
        case VALUE_BOOL:
            return box_bool(a->data.number == b->data.number);
        case VALUE_STRING:
            if (!a->data.string || !b->data.string)
                return box_bool(a->data.string == b->data.string);
            return box_bool(strcmp(a->data.string, b->data.string) == 0);
        default:
            return box_bool(a == b);  // reference equality
    }
}

Value* value_less_than(Value *a, Value *b) {
    return box_bool(unbox_number(a) < unbox_number(b));
}

Value* value_greater_than(Value *a, Value *b) {
    return box_bool(unbox_number(a) > unbox_number(b));
}

/* Array/Object index access - runtime version */
Value* value_index(Value *obj, Value *index) {
    if (!obj) return box_null();
    
    // For arrays stored as pointers
    if (obj->type == VALUE_ARRAY && obj->data.pointer) {
        int idx = (int)unbox_number(index);
        Value **array = (Value **)obj->data.pointer;
        // Note: no bounds checking for now
        return array[idx];
    }
    
    return box_null();
}

/* Free a value */
void value_free(Value *v) {
    if (v) {
        // Note: we don't free strings as they might be global constants
        free(v);
    }
}

/*
 * 将值转换为字符串（用于 %s 格式符，不带引号）
 */
static void value_to_string(Value *v, char *buf, size_t size) {
    if (!v || !buf || size == 0) {
        return;
    }
    
    switch (v->type) {
        case VALUE_NUMBER:
            if (isinf(v->data.number)) {
                snprintf(buf, size, "%s", v->data.number > 0 ? "+Inf" : "-Inf");
            } else if (isnan(v->data.number)) {
                snprintf(buf, size, "NaN");
            } else {
                snprintf(buf, size, "%.16g", v->data.number);
            }
            break;
        case VALUE_STRING:
            snprintf(buf, size, "%s", v->data.string);  // 不带引号
            break;
        case VALUE_BOOL:
            snprintf(buf, size, "%s", v->data.number != 0 ? "true" : "false");
            break;
        case VALUE_NULL:
            snprintf(buf, size, "null");
            break;
        case VALUE_UNDEF:
            snprintf(buf, size, "undef");
            break;
        case VALUE_ARRAY:
            snprintf(buf, size, "[...]");
            break;
        case VALUE_OBJECT:
            snprintf(buf, size, "{...}");
            break;
        default:
            snprintf(buf, size, "unknown");
            break;
    }
}

/*
 * 格式化浮点数到指定精度（截断而不是四舍五入）
 * 用于 printf 的 %f 格式符
 */
static void format_double_truncate(char *buf, size_t buf_size, double num, int precision) {
    if (precision < 0) precision = 6;  // 默认精度
    
    // 使用更高的精度先格式化
    char temp[256];
    snprintf(temp, sizeof(temp), "%.*f", precision + 2, num);
    
    // 找到小数点
    char *dot = strchr(temp, '.');
    if (dot && precision >= 0) {
        // 截断到目标精度（不四舍五入）
        size_t dot_pos = dot - temp;
        if (precision == 0) {
            // precision=0 时不输出小数点
            temp[dot_pos] = '\0';
        } else {
            // precision>0 时保留小数点和小数位
            size_t target_len = dot_pos + 1 + precision;  // 整数部分 + . + 小数位
            if (target_len < strlen(temp)) {
                temp[target_len] = '\0';
            }
        }
    }
    
    snprintf(buf, buf_size, "%s", temp);
}

/* 
 * FLYUX printf 函数
 * 支持格式化输出，类似 C 的 printf
 * 格式说明符：
 *   %d, %i - 整数
 *   %f - 浮点数（默认6位小数）
 *   %.Nf - N位小数的浮点数
 *   %s - 字符串（支持宽度和对齐，如 %10s, %-10s）
 *   %b - 布尔值
 *   %v - 值（自动判断类型，JSON格式）
 *   %% - 百分号字面量
 */
void value_printf(Value *format, Value **args, long arg_count) {
    if (!format || format->type != VALUE_STRING) {
        return;
    }
    
    char *fmt = format->data.string;
    size_t fmt_len = format->string_length;
    long arg_index = 0;
    
    for (size_t i = 0; i < fmt_len; i++) {
        if (fmt[i] == '%' && i + 1 < fmt_len) {
            char spec = fmt[i + 1];
            
            if (spec == '%') {
                // %% -> %
                putchar('%');
                i++;
                continue;
            }
            
            if (arg_index >= arg_count) {
                // 参数不足，打印原样
                putchar('%');
                continue;
            }
            
            Value *arg = args[arg_index];
            
            // 解析格式符：支持 %[-][width][.precision]type
            // 例如：%-10s, %5d, %8.2f
            int left_align = 0;     // 是否左对齐
            int width = 0;          // 宽度
            int precision = -1;     // 精度
            size_t j = i + 1;       // 从 % 后开始解析
            
            // 1. 检查对齐标志
            if (j < fmt_len && fmt[j] == '-') {
                left_align = 1;
                j++;
            }
            
            // 2. 解析宽度
            while (j < fmt_len && isdigit(fmt[j])) {
                width = width * 10 + (fmt[j] - '0');
                j++;
            }
            
            // 3. 解析精度
            if (j < fmt_len && fmt[j] == '.') {
                j++;
                precision = 0;
                while (j < fmt_len && isdigit(fmt[j])) {
                    precision = precision * 10 + (fmt[j] - '0');
                    j++;
                }
            }
            
            // 4. 获取类型符
            if (j >= fmt_len) {
                putchar('%');
                continue;
            }
            spec = fmt[j];
            i = j;  // 更新位置到格式符
            
            switch (spec) {
                case 'd':
                case 'i': {
                    // 整数（带颜色，支持宽度）
                    double num = unbox_number(arg);
                    int use_colors = should_use_colors();
                    
                    char temp[64];
                    snprintf(temp, sizeof(temp), "%lld", (long long)num);
                    
                    if (use_colors) printf(COLOR_NUM);
                    if (width > 0) {
                        printf("%*s", left_align ? -width : width, temp);
                    } else {
                        printf("%s", temp);
                    }
                    if (use_colors) printf(COLOR_RESET);
                    break;
                }
                case 'f':
                case 'g': {
                    // 浮点数（带颜色，支持宽度和精度）
                    double num = unbox_number(arg);
                    int use_colors = should_use_colors();
                    
                    char temp[128];
                    if (isinf(num)) {
                        snprintf(temp, sizeof(temp), "%s", num > 0 ? "+Inf" : "-Inf");
                    } else if (isnan(num)) {
                        snprintf(temp, sizeof(temp), "NaN");
                    } else {
                        if (precision >= 0) {
                            if (spec == 'f') {
                                // 使用截断而不是四舍五入
                                format_double_truncate(temp, sizeof(temp), num, precision);
                            } else {  // 'g'
                                snprintf(temp, sizeof(temp), "%.*g", precision, num);
                            }
                        } else {
                            snprintf(temp, sizeof(temp), "%.16g", num);
                        }
                    }
                    
                    if (use_colors) printf(COLOR_NUM);
                    if (width > 0) {
                        printf("%*s", left_align ? -width : width, temp);
                    } else {
                        printf("%s", temp);
                    }
                    if (use_colors) printf(COLOR_RESET);
                    break;
                }
                case 's': {
                    // 字符串（带颜色，支持宽度和对齐）
                    int use_colors = should_use_colors();
                    char temp_buf[256];
                    
                    // 转换为纯文本字符串（不带引号）
                    value_to_string(arg, temp_buf, sizeof(temp_buf));
                    
                    if (use_colors) printf(ANSI_RED_BROWN);
                    if (width > 0) {
                        printf("%*s", left_align ? -width : width, temp_buf);
                    } else {
                        printf("%s", temp_buf);
                    }
                    if (use_colors) printf(COLOR_RESET);
                    break;
                }
                case 'b': {
                    // 布尔值（带颜色）
                    int use_colors = should_use_colors();
                    
                    char temp_buf[8];
                    if (arg && arg->type == VALUE_BOOL) {
                        snprintf(temp_buf, sizeof(temp_buf), "%s", arg->data.number != 0 ? "true" : "false");
                    } else {
                        snprintf(temp_buf, sizeof(temp_buf), "%s", unbox_number(arg) != 0 ? "true" : "false");
                    }
                    
                    if (use_colors) printf(ANSI_BLUE);
                    if (width > 0) {
                        printf("%*s", left_align ? -width : width, temp_buf);
                    } else {
                        printf("%s", temp_buf);
                    }
                    if (use_colors) printf(COLOR_RESET);
                    break;
                }
                case 'v': {
                    // 值（JSON格式，带颜色）
                    print_value_json_depth(arg, 0);
                    break;
                }
                default:
                    // 未知格式符，打印原样
                    putchar('%');
                    putchar(spec);
                    arg_index--;  // 不消耗参数
                    break;
            }
            
            arg_index++;
            // i 已经指向格式符位置，for 循环会自动 i++
        } else {
            // 普通字符
            putchar(fmt[i]);
        }
    }
}

/* 获取数组长度 */
long value_array_length(Value *v) {
    if (!v || v->type != VALUE_ARRAY) {
        return 0;
    }
    return v->array_size;
}

/* 数组元素访问 */
Value* value_array_get(Value *array, Value *index) {
    if (!array || array->type != VALUE_ARRAY) {
        return box_undef();
    }
    
    double idx = unbox_number(index);
    long i = (long)idx;
    
    if (i < 0 || i >= array->array_size) {
        return box_undef();  // 越界返回 undef
    }
    
    Value **elements = (Value**)array->data.pointer;
    return elements[i];
}

/* ============================================================================
 * 输入输出函数
 * ============================================================================
 */

/* 
 * input(prompt) - 从标准输入读取一行
 * 
 * 参数：
 *   prompt: 提示字符串（可选，可以为 null）
 * 
 * 返回：
 *   成功: 返回输入的字符串（不包含换行符）
 *   失败/EOF: 返回 null，并设置状态码
 * 
 * 状态码：
 *   FLYUX_OK: 读取成功
 *   FLYUX_EOF: 遇到文件结束（Ctrl+D / Ctrl+Z）
 *   FLYUX_IO_ERROR: 读取错误
 * 
 * 示例：
 *   name := input("请输入姓名: ")
 *   if (name == null) {
 *       println("输入已取消")
 *   }
 */
Value* value_input(Value *prompt) {
    // 清除之前的错误状态
    set_runtime_status(FLYUX_OK, NULL);
    
    // 显示提示符（如果提供）
    if (prompt && prompt->type == VALUE_STRING && prompt->data.string) {
        printf("%s", prompt->data.string);
        fflush(stdout);  // 确保提示符立即显示
    }
    
    // 读取输入
    char buffer[4096];  // 支持最多 4KB 的输入
    
    if (fgets(buffer, sizeof(buffer), stdin) == NULL) {
        // 读取失败：可能是 EOF 或错误
        if (feof(stdin)) {
            set_runtime_status(FLYUX_EOF, "End of input (EOF)");
        } else {
            set_runtime_status(FLYUX_IO_ERROR, "Input read error");
        }
        return box_null();
    }
    
    // 移除末尾的换行符
    size_t len = strlen(buffer);
    if (len > 0 && buffer[len - 1] == '\n') {
        buffer[len - 1] = '\0';
        len--;
    }
    // 处理 Windows 的 \r\n
    if (len > 0 && buffer[len - 1] == '\r') {
        buffer[len - 1] = '\0';
        len--;
    }
    
    // 复制字符串并创建 Value
    char *result_str = (char*)malloc(len + 1);
    if (!result_str) {
        set_runtime_status(FLYUX_ERROR, "Memory allocation failed");
        return box_null();
    }
    
    memcpy(result_str, buffer, len + 1);
    
    Value *result = (Value*)malloc(sizeof(Value));
    result->type = VALUE_STRING;
    result->declared_type = VALUE_STRING;
    result->data.string = result_str;
    result->string_length = len;
    
    set_runtime_status(FLYUX_OK, NULL);
    return result;
}

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
    return box_null();
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
                set_runtime_status(FLYUX_TYPE_ERROR, "Empty string cannot be converted to number");
                return box_number(0.0);
            }
            
            char *endptr;
            double result = strtod(v->data.string, &endptr);
            
            // 检查是否有无效字符
            if (endptr == v->data.string || *endptr != '\0') {
                set_runtime_status(FLYUX_TYPE_ERROR, "Invalid number format");
                return box_number(0.0);
            }
            
            return box_number(result);
        }
        
        case VALUE_BOOL:
            return box_number(v->data.number != 0 ? 1.0 : 0.0);
            
        case VALUE_NULL:
            return box_number(0.0);
            
        default:
            set_runtime_status(FLYUX_TYPE_ERROR, "Cannot convert array/object to number");
            return box_number(0.0);
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
        return box_string(str);
    }
    
    char buffer[256];
    
    switch (v->type) {
        case VALUE_STRING: {
            // 复制字符串
            char *str = (char*)malloc(v->string_length + 1);
            memcpy(str, v->data.string, v->string_length);
            str[v->string_length] = '\0';
            return box_string(str);
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
            return box_string(str);
        }
        
        case VALUE_BOOL:
            return box_string(strdup(v->data.number != 0 ? "true" : "false"));
            
        case VALUE_NULL:
            return box_string(strdup("null"));
            
        case VALUE_ARRAY:
        case VALUE_OBJECT: {
            // 使用 JSON 格式
            value_to_string(v, buffer, sizeof(buffer));
            char *str = strdup(buffer);
            return box_string(str);
        }
        
        default:
            return box_string(strdup("unknown"));
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
        return box_null();
    }
    
    // 检查obj是否为对象类型
    if (obj->type != VALUE_OBJECT) {
        return box_null();
    }
    
    // 检查field_name是否为字符串
    if (field_name->type != VALUE_STRING) {
        return box_null();
    }
    
    // 获取字段名
    const char *key = (const char*)field_name->data.pointer;
    
    // 遍历对象的所有字段
    ObjectEntry *entries = (ObjectEntry*)obj->data.pointer;
    for (size_t i = 0; i < obj->array_size; i++) {
        if (strcmp(entries[i].key, key) == 0) {
            return entries[i].value;
        }
    }
    
    // 字段不存在
    return box_null();
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
    
    if (!v) {
        return box_number(0);
    }
    
    switch (v->type) {
        case VALUE_STRING:
            return box_number((double)v->string_length);
        case VALUE_ARRAY:
        case VALUE_OBJECT:
            return box_number((double)v->array_size);
        default:
            return box_number(0);
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
            set_runtime_status(FLYUX_TYPE_ERROR, "charAt requires numeric index");
            return box_null();
        }
        
        int idx = (int)index->data.number;
        if (idx < 0 || idx >= (int)str->array_size) {
            set_runtime_status(FLYUX_OUT_OF_BOUNDS, "Array index out of range");
            return box_null();
        }
        
        Value **elements = (Value**)str->data.pointer;
        return elements[idx];
    }
    
    // 支持字符串访问
    if (!str || str->type != VALUE_STRING) {
        set_runtime_status(FLYUX_TYPE_ERROR, "charAt requires string or array");
        return box_string("");
    }
    
    if (!index || index->type != VALUE_NUMBER) {
        set_runtime_status(FLYUX_TYPE_ERROR, "charAt requires numeric index");
        return box_string("");
    }
    
    int idx = (int)index->data.number;
    if (idx < 0 || idx >= (int)str->string_length) {
        set_runtime_status(FLYUX_OUT_OF_BOUNDS, "Index out of range");
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
        set_runtime_status(FLYUX_TYPE_ERROR, "substr requires string");
        return box_string("");
    }
    
    if (!start || start->type != VALUE_NUMBER) {
        set_runtime_status(FLYUX_TYPE_ERROR, "substr requires numeric start");
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
 * indexOf(str, substr) - 查找子字符串位置
 */
Value* value_index_of(Value *str, Value *substr) {
    set_runtime_status(FLYUX_OK, NULL);
    
    if (!str || str->type != VALUE_STRING || !substr || substr->type != VALUE_STRING) {
        set_runtime_status(FLYUX_TYPE_ERROR, "indexOf requires two strings");
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
        set_runtime_status(FLYUX_TYPE_ERROR, "replace requires three strings");
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
        set_runtime_status(FLYUX_TYPE_ERROR, "split requires string");
        return box_null();
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
        set_runtime_status(FLYUX_TYPE_ERROR, "join requires array");
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
        set_runtime_status(FLYUX_TYPE_ERROR, "trim requires string");
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
        set_runtime_status(FLYUX_TYPE_ERROR, "upper requires string");
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
        set_runtime_status(FLYUX_TYPE_ERROR, "lower requires string");
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
        set_runtime_status(FLYUX_TYPE_ERROR, "push requires array");
        return box_null();
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
        set_runtime_status(FLYUX_TYPE_ERROR, "pop requires array");
        return box_null();
    }
    
    if (arr->array_size == 0) {
        set_runtime_status(FLYUX_OUT_OF_BOUNDS, "Cannot pop from empty array");
        return box_null();
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
        set_runtime_status(FLYUX_TYPE_ERROR, "shift requires array");
        return box_null();
    }
    
    if (arr->array_size == 0) {
        set_runtime_status(FLYUX_OUT_OF_BOUNDS, "Cannot shift from empty array");
        return box_null();
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
        set_runtime_status(FLYUX_TYPE_ERROR, "unshift requires array");
        return box_null();
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
        set_runtime_status(FLYUX_TYPE_ERROR, "slice requires array");
        return box_null();
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
        set_runtime_status(FLYUX_TYPE_ERROR, "concat requires two arrays");
        return box_null();
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
    result->data.pointer = new_elements;
    result->array_size = new_size;
    
    return result;
}
