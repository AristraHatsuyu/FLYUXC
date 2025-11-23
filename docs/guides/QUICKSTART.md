# FLYUX 编译器快速开始指南

## 概述

FLYUX 是一个现代化的编程语言编译器，支持 Unicode/Emoji 变量名，具有类型推断和语义分析能力。

## 系统要求

- **操作系统**: macOS / Linux / Windows (WSL)
- **编译器**: GCC 4.8+ / Clang 3.5+ / MSVC 2015+
- **构建工具**: CMake 3.10+
- **C标准**: C11

## 快速安装

### 1. 克隆项目

```bash
git clone https://github.com/your-org/flyuxc.git
cd flyuxc
```

### 2. 构建编译器

```bash
cmake -B build
cmake --build build
```

**预期输出**:
```
[100%] Linking C executable flyuxc
[100%] Built target flyuxc
```

### 3. 验证安装

```bash
./build/flyuxc --version
```

**预期输出**:
```
FLYUX Compiler v0.1
```

## 第一个程序

### 创建 hello.fx

```flyux
// hello.fx - 你的第一个FLYUX程序

main := () {
    message := "Hello, FLYUX!";
    print(message);
};
```

### 编译并运行

```bash
./build/flyuxc hello.fx
```

**输出** (当前阶段):
```
=== Normalized Source ===
main:=(){message:="Hello, FLYUX!";print(message);};

=== Lexer Tokens ===
IDENT   "main"     1:1+4
DEFINE  ":="       1:5+2
...

=== Compilation Summary ===
✓ Lexical analysis: PASSED
✓ Syntax analysis: PASSED
✓ Semantic analysis: PASSED
Status: READY FOR CODE GENERATION
```

> ⚠️ **注意**: 当前版本仅支持前端编译（词法+语法+语义分析），代码生成功能正在开发中。

## 语言特性示例

### 1. 变量声明

```flyux
// 基本类型
name := "Alice";       // 字符串
age := 25;             // 数字
active := true;        // 布尔值
```

### 2. 类型注解

```flyux
// 显式类型标注
score:<num> := 95;
username:<str> := "Bob";
enabled:<bl> := false;
```

### 3. 函数定义

```flyux
// 带类型标注的函数
add:<num> = (a, b) {
    R> a + b;  // R> 表示 return
};

// 调用函数
result := add(10, 20);
```

### 4. 数组和对象

```flyux
// 数组
fruits := ["apple", "banana", "orange"];
numbers := [1, 2, 3, 4, 5];

// 对象
person := {
    name: "Charlie",
    age: 30,
    hobbies: ["reading", "coding"]
};

// 访问属性
print(person.name);
print(fruits[0]);
```

### 5. 控制流

```flyux
// If 语句
if (age > 18) {
    print("成年人");
} {
    print("未成年");
};

// 循环
L> (i := 0; i < 10; i++) {  // L> 表示 loop
    print(i);
};
```

### 6. Unicode/Emoji 变量名 🎉

FLYUX 的独特特性 - 支持任意 Unicode 字符作为变量名！

```flyux
// Emoji 变量
🚀 := "rocket";
🎯 := 100;
😀 := true;

// 中文变量
姓名 := "张三";
年龄 := 25;

// 日文变量
名前 := "田中";
値 := 42;

// 函数也可以用 Emoji
🤪🫵:<num> = (🐙, 🍄) {
    R> 🐙 + 🍄 * 🐙;
};

main := () {
    result := 🤪🫵(5, 3);  // 5 + 3*5 = 20
    print(result);
};
```

编译器会自动将 Unicode 变量映射为 ASCII 标识符（如 `_00001`），确保兼容性。

## 完整示例

查看 `testfx/demo.fx` 了解完整语法：

```bash
./build/flyuxc testfx/demo.fx
```

这个文件展示了：
- ✅ 类型标注函数
- ✅ 数组和对象字面量
- ✅ 复杂表达式
- ✅ 控制流（if/loop）
- ✅ Emoji 变量名
- ✅ 链式属性访问

## 编译器输出说明

FLYUX 编译器提供详细的分析输出：

### 1. 规范化源码
```
=== Normalized Source ===
```
显示去除注释、合并行后的代码。

### 2. 变量映射表
```
=== Variable Mapping Table ===
[1] 🤪🫵 -> _00001 (UNKNOWN)
```
Unicode/Emoji 变量名的映射关系。

### 3. 词法分析
```
=== Lexer Tokens ===
IDENT   "_00001"        2:1+4
```
Token 列表，包含类型、字面值和位置。

### 4. JSON AST
```json
{
  "kind": "IDENT",
  "lexeme": "_00001",
  "loc": {"line": 2, "column": 1, "orig_length": 4}
}
```
结构化的 Token 表示。

