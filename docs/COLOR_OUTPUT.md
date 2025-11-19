# FLYUX 彩色终端输出功能

## 概述

FLYUX 编译器现在支持类似 Node.js 的彩色终端输出。当 `print()` 和 `println()` 函数检测到输出是终端（TTY）时，会自动为不同类型的值添加 ANSI 颜色代码。

## 颜色方案（JavaScript 控制台风格）

| 数据类型 | 颜色 | ANSI 代码 | 示例 |
|---------|------|-----------|------|
| 数字 (num) | 蓝色 | `\033[34m` | `42`, `3.14` |
| 字符串 (str) | 浅红褐色 | `\033[38;5;131m` | `"Hello"` |
| 布尔值 (bl) | 蓝色 | `\033[34m` | `true`, `false` |
| null | 灰色加粗 | `\033[1m\033[90m` | `<null>` |
| 数组 | 多色 | - | `[1, "two", true]` |
| 对象 (obj) | 青色 | `\033[36m` | `{...}` |

### 数组的颜色规则

- **括号 `[]`**: 灰色 (`\033[90m`)
- **逗号和空格**: 灰色 (`\033[90m`)
- **数组元素**: 根据元素类型应用对应颜色
  - 数字: 蓝色
  - 字符串: 浅红褐色
  - 布尔值: 蓝色
  - null: 加粗
  - 嵌套数组: 递归应用颜色规则

## TTY 检测

程序在运行时自动检测输出是否为终端：

```c
int should_use_colors() {
    static int use_colors = -1;
    if (use_colors == -1) {
        use_colors = isatty(fileno(stdout));
    }
    return use_colors;
}
```

- ✅ **直接在终端运行**: 显示彩色输出
- ❌ **重定向到文件**: 不添加颜色代码（`./program > output.txt`）
- ❌ **管道到其他命令**: 不添加颜色代码（`./program | cat`）

## 示例

### FLYUX 代码
```flyux
num := 42
str := "Hello"
bool := true
arr := [1, "two", true]

println(num)    // 黄色的 42
println(str)    // 普通的 Hello
println(bool)   // 黄色的 true
println(arr)    // [1, "two", true] 带多种颜色
```

### 终端效果
在支持 ANSI 颜色的终端中运行时：
- 数字 `42` 显示为蓝色
- 字符串 `"Hello"` 显示为浅红褐色（类似 JS 控制台）
- 布尔值 `true` 显示为蓝色
- 数组的括号和逗号为灰色，元素根据类型着色

## 实现细节

### 修改的文件

1. **`src/backend/runtime/value_runtime.c`**
   - 添加 ANSI 颜色常量定义
   - 添加 `should_use_colors()` TTY 检测函数
   - 修改 `value_print()` 函数添加颜色输出
   - 修改 `print_array_json()` 函数为数组元素着色

2. **依赖**
   - `#include <unistd.h>` 用于 `isatty()` 函数

### ANSI 颜色代码表

```c
#define ANSI_RESET       "\033[0m"       // 重置所有属性
#define ANSI_BOLD        "\033[1m"       // 加粗
#define ANSI_DIM         "\033[2m"       // 暗淡
#define ANSI_CYAN        "\033[36m"      // 青色（对象）
#define ANSI_BLUE        "\033[34m"      // 蓝色（数字、布尔值）
#define ANSI_RED_BROWN   "\033[38;5;131m"  // 浅红褐色（字符串）
#define ANSI_GRAY        "\033[90m"      // 灰色（括号、逗号）
```

## 自定义颜色配置

要修改颜色方案，编辑 `src/backend/runtime/value_runtime.c` 文件：

### 1. 修改颜色定义（第 8-18 行）

```c
/* ANSI color codes for terminal output */
#define ANSI_BLUE        "\033[34m"      /* 数字和布尔值 */
#define ANSI_RED_BROWN   "\033[38;5;131m"  /* 字符串 - 浅红褐色 */
// ... 其他颜色定义
```

### 2. 修改类型对应的颜色

在 `value_print()` 和 `print_array_json()` 函数中：

- **数字**: 搜索 `VALUE_NUMBER` → 修改 `ANSI_BLUE`
- **字符串**: 搜索 `VALUE_STRING` → 修改 `ANSI_RED_BROWN`
- **布尔值**: 搜索 `VALUE_BOOL` → 修改 `ANSI_BLUE`

### 3. 重新编译运行时

```bash
# 1. 编译 C 源码为对象文件
clang -c -o src/backend/runtime_object.o src/backend/runtime/value_runtime.c

# 2. 生成嵌入的对象文件
xxd -i src/backend/runtime_object.o | \
  sed 's/src_backend_runtime_object_o/runtime_object_o/g' | \
  sed 's/unsigned char/static const unsigned char/' > \
  src/backend/runtime_object_embedded.h

# 3. 生成嵌入的源码文件
python3 << 'EOF'
with open('src/backend/runtime/value_runtime.c', 'r') as f:
    code = f.read()
escaped = code.replace('\\', '\\\\').replace('"', '\\"').replace('\n', '\\n"\n    "')
with open('src/backend/runtime_embedded.h', 'w') as f:
    f.write(f'    "{escaped}"')
EOF

# 4. 重新编译 flyuxc
cmake --build build
```

### 常用 ANSI 颜色代码

| 颜色 | 基础代码 | 256色代码示例 |
|-----|---------|--------------|
| 黑色 | `\033[30m` | - |
| 红色 | `\033[31m` | `\033[38;5;196m` |
| 绿色 | `\033[32m` | `\033[38;5;46m` |
| 黄色 | `\033[33m` | `\033[38;5;226m` |
| 蓝色 | `\033[34m` | `\033[38;5;21m` |
| 品红 | `\033[35m` | `\033[38;5;201m` |
| 青色 | `\033[36m` | `\033[38;5;51m` |
| 白色 | `\033[37m` | - |
| 浅红褐 | - | `\033[38;5;131m` |
| 橙色 | - | `\033[38;5;208m` |

## 兼容性

- ✅ macOS 终端
- ✅ Linux 终端
- ✅ Windows Terminal
- ✅ VS Code 集成终端
- ✅ iTerm2
- ⚠️ Windows CMD（需要启用 ANSI 支持）

## 测试

运行 `demo_colors.fx` 查看完整的颜色演示：

```bash
./build/flyuxc demo_colors.fx
./demo_colors
```

## 对比 Node.js

FLYUX 的彩色输出完全模仿了 Node.js 的 `console.log()` 行为：

| 特性 | Node.js | FLYUX |
|-----|---------|-------|
| 数字颜色 | 蓝色 | ✅ 蓝色 |
| 字符串颜色 | 浅红褐色 | ✅ 浅红褐色 |
| 布尔值颜色 | 蓝色 | ✅ 蓝色 |
| null 颜色 | 灰色加粗 | ✅ 灰色加粗 |
| 数组括号 | 灰色 | ✅ 灰色 |
| 数组元素字符串 | 浅红褐色 | ✅ 浅红褐色 |
| TTY 检测 | ✅ | ✅ |
| 对象展开 | ✅ 完整 | ⚠️ 简化为 `{...}` |

## 未来改进

- [ ] 完整的对象属性展开和着色
- [ ] 自定义颜色主题支持
- [ ] 强制启用/禁用颜色的命令行选项
- [ ] 支持 `NO_COLOR` 环境变量
