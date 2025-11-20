/* Runtime support functions for FLYUX mixed-type system */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <unistd.h>
#include <stdarg.h>
#include <ctype.h>
#include <sys/stat.h>
#include <dirent.h>
#include <errno.h>

/* ANSI Color codes (JS console style) */
#define COLOR_NUM      "\033[38;5;151m"      /* 数字 (浅青色) */
#define ANSI_RED_BROWN  "\033[38;5;173m" /* 字符串 (红褐色) */
#define ANSI_BLUE       "\033[34m"      /* 布尔值 */
#define COLOR_GRAY      "\033[90m"      /* null/undefined */
#define COLOR_GREEN     "\033[38;5;79m"      /* 对象/数组 (绿色) */
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

/* Extended object type tags */
#define EXT_TYPE_NONE      0  /* 普通obj */
#define EXT_TYPE_BUFFER    1  /* Buffer类型 */
#define EXT_TYPE_FILE      2  /* FileHandle类型 */
#define EXT_TYPE_ERROR     3  /* Error类型 */

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
    int ext_type;       /* 扩展对象类型标识 (仅当type==VALUE_OBJECT时有效) */
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

/* ============================================================================
 * 扩展对象类型结构定义
 * ============================================================================ */

/* Buffer对象 - 二进制数据容器 */
typedef struct {
    unsigned char *data;  /* 原始二进制数据 */
    size_t size;          /* 数据大小(字节) */
    size_t capacity;      /* 分配容量 */
} BufferObject;

/* FileHandle对象 - 文件句柄 */
typedef struct {
    FILE *fp;             /* C文件指针 */
    char *path;           /* 文件路径 */
    char *mode;           /* 打开模式 */
    int is_open;          /* 是否打开 */
    long position;        /* 当前位置 */
} FileHandleObject;

/* Error对象 - 错误信息 */
typedef struct {
    char *message;        /* 错误消息 */
    int code;             /* 错误代码 */
    char *error_type;     /* 错误类型(Error/TypeError/IOError) */
} ErrorObject;

/* Box a number into a Value */
Value* box_number(double num) {
    Value *v = (Value*)malloc(sizeof(Value));
    v->type = VALUE_NUMBER;
    v->declared_type = VALUE_NUMBER;  /* 默认声明类型等于实际类型 */
    v->ext_type = EXT_TYPE_NONE;
    v->data.number = num;
    return v;
}

/* Box a string into a Value */
Value* box_string(char *str) {
    Value *v = (Value*)malloc(sizeof(Value));
    v->type = VALUE_STRING;
    v->declared_type = VALUE_STRING;
    v->ext_type = EXT_TYPE_NONE;
    v->data.string = str;  // 不复制，直接使用全局常量
    v->string_length = str ? strlen(str) : 0;  // 保存长度，支持\0字符串
    return v;
}

/* Box a string with explicit length (supports \0 in string) */
Value* box_string_with_length(char *str, size_t len) {
    Value *v = (Value*)malloc(sizeof(Value));
    v->type = VALUE_STRING;
    v->declared_type = VALUE_STRING;
    v->ext_type = EXT_TYPE_NONE;
    v->data.string = str;  // 不复制，直接使用全局常量
    v->string_length = len;  // 使用显式长度，支持包含\0的字符串
    return v;
}

/* Box a boolean into a Value */
Value* box_bool(int b) {
    Value *v = (Value*)malloc(sizeof(Value));
    v->type = VALUE_BOOL;
    v->declared_type = VALUE_BOOL;
    v->ext_type = EXT_TYPE_NONE;
    v->data.number = b ? 1.0 : 0.0;
    return v;
}

/* Box null */
Value* box_null() {
    Value *v = (Value*)malloc(sizeof(Value));
    v->type = VALUE_NULL;
    v->declared_type = VALUE_NULL;  /* 默认声明类型为null */
    v->ext_type = EXT_TYPE_NONE;
    return v;
}

/* Box undef - for undefined variables */
Value* box_undef() {
    Value *v = (Value*)malloc(sizeof(Value));
    v->type = VALUE_UNDEF;
    v->declared_type = VALUE_UNDEF;
    v->ext_type = EXT_TYPE_NONE;
    return v;
}

