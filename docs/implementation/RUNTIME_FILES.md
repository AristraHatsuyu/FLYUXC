# FLYUX Runtime 文件说明

## 四个 Runtime 文件的作用

```
src/backend/runtime/
└── value_runtime.c          ← 【唯一需要手动编辑的源文件】
    ↓ 编译
src/backend/runtime_object.o       ← 编译后的对象文件
    ↓ xxd
src/backend/runtime_object_embedded.h  ← 嵌入的二进制数组（用于链接）
    
src/backend/runtime_embedded.h     ← 嵌入的源码字符串（Python生成）
```

### 1. value_runtime.c（源文件）✏️
- **路径**: `src/backend/runtime/value_runtime.c`
- **用途**: 运行时函数的 C 源代码实现
- **包含**: 类型定义、boxing/unboxing、print、运算符等
- **修改**: ✅ **在这里修改颜色配置！**

### 2. runtime_object.o（对象文件）⚙️
- **路径**: `src/backend/runtime_object.o`
- **用途**: 从 value_runtime.c 编译生成的机器码
- **生成**: `clang -c -o src/backend/runtime_object.o src/backend/runtime/value_runtime.c`
- **修改**: ❌ 自动生成，不要手动修改

### 3. runtime_object_embedded.h（嵌入二进制）📦
- **路径**: `src/backend/runtime_object_embedded.h`
- **用途**: 将 .o 文件转换为 C 数组，用于编译时链接到 flyuxc
- **格式**: `static const unsigned char runtime_object_o[] = { 0xcf, 0xfa, ... }`
- **生成**: `xxd -i src/backend/runtime_object.o | sed ...`
- **修改**: ❌ 自动生成，不要手动修改

### 4. runtime_embedded.h（嵌入源码）📄
- **路径**: `src/backend/runtime_embedded.h`
- **用途**: 将源码转换为 C 字符串，用于某些需要源码的场景
- **格式**: `"/* Runtime support ... */\n" "..."`
- **生成**: Python 脚本将 value_runtime.c 转换为转义字符串
- **修改**: ❌ 自动生成，不要手动修改

## 修改颜色配置的完整流程

### 第 1 步：修改源文件

编辑 `src/backend/runtime/value_runtime.c`：

```c
/* ANSI color codes for terminal output */
#define ANSI_BLUE        "\033[34m"      /* 数字和布尔值 */
#define ANSI_RED_BROWN   "\033[38;5;131m"  /* 字符串 */
```

找到需要修改颜色的位置：

```c
// 例如：修改数字的颜色
case VALUE_NUMBER:
    if (colors) printf("%s", ANSI_BLUE);  // ← 改成你想要的颜色
    printf("%g", v->data.number);
    if (colors) printf("%s", ANSI_RESET);
```

### 第 2 步：重新生成所有文件

运行以下命令（在项目根目录）：

```bash
# 一键重新生成所有 runtime 文件
cd /Users/userfolder/Desktop/Project/FLYUXC

# 编译对象文件
clang -c -o src/backend/runtime_object.o src/backend/runtime/value_runtime.c

# 生成嵌入的二进制数组
xxd -i src/backend/runtime_object.o | \
  sed 's/src_backend_runtime_object_o/runtime_object_o/g' | \
  sed 's/unsigned char/static const unsigned char/' > \
  src/backend/runtime_object_embedded.h

# 生成嵌入的源码字符串
python3 << 'EOF'
with open('src/backend/runtime/value_runtime.c', 'r') as f:
    code = f.read()
escaped = code.replace('\\', '\\\\').replace('"', '\\"').replace('\n', '\\n"\n    "')
with open('src/backend/runtime_embedded.h', 'w') as f:
    f.write(f'    "{escaped}"')
print("✅ 生成完成")
EOF

# 重新编译 flyuxc
cmake --build build
```

### 第 3 步：测试

```bash
./build/flyuxc test_js_colors.fx
./test_js_colors
```

## 颜色配置快速参考

### 当前配置（JS 控制台风格）

| 类型 | 颜色 | 代码 | 位置 |
|-----|------|------|------|
| 数字 | 蓝色 | `ANSI_BLUE` | `VALUE_NUMBER` case |
| 字符串 | 浅红褐色 | `ANSI_RED_BROWN` | `VALUE_STRING` case（数组内） |
| 字符串 | 无颜色 | - | `VALUE_STRING` case（直接打印） |
| 布尔值 | 蓝色 | `ANSI_BLUE` | `VALUE_BOOL` case |
| null | 灰色加粗 | `ANSI_BOLD + ANSI_GRAY` | `VALUE_NULL` case |
| 对象 | 青色 | `ANSI_CYAN` | `VALUE_OBJECT` case |

### 修改示例

**想让所有数字显示为红色？**

```c
// 找到所有 VALUE_NUMBER 的 case
case VALUE_NUMBER:
    if (colors) printf("%s", ANSI_RED);  // 改这里
    printf("%g", v->data.number);
    if (colors) printf("%s", ANSI_RESET);
```

**想让字符串显示为绿色？**

```c
// 在数组打印函数中找到 VALUE_STRING
case VALUE_STRING:
    if (colors) printf("%s", ANSI_GREEN);  // 改这里
    printf("\"");
    // ... 字符串内容 ...
```

## 常见 ANSI 颜色代码

### 基础 16 色
```c
#define ANSI_BLACK   "\033[30m"
#define ANSI_RED     "\033[31m"
#define ANSI_GREEN   "\033[32m"
#define ANSI_YELLOW  "\033[33m"
#define ANSI_BLUE    "\033[34m"
#define ANSI_MAGENTA "\033[35m"
#define ANSI_CYAN    "\033[36m"
#define ANSI_WHITE   "\033[37m"

// 亮色版本（加 90）
#define ANSI_BRIGHT_BLACK   "\033[90m"
#define ANSI_BRIGHT_RED     "\033[91m"
#define ANSI_BRIGHT_GREEN   "\033[92m"
#define ANSI_BRIGHT_YELLOW  "\033[93m"
#define ANSI_BRIGHT_BLUE    "\033[94m"
#define ANSI_BRIGHT_MAGENTA "\033[95m"
#define ANSI_BRIGHT_CYAN    "\033[96m"
#define ANSI_BRIGHT_WHITE   "\033[97m"
```

### 256 色（更多颜色选择）
```c
// 格式: \033[38;5;NUMm（前景色）或 \033[48;5;NUMm（背景色）
#define ANSI_ORANGE      "\033[38;5;208m"
#define ANSI_PINK        "\033[38;5;205m"
#define ANSI_PURPLE      "\033[38;5;93m"
#define ANSI_RED_BROWN   "\033[38;5;131m"  // 当前字符串颜色
#define ANSI_DARK_GREEN  "\033[38;5;22m"
```

查看所有 256 色：https://misc.flogisoft.com/bash/tip_colors_and_formatting

## 故障排查

### Q: 修改后没有效果？
A: 确保执行了完整的 3 步流程（编辑源文件 → 重新生成 → 重新编译）

### Q: 颜色在某些终端不显示？
A: 检查 TTY 检测是否生效：
```bash
# 直接运行（应该有颜色）
./test_js_colors

# 管道运行（应该无颜色）
./test_js_colors | cat
```

### Q: 编译失败？
A: 检查 ANSI 代码语法：
- 确保字符串用双引号：`"\033[34m"`
- 确保有转义符：`\033` 不是 `033`
- 确保有闭合的 `m`：`\033[34m` 不是 `\033[34`
