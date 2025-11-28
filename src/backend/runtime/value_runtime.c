/*
 * Auto-generated aggregator for FLYUX runtime.
 * 原始实现已拆分为多个模块文件（见同目录的 value_runtime_*.c）。
 * 如果要修改具体实现，请编辑对应模块文件，而不是这个聚合入口。
 */

/* Runtime support functions for FLYUX mixed-type system */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <unistd.h>
#include <stdarg.h>
#include <ctype.h>
#include <time.h>
#include <sys/stat.h>
#include <dirent.h>
#include <errno.h>

#define FLYUXC_VERSION "0.1"

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


#include "value_runtime_state.c"
#include "value_runtime_value.c"
#include "value_runtime_ext.c"
#include "value_runtime_io.c"
#include "value_runtime_state_check.c"
#include "value_runtime_cast.c"
#include "value_runtime_string.c"
#include "value_runtime_array.c"
#include "value_runtime_file.c"
#include "value_runtime_json.c"
#include "value_runtime_math.c"