/* Box null with declared type - for typed variables */
Value* box_null_typed(int decl_type) {
    Value *v = (Value*)malloc(sizeof(Value));
    v->type = VALUE_NULL;
    v->declared_type = decl_type;  /* 使用指定的声明类型 */
    v->ext_type = EXT_TYPE_NONE;
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
    v->ext_type = EXT_TYPE_NONE;
    v->data.pointer = array_ptr;
    v->array_size = size;
    return v;
}

/* Box an object - takes array of ObjectEntry */
Value* box_object(void *entries_ptr, long count) {
    Value *v = (Value*)malloc(sizeof(Value));
    v->type = VALUE_OBJECT;
    v->declared_type = VALUE_OBJECT;
    v->ext_type = EXT_TYPE_NONE;  /* 普通obj */
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
static void print_extended_object_meta_depth(Value *v, int depth);

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
            /* 检查是否为扩展对象类型 */
            if (v->ext_type != EXT_TYPE_NONE) {
                print_extended_object_meta_depth(v, depth);
            } else {
                ObjectEntry *entries = (ObjectEntry *)v->data.pointer;
                print_object_json_depth(entries, v->array_size, depth);
            }
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

/* Print extended object meta information */
static void print_extended_object_meta_depth(Value *v, int depth) {
    int use_colors = should_use_colors();
    const char* type_color = use_colors ? COLOR_GREEN : "";
    const char* bracket_color = use_colors ? bracket_colors[depth % NUM_BRACKET_COLORS] : "";
    const char* string_color = use_colors ? ANSI_RED_BROWN : "";
    const char* number_color = use_colors ? COLOR_NUM : "";
    const char* bool_color = use_colors ? ANSI_BLUE : "";
    const char* reset = use_colors ? COLOR_RESET : "";
    
    switch (v->ext_type) {
        case EXT_TYPE_BUFFER: {
            BufferObject *buf = (BufferObject*)v->data.pointer;
            printf("%sBuffer%s %s{%s size: %s%zu%s, type: %s\"Buffer\"%s %s}%s", 
                   type_color, reset, bracket_color, reset,
                   number_color, buf ? buf->size : 0, reset,
                   string_color, reset,
                   bracket_color, reset);
            break;
        }
        case EXT_TYPE_FILE: {
            FileHandleObject *file = (FileHandleObject*)v->data.pointer;
            if (file) {
                printf("%sFileHandle%s %s{%s path: %s\"%s\"%s, mode: %s\"%s\"%s, position: %s%ld%s, isOpen: %s%s%s %s}%s", 
                       type_color, reset, bracket_color, reset,
                       string_color, file->path ? file->path : "", reset,
                       string_color, file->mode ? file->mode : "", reset,
                       number_color, file->position, reset,
                       bool_color, file->is_open ? "true" : "false", reset,
                       bracket_color, reset);
            } else {
                printf("%sFileHandle%s %s{%s %s}%s", type_color, reset, bracket_color, reset, bracket_color, reset);
            }
            break;
        }
        case EXT_TYPE_ERROR: {
            ErrorObject *err = (ErrorObject*)v->data.pointer;
            if (err) {
                printf("%sError%s %s{%s message: %s\"%s\"%s, code: %s%d%s, errorType: %s\"%s\"%s %s}%s",
                       type_color, reset, bracket_color, reset,
                       string_color, err->message ? err->message : "", reset,
                       number_color, err->code, reset,
                       string_color, err->error_type ? err->error_type : "Error", reset,
                       bracket_color, reset);
            } else {
                printf("%sError%s %s{%s %s}%s", type_color, reset, bracket_color, reset, bracket_color, reset);
            }
            break;
        }
        default:
            printf("%sExtendedObject%s %s{%s type: %s%d%s %s}%s", 
                   type_color, reset, bracket_color, reset, 
                   number_color, v->ext_type, reset,
                   bracket_color, reset);
            break;
    }
}

/* Print extended object meta information (wrapper for depth 0) */
static void print_extended_object_meta(Value *v) {
    print_extended_object_meta_depth(v, 0);
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
                int use_colors = should_use_colors();
                const char* bracket_color = use_colors ? bracket_colors[0] : "";
                if (use_colors) printf("%s", bracket_color);
                printf("[]");
                if (use_colors) printf("%s", COLOR_RESET);
            } else {
                print_array_json(arr, v->array_size);
            }
            break;
        }
        case VALUE_OBJECT: {
            /* 检查是否为扩展对象类型 */
            if (v->ext_type != EXT_TYPE_NONE) {
                print_extended_object_meta(v);  /* 顶层打印使用 depth=0 */
                break;
            }
            
            /* 输出JSON格式的普通对象 */
            ObjectEntry *entries = (ObjectEntry *)v->data.pointer;
            if (!entries || v->array_size == 0) {
                int use_colors = should_use_colors();
                const char* bracket_color = use_colors ? bracket_colors[0] : "";
                if (use_colors) printf("%s", bracket_color);
                printf("{}");
                if (use_colors) printf("%s", COLOR_RESET);
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

/* 打印致命错误并退出 */
void value_fatal_error() {
    fprintf(stderr, "\n\033[34mFLYUX 0.1.0\033[0m \033[31m[Err]\033[0m Fatal Error: %s\nAbort.\n", g_runtime_state.error_msg);
    abort();
}

/* Get type of value as string */
char* value_typeof(Value *v) {
    if (!v) return "undef";
    
    /* 检查扩展对象类型 */
    if (v->declared_type == VALUE_OBJECT && v->ext_type != EXT_TYPE_NONE) {
        static char type_buf[64];
        const char *ext_name = NULL;
        
        switch (v->ext_type) {
            case EXT_TYPE_BUFFER:
                ext_name = "Buffer";
                break;
            case EXT_TYPE_FILE:
                ext_name = "FileHandle";
                break;
            case EXT_TYPE_ERROR:
                ext_name = "Error";
                break;
            default:
                ext_name = "Extended";
                break;
        }
        
        snprintf(type_buf, sizeof(type_buf), "obj:%s", ext_name);
        return type_buf;
    }
    
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
    
    // For arrays with numeric index
    if (obj->type == VALUE_ARRAY && obj->data.pointer) {
        int idx = (int)unbox_number(index);
        Value **array = (Value **)obj->data.pointer;
        // Note: no bounds checking for now
        return array[idx];
    }
    
    // For Buffer objects with numeric index
    if (obj->type == VALUE_OBJECT && obj->ext_type == EXT_TYPE_BUFFER && index && index->type == VALUE_NUMBER) {
        BufferObject *buf = (BufferObject*)obj->data.pointer;
        int idx = (int)unbox_number(index);
        
        // 边界检查
        if (idx < 0 || (size_t)idx >= buf->size) {
            set_runtime_status(FLYUX_OUT_OF_BOUNDS, "Buffer index out of bounds");
            return box_null();
        }
        
        // 返回字节值（0-255）
        return box_number((double)buf->data[idx]);
    }
    
    // For objects with string index (inline implementation to avoid forward declaration)
    if (obj->type == VALUE_OBJECT && index && index->type == VALUE_STRING) {
        const char *key = (const char*)index->data.pointer;
        ObjectEntry *entries = (ObjectEntry*)obj->data.pointer;
        size_t count = obj->array_size;
        
        for (size_t i = 0; i < count; i++) {
            if (strcmp(entries[i].key, key) == 0) {
                return entries[i].value;
            }
        }
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
                return box_null_typed(VALUE_NUMBER);
            }
            
            char *endptr;
            double result = strtod(v->data.string, &endptr);
            
            // 检查是否有无效字符
            if (endptr == v->data.string || *endptr != '\0') {
                set_runtime_status(FLYUX_TYPE_ERROR, "Invalid number format");
                return box_null_typed(VALUE_NUMBER);
            }
            
            return box_number(result);
        }
        
        case VALUE_BOOL:
            return box_number(v->data.number != 0 ? 1.0 : 0.0);
            
        case VALUE_NULL:
            return box_number(0.0);
            
        default:
            set_runtime_status(FLYUX_TYPE_ERROR, "Cannot convert array/object to number");
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
    for (size_t i = 0; i < obj->array_size; i++) {
        if (strcmp(entries[i].key, key) == 0) {
            return entries[i].value;
        }
    }
    
    // 字段不存在
    return box_null();
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
 * 
 * 注意：
 *   使用malloc分配新数组而不是realloc，因为原数组可能在栈上
 * 
 * 示例：
 *   value_set_field(obj, box_string("name"), box_string("Alice"))
 */
Value* value_set_field(Value *obj, Value *field_name, Value *value) {
    if (!obj || !field_name || !value) {
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
    
    // 查找是否已存在该字段
    for (size_t i = 0; i < count; i++) {
        if (strcmp(entries[i].key, key) == 0) {
            // 字段已存在，更新值
            entries[i].value = value;
            return box_bool(1);
        }
    }
    
    // 字段不存在，需要添加新字段
    // 分配新数组（不能用realloc，因为原数组可能在栈上）
    ObjectEntry *new_entries = (ObjectEntry*)malloc(sizeof(ObjectEntry) * (count + 1));
    if (!new_entries) {
        return box_bool(0);  // 内存分配失败
    }
    
    // 复制旧entries
    for (size_t i = 0; i < count; i++) {
        new_entries[i].key = entries[i].key;      // 保持原key指针
        new_entries[i].value = entries[i].value;  // 保持原value指针
    }
    
    // 添加新字段
    new_entries[count].key = strdup(key);  // 复制字段名
    new_entries[count].value = value;
    
    // 更新对象指针和大小
    obj->data.pointer = new_entries;
    obj->array_size = count + 1;
    
    // 注意：不释放旧entries，因为可能在栈上
    
    return box_bool(1);
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
 * 注意：
 *   使用malloc分配新数组，不释放旧数组（可能在栈上）
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
        obj->data.pointer = NULL;
        obj->array_size = 0;
        // 注意：不释放entries[0].key，可能是字面量
        return box_bool(1);
    }
    
    // 分配新数组（不用realloc，因为可能在栈上）
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
        // 注意：不释放 entries[found_index].key，可能是字面量
    }
    
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
    for (size_t i = 0; i < count; i++) {
        keys[i] = box_string(entries[i].key);
    }
    
    return box_array(keys, count);
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
 */
Value* value_set_index(Value *obj, Value *index, Value *value) {
    if (!obj || !index || !value) {
        return box_bool(0);
    }
    
    // 如果是数组
    if (obj->type == VALUE_ARRAY && index->type == VALUE_NUMBER) {
        Value **elements = (Value**)obj->data.pointer;
        size_t count = obj->array_size;
        
        double idx_double = index->data.number;
        if (idx_double < 0 || idx_double >= (double)count) {
            return box_bool(0);  // 索引越界
        }
        
        size_t idx = (size_t)idx_double;
        elements[idx] = value;
        return box_bool(1);
    }
    
    // 如果是对象且索引是字符串
    if (obj->type == VALUE_OBJECT && index->type == VALUE_STRING) {
        return value_set_field(obj, index, value);
    }
    
    return box_bool(0);
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
    result->ext_type = EXT_TYPE_NONE;
    result->data.pointer = new_elements;
    result->array_size = new_size;
    
    return result;
}

/* ============================================================================
 * 文件I/O函数实现
 * ============================================================================ */

/* readFile(path) -> string | null - 读取文本文件 */
Value* value_read_file(Value *path) {
    if (!path || path->type != VALUE_STRING) {
        set_runtime_status(FLYUX_TYPE_ERROR, "readFile: path必须是字符串");
        return box_null_typed(VALUE_STRING);
    }
    
    const char *filepath = (const char*)path->data.pointer;
    FILE *fp = fopen(filepath, "r");
    
    if (!fp) {
        set_runtime_status(FLYUX_IO_ERROR, "Cannot open file");
        return box_null_typed(VALUE_STRING);
    }
    
    // 获取文件大小
    fseek(fp, 0, SEEK_END);
    long size = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    
    // 读取文件内容
    char *content = (char*)malloc(size + 1);
    size_t read_size = fread(content, 1, size, fp);
    content[read_size] = '\0';
    fclose(fp);
    
    set_runtime_status(FLYUX_OK, NULL);
    return box_string(content);
}

/* writeFile(path, content) -> bool - 写入文本文件 */
Value* value_write_file(Value *path, Value *content) {
    if (!path || path->type != VALUE_STRING) {
        set_runtime_status(FLYUX_TYPE_ERROR, "writeFile: path必须是字符串");
        return box_bool(0);
    }
    if (!content || content->type != VALUE_STRING) {
        set_runtime_status(FLYUX_TYPE_ERROR, "writeFile: content必须是字符串");
        return box_bool(0);
    }
    
    const char *filepath = (const char*)path->data.pointer;
    const char *text = (const char*)content->data.pointer;
    
    FILE *fp = fopen(filepath, "w");
    if (!fp) {
        set_runtime_status(FLYUX_IO_ERROR, "Cannot create file");
        return box_bool(0);
    }
    
    fputs(text, fp);
    fclose(fp);
    
    set_runtime_status(FLYUX_OK, NULL);
    return box_bool(1);
}

/* appendFile(path, content) -> bool - 追加到文件 */
Value* value_append_file(Value *path, Value *content) {
    if (!path || path->type != VALUE_STRING) {
        set_runtime_status(FLYUX_TYPE_ERROR, "appendFile: path必须是字符串");
        return box_bool(0);
    }
    if (!content || content->type != VALUE_STRING) {
        set_runtime_status(FLYUX_TYPE_ERROR, "appendFile: content必须是字符串");
        return box_bool(0);
    }
    
    const char *filepath = (const char*)path->data.pointer;
    const char *text = (const char*)content->data.pointer;
    
    FILE *fp = fopen(filepath, "a");
    if (!fp) {
        set_runtime_status(FLYUX_IO_ERROR, "Cannot open file");
        return box_bool(0);
    }
    
    fputs(text, fp);
    fclose(fp);
    
    set_runtime_status(FLYUX_OK, NULL);
    return box_bool(1);
}

/* fileExists(path) -> bool - 检查文件是否存在 */
Value* value_file_exists(Value *path) {
    if (!path || path->type != VALUE_STRING) {
        return box_bool(0);
    }
    
    const char *filepath = (const char*)path->data.pointer;
    FILE *fp = fopen(filepath, "r");
    
    if (fp) {
        fclose(fp);
        return box_bool(1);
    }
    return box_bool(0);
}

/* deleteFile(path) -> bool - 删除文件 */
Value* value_delete_file(Value *path) {
    if (!path || path->type != VALUE_STRING) {
        set_runtime_status(FLYUX_TYPE_ERROR, "deleteFile: path必须是字符串");
        return box_bool(0);
    }
    
    const char *filepath = (const char*)path->data.pointer;
    int result = remove(filepath);
    
    if (result == 0) {
        set_runtime_status(FLYUX_OK, NULL);
        return box_bool(1);
    } else {
        set_runtime_status(FLYUX_IO_ERROR, "删除文件失败");
        return box_bool(0);
    }
}

/* getFileSize(path) -> num - 获取文件大小 */
Value* value_get_file_size(Value *path) {
    if (!path || path->type != VALUE_STRING) {
        return box_number(-1);
    }
    
    const char *filepath = (const char*)path->data.pointer;
    FILE *fp = fopen(filepath, "r");
    
    if (!fp) {
        return box_number(-1);
    }
    
    fseek(fp, 0, SEEK_END);
    long size = ftell(fp);
    fclose(fp);
    
    return box_number((double)size);
}

/* readBytes(path) -> Buffer | null - 读取二进制文件 */
Value* value_read_bytes(Value *path) {
    if (!path || path->type != VALUE_STRING) {
        set_runtime_status(FLYUX_TYPE_ERROR, "readBytes: path必须是字符串");
        return box_null_typed(VALUE_OBJECT);
    }
    
    const char *filepath = (const char*)path->data.pointer;
    FILE *fp = fopen(filepath, "rb");
    
    if (!fp) {
        set_runtime_status(FLYUX_IO_ERROR, "Cannot open file");
        return box_null_typed(VALUE_OBJECT);
    }
    
    // 获取文件大小
    fseek(fp, 0, SEEK_END);
    long size = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    
    // 创建Buffer对象
    BufferObject *buffer = (BufferObject*)malloc(sizeof(BufferObject));
    buffer->data = (unsigned char*)malloc(size);
    buffer->size = size;
    buffer->capacity = size;
    
    size_t read_size = fread(buffer->data, 1, size, fp);
    buffer->size = read_size;
    fclose(fp);
    
    // 创建Value
    Value *v = (Value*)malloc(sizeof(Value));
    v->type = VALUE_OBJECT;
    v->declared_type = VALUE_OBJECT;
    v->ext_type = EXT_TYPE_BUFFER;
    v->data.pointer = buffer;
    v->array_size = 0;
    
    set_runtime_status(FLYUX_OK, NULL);
    return v;
}

/* writeBytes(path, data) -> bool - 写入二进制文件 */
Value* value_write_bytes(Value *path, Value *data) {
    if (!path || path->type != VALUE_STRING) {
        set_runtime_status(FLYUX_TYPE_ERROR, "writeBytes: path必须是字符串");
        return box_bool(0);
    }
    
    const char *filepath = (const char*)path->data.pointer;
    FILE *fp = fopen(filepath, "wb");
    
    if (!fp) {
        set_runtime_status(FLYUX_IO_ERROR, "Cannot create file");
        return box_bool(0);
    }
    
    if (data->type == VALUE_OBJECT && data->ext_type == EXT_TYPE_BUFFER) {
        // Buffer对象
        BufferObject *buf = (BufferObject*)data->data.pointer;
        fwrite(buf->data, 1, buf->size, fp);
    } else if (data->type == VALUE_ARRAY) {
        // 数字数组
        Value **arr = (Value **)data->data.pointer;
        for (long i = 0; i < data->array_size; i++) {
            unsigned char byte = (unsigned char)unbox_number(arr[i]);
            fputc(byte, fp);
        }
    } else {
        fclose(fp);
        set_runtime_status(FLYUX_TYPE_ERROR, "writeBytes: data必须是Buffer或数组");
        return box_bool(0);
    }
    
    fclose(fp);
    set_runtime_status(FLYUX_OK, NULL);
    return box_bool(1);
}

/* ============================================================================
 * Phase 1: 核心文件I/O扩展函数
 * ============================================================================ */

/* readLines(path) -> str[] - 逐行读取文本文件 */
Value* value_read_lines(Value *path) {
    if (!path || path->type != VALUE_STRING) {
        set_runtime_status(FLYUX_TYPE_ERROR, "readLines: path必须是字符串");
        return box_null_typed(VALUE_ARRAY);
    }
    
    const char *filepath = (const char*)path->data.pointer;
    FILE *fp = fopen(filepath, "r");
    
    if (!fp) {
        set_runtime_status(FLYUX_IO_ERROR, "Cannot open file");
        return box_null_typed(VALUE_ARRAY);
    }
    
    // 动态数组存储行
    size_t capacity = 16;
    size_t count = 0;
    Value **lines = (Value **)malloc(capacity * sizeof(Value *));
    
    char *line = NULL;
    size_t len = 0;
    ssize_t read;
    
    while ((read = getline(&line, &len, fp)) != -1) {
        // 移除换行符
        if (read > 0 && line[read - 1] == '\n') {
            line[read - 1] = '\0';
            read--;
        }
        if (read > 0 && line[read - 1] == '\r') {
            line[read - 1] = '\0';
            read--;
        }
        
        // 扩容
        if (count >= capacity) {
            capacity *= 2;
            Value **new_lines = (Value **)realloc(lines, capacity * sizeof(Value *));
            if (!new_lines) {
                // 清理已分配的内存
                for (size_t i = 0; i < count; i++) {
                    free(lines[i]);
                }
                free(lines);
                free(line);
                fclose(fp);
                set_runtime_status(FLYUX_ERROR, "内存分配失败");
                return box_null_typed(VALUE_ARRAY);
            }
            lines = new_lines;
        }
        
        // 复制行内容（需要strdup因为line会被重用）
        char *line_copy = strdup(line);
        lines[count++] = box_string(line_copy);
    }
    
    free(line);
    fclose(fp);
    
    // 创建数组Value
    char *arr_ptr = (char*)lines;
    Value *result = box_array(arr_ptr, count);
    
    set_runtime_status(FLYUX_OK, NULL);
    return result;
}

/* renameFile(oldPath, newPath) -> bool - 重命名/移动文件 */
Value* value_rename_file(Value *old_path, Value *new_path) {
    if (!old_path || old_path->type != VALUE_STRING) {
        set_runtime_status(FLYUX_TYPE_ERROR, "renameFile: oldPath必须是字符串");
        return box_bool(0);
    }
    
    if (!new_path || new_path->type != VALUE_STRING) {
        set_runtime_status(FLYUX_TYPE_ERROR, "renameFile: newPath必须是字符串");
        return box_bool(0);
    }
    
    const char *old_filepath = (const char*)old_path->data.pointer;
    const char *new_filepath = (const char*)new_path->data.pointer;
    
    int result = rename(old_filepath, new_filepath);
    
    if (result == 0) {
        set_runtime_status(FLYUX_OK, NULL);
        return box_bool(1);
    } else {
        set_runtime_status(FLYUX_IO_ERROR, "重命名文件失败");
        return box_bool(0);
    }
}

/* copyFile(src, dest) -> bool - 复制文件 */
Value* value_copy_file(Value *src_path, Value *dest_path) {
    if (!src_path || src_path->type != VALUE_STRING) {
        set_runtime_status(FLYUX_TYPE_ERROR, "copyFile: src必须是字符串");
        return box_bool(0);
    }
    
    if (!dest_path || dest_path->type != VALUE_STRING) {
        set_runtime_status(FLYUX_TYPE_ERROR, "copyFile: dest必须是字符串");
        return box_bool(0);
    }
    
    const char *src = (const char*)src_path->data.pointer;
    const char *dest = (const char*)dest_path->data.pointer;
    
    FILE *src_fp = fopen(src, "rb");
    if (!src_fp) {
        set_runtime_status(FLYUX_IO_ERROR, "Cannot open source file");
        return box_bool(0);
    }
    
    FILE *dest_fp = fopen(dest, "wb");
    if (!dest_fp) {
        fclose(src_fp);
        set_runtime_status(FLYUX_IO_ERROR, "Cannot create destination file");
        return box_bool(0);
    }
    
    // 缓冲区复制
    char buffer[8192];
    size_t bytes;
    
    while ((bytes = fread(buffer, 1, sizeof(buffer), src_fp)) > 0) {
        if (fwrite(buffer, 1, bytes, dest_fp) != bytes) {
            fclose(src_fp);
            fclose(dest_fp);
            set_runtime_status(FLYUX_IO_ERROR, "写入目标文件失败");
            return box_bool(0);
        }
    }
    
    fclose(src_fp);
    fclose(dest_fp);
    
    // 复制文件权限
    struct stat st;
    if (stat(src, &st) == 0) {
        chmod(dest, st.st_mode);
    }
    
    set_runtime_status(FLYUX_OK, NULL);
    return box_bool(1);
}

/* createDir(path) -> bool - 创建目录 */
Value* value_create_dir(Value *path) {
    if (!path || path->type != VALUE_STRING) {
        set_runtime_status(FLYUX_TYPE_ERROR, "createDir: path必须是字符串");
        return box_bool(0);
    }
    
    const char *dirpath = (const char*)path->data.pointer;
    
#ifdef _WIN32
    int result = _mkdir(dirpath);
#else
    int result = mkdir(dirpath, 0755);
#endif
    
    if (result == 0) {
        set_runtime_status(FLYUX_OK, NULL);
        return box_bool(1);
    } else {
        if (errno == EEXIST) {
            set_runtime_status(FLYUX_IO_ERROR, "目录已存在");
        } else {
            set_runtime_status(FLYUX_IO_ERROR, "创建目录失败");
        }
        return box_bool(0);
    }
}

/* removeDir(path) -> bool - 删除空目录 */
Value* value_remove_dir(Value *path) {
    if (!path || path->type != VALUE_STRING) {
        set_runtime_status(FLYUX_TYPE_ERROR, "removeDir: path必须是字符串");
        return box_bool(0);
    }
    
    const char *dirpath = (const char*)path->data.pointer;
    
#ifdef _WIN32
    int result = _rmdir(dirpath);
#else
    int result = rmdir(dirpath);
#endif
    
    if (result == 0) {
        set_runtime_status(FLYUX_OK, NULL);
        return box_bool(1);
    } else {
        if (errno == ENOTEMPTY) {
            set_runtime_status(FLYUX_IO_ERROR, "目录不为空");
        } else if (errno == ENOENT) {
            set_runtime_status(FLYUX_IO_ERROR, "目录不存在");
        } else {
            set_runtime_status(FLYUX_IO_ERROR, "删除目录失败");
        }
        return box_bool(0);
    }
}

/* listDir(path) -> str[] - 列出目录内容 */
Value* value_list_dir(Value *path) {
    if (!path || path->type != VALUE_STRING) {
        set_runtime_status(FLYUX_TYPE_ERROR, "listDir: path必须是字符串");
        return box_null_typed(VALUE_ARRAY);
    }
    
    const char *dirpath = (const char*)path->data.pointer;
    DIR *dir = opendir(dirpath);
    
    if (!dir) {
        set_runtime_status(FLYUX_IO_ERROR, "Cannot open directory");
        return box_null_typed(VALUE_ARRAY);
    }
    
    // 动态数组存储文件名
    size_t capacity = 16;
    size_t count = 0;
    Value **entries = (Value **)malloc(capacity * sizeof(Value *));
    
    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        // 跳过 "." 和 ".."
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }
        
        // 扩容
        if (count >= capacity) {
            capacity *= 2;
            Value **new_entries = (Value **)realloc(entries, capacity * sizeof(Value *));
            if (!new_entries) {
                for (size_t i = 0; i < count; i++) {
                    free(entries[i]);
                }
                free(entries);
                closedir(dir);
                set_runtime_status(FLYUX_ERROR, "内存分配失败");
                return box_null_typed(VALUE_ARRAY);
            }
            entries = new_entries;
        }
        
        // 复制文件名（entry->d_name会被重用）
        char *name_copy = strdup(entry->d_name);
        entries[count++] = box_string(name_copy);
    }
    
    closedir(dir);
    
    // 创建数组Value
    char *arr_ptr = (char*)entries;
    Value *result = box_array(arr_ptr, count);
    
    set_runtime_status(FLYUX_OK, NULL);
    return result;
}

