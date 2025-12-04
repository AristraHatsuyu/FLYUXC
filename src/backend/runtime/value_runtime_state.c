/*
 * Auto-generated fragment from value_runtime.c
 * Module: value_runtime_state.c
 */

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
#define FLYUX_MATH_ERROR    6

/* 全局运行时状态 */
typedef struct {
    int last_status;        /* 最后一次操作的状态码 */
    char error_msg[256];    /* 错误消息 */
    int error_line;         /* 错误行号（供调试用）*/
    int last_output_was_newline;  /* 最后输出是否以换行符结尾 */
} RuntimeState;

static RuntimeState g_runtime_state = {
    .last_status = FLYUX_OK,
    .error_msg = "",
    .error_line = 0,
    .last_output_was_newline = 1  /* 初始假设为true */
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