### 5. 语义分析
```
=== AST Construction & Semantic Analysis ===
✓ Function 'main' at line 6:1
✓ Loop statement at line 9:5
```
检测到的语法结构。

### 6. 类型推断
```
=== Type Inference ===
• Literal '3' → type: num (at 7:16)
```
自动推断的类型信息。

### 7. 作用域分析
```
=== Scope Analysis ===
→ Entering scope (depth: 1) at line 2:15
← Leaving scope (depth: 1) at line 4:1
```
作用域层级追踪。

### 8. 编译总结
```
=== Compilation Summary ===
✓ Lexical analysis: PASSED
✓ Syntax analysis: PASSED
✓ Semantic analysis: PASSED
Status: READY FOR CODE GENERATION
```

## 命令行选项

```bash
# 显示版本
./build/flyuxc --version

# 显示帮助
./build/flyuxc --help

# 编译文件
./build/flyuxc <file.fx>

# 即将支持的选项
./build/flyuxc -o output.ll <file.fx>      # 生成 LLVM IR
./build/flyuxc -O2 <file.fx>               # 优化级别
./build/flyuxc --emit-ast <file.fx>        # 仅输出 AST
```

## 项目结构

```
FLYUXC/
├── build/              # 构建输出
│   └── flyuxc          # 编译器可执行文件
├── include/            # 头文件
├── src/                # 源代码
│   ├── frontend/       # 前端（词法/语法/语义）
│   ├── middle/         # 中间层（IR/优化）
│   ├── backend/        # 后端（代码生成）
│   └── utils/          # 工具库
├── testfx/             # 测试用例
├── CMakeLists.txt      # 构建配置
└── *.md                # 文档
```

查看详细文档：
- `PROJECT_STRUCTURE.md` - 完整项目结构说明
- `ARCHITECTURE.md` - 编译器架构设计
- `REFACTORING.md` - 重构过程记录
- `FLYUX_SYNTAX.md` - 语法规范

## 开发任务（VS Code）

项目配置了 VS Code 任务，方便开发：

### 配置项目

```bash
# 方式1: 使用 VS Code 任务
Cmd+Shift+P → Tasks: Run Task → "cmake configure"

# 方式2: 命令行
cmake -B build
```

### 构建项目

```bash
# 方式1: VS Code 默认构建任务
Cmd+Shift+B

# 方式2: 使用任务菜单
Cmd+Shift+P → Tasks: Run Task → "cmake build"

# 方式3: 命令行
cmake --build build
```

### 运行测试

```bash
# 方式1: VS Code 任务
Cmd+Shift+P → Tasks: Run Task → "run"

# 方式2: 命令行
./build/flyuxc testfx/demo.fx
```

## 常见问题

### Q1: 编译失败 "No such file or directory"

**问题**: 找不到头文件

**解决**:
```bash
# 确保在项目根目录
pwd  # 应该是 .../FLYUXC

# 清理并重新构建
rm -rf build
cmake -B build
cmake --build build
```

### Q2: 运行时 "command not found"

**问题**: 可执行文件路径错误

**解决**:
```bash
# 使用完整路径
./build/flyuxc testfx/demo.fx

# 或添加到 PATH
export PATH="$PWD/build:$PATH"
flyuxc testfx/demo.fx
```

### Q3: Unicode 字符显示乱码

**问题**: 终端编码不是 UTF-8

**解决**:
```bash
# macOS/Linux
export LANG=en_US.UTF-8

# 或设置终端编码为 UTF-8
```

### Q4: 代码生成在哪里？

**回答**: 当前版本（v0.1）仅实现了前端：
- ✅ 词法分析
- ✅ 基础语法分析
- ✅ 语义分析
- ⏳ LLVM IR 生成（开发中）
- ⏳ 代码优化（规划中）
- ⏳ 目标代码生成（规划中）

## 贡献指南

欢迎贡献！请查看：
1. Fork 项目
2. 创建特性分支 (`git checkout -b feature/amazing`)
3. 提交更改 (`git commit -m 'Add amazing feature'`)
4. 推送到分支 (`git push origin feature/amazing`)
5. 创建 Pull Request

## 许可证

查看 LICENSE 文件了解详情。

## 获取帮助

- 📖 文档: 查看项目根目录的 `*.md` 文件
- 🐛 Bug 报告: 提交 GitHub Issue
- 💬 讨论: GitHub Discussions
- 📧 邮件: flyux-dev@example.com

---

**开始你的 FLYUX 之旅吧！** 🚀

如有问题，请查看 `ARCHITECTURE.md` 了解编译器内部工作原理。