/* dirExists(path) -> bool - 检查目录是否存在 */
Value* value_dir_exists(Value *path) {
    if (!path || path->type != VALUE_STRING) {
        return box_bool(0);
    }
    
    const char *dirpath = (const char*)path->data.pointer;
    struct stat st;
    
    if (stat(dirpath, &st) == 0 && S_ISDIR(st.st_mode)) {
        return box_bool(1);
    }
    
    return box_bool(0);
}

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
    
    return box_string(strdup(buffer));
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
        set_runtime_status(FLYUX_TYPE_ERROR, "parseJSON: argument must be a string");
        return box_null_typed(VALUE_OBJECT);  // 返回 obj 类型的 null
    }
    
    const char* str = (const char*)json_str->data.pointer;
    const char* ptr = str;
    ptr = skip_whitespace(ptr);
    
    // 检查是否为空或无效起始字符
    if (*ptr == '\0' || (*ptr != '{' && *ptr != '[' && *ptr != '"' &&
        *ptr != 't' && *ptr != 'f' && *ptr != 'n' && *ptr != '-' &&
        !(*ptr >= '0' && *ptr <= '9'))) {
        set_runtime_status(FLYUX_TYPE_ERROR, "parseJSON: invalid JSON format");
        return box_null_typed(VALUE_OBJECT);  // 返回 obj 类型的 null
    }
    
    Value* result = parse_json_value(&ptr);
    
    if (!result) {
        set_runtime_status(FLYUX_TYPE_ERROR, "parseJSON: parse error");
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

/* 前向声明 */
static void serialize_value_to_json(Value* v, char** buffer, size_t* size, size_t* capacity);

/* 将值序列化为 JSON */
static void serialize_value_to_json(Value* v, char** buffer, size_t* size, size_t* capacity) {
    if (!v) {
        append_string(buffer, size, capacity, "null");
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
            append_char(buffer, size, capacity, '[');
            Value** arr = (Value**)v->data.pointer;
            for (long i = 0; i < v->array_size; i++) {
                if (i > 0) append_char(buffer, size, capacity, ',');
                serialize_value_to_json(arr[i], buffer, size, capacity);
            }
            append_char(buffer, size, capacity, ']');
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
                    serialize_value_to_json(entries[i].value, buffer, size, capacity);
                }
                append_char(buffer, size, capacity, '}');
            }
            break;
        }
        default:
            append_string(buffer, size, capacity, "null");
    }
}

/* toJSON(obj) -> str - 将值转换为 JSON 字符串 */
Value* value_to_json(Value* obj) {
    size_t capacity = 256;
    size_t size = 0;
    char* buffer = (char*)malloc(capacity);
    buffer[0] = '\0';
    
    serialize_value_to_json(obj, &buffer, &size, &capacity);
    
    Value* result = box_string(buffer);
    set_runtime_status(FLYUX_OK, NULL);
    return result;
}
