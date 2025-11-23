# FLYUXC 文档索引

> FLYUX 编译器完整文档导航 - 版本 0.1

---

## 📚 快速导航

### 🚀 新手入门

| 文档 | 说明 |
|------|------|
| **[../README.md](../README.md)** | 项目概述、特性介绍、快速开始（中文） |
| **[../README_EN.md](../README_EN.md)** | Project Overview, Features, Quick Start (English) |
| **[guides/QUICKSTART.md](guides/QUICKSTART.md)** | 5 分钟快速上手教程 |
| **[reference/FLYUX_SYNTAX.md](reference/FLYUX_SYNTAX.md)** | 完整语言语法参考手册 |

---

## 🏗️ 架构与设计

### 核心架构

| 文档 | 说明 |
|------|------|
| **[architecture/ARCHITECTURE.md](architecture/ARCHITECTURE.md)** | 编译器整体架构设计 |
| **[architecture/PROJECT_STRUCTURE.md](architecture/PROJECT_STRUCTURE.md)** | 项目目录结构说明 |
| **[architecture/OPERATOR_PRECEDENCE.md](architecture/OPERATOR_PRECEDENCE.md)** | 运算符优先级设计 |

---

## 🔧 实现文档

### 编译器组件

| 文档 | 说明 |
|------|------|
| **[implementation/LEXER_ANALYSIS.md](implementation/LEXER_ANALYSIS.md)** | 词法分析器设计分析 |
| **[implementation/PARSER_DESIGN.md](implementation/PARSER_DESIGN.md)** | 语法分析器设计 |
| **[implementation/PARSER_IMPLEMENTATION.md](implementation/PARSER_IMPLEMENTATION.md)** | 语法分析器实现细节 |

### 数据结构与类型

| 文档 | 说明 |
|------|------|
| **[implementation/ARRAY_IMPLEMENTATION.md](implementation/ARRAY_IMPLEMENTATION.md)** | 数组实现详解 |
| **[implementation/EXTENDED_OBJECT_TYPES.md](implementation/EXTENDED_OBJECT_TYPES.md)** | 扩展对象类型系统设计 |

### 后端与运行时

| 文档 | 说明 |
|------|------|
| **[implementation/RUNTIME_FILES.md](implementation/RUNTIME_FILES.md)** | 运行时文件系统说明 |
| **[implementation/RUNTIME_EMBEDDING.md](implementation/RUNTIME_EMBEDDING.md)** | 运行时内嵌机制 |
| **[implementation/AUTO_BUILD_RUNTIME.md](implementation/AUTO_BUILD_RUNTIME.md)** | 自动构建运行时流程 |

### 质量与警告

| 文档 | 说明 |
|------|------|
| **[implementation/WARNING_SYSTEM.md](implementation/WARNING_SYSTEM.md)** | 警告系统设计 |

### 开发状态

| 文档 | 说明 |
|------|------|
| **[implementation/STATUS.md](implementation/STATUS.md)** | 当前开发状态和功能完成度 |
| **[implementation/PROGRESS.md](implementation/PROGRESS.md)** | 详细开发进度报告 |
| **[implementation/IMPLEMENTATION_PROGRESS.md](implementation/IMPLEMENTATION_PROGRESS.md)** | 功能实现进度追踪 |

---

## 📖 语法参考

| 文档 | 说明 |
|------|------|
| **[reference/FLYUX_SYNTAX.md](reference/FLYUX_SYNTAX.md)** | 完整语法规范 |
| **[reference/BUILTIN_FUNCTIONS_STATUS.md](reference/BUILTIN_FUNCTIONS_STATUS.md)** | 内置函数状态 |

---

## 🔧 技术细节

### 当前版本特性 (v0.1)

#### ✅ 完全静态链接
- **二进制大小**: 76MB (包含完整 LLVM 20.1.6)
- **LLVM 组件**: Core, IRReader, Passes, native (仅本地目标)
- **运行时嵌入**: 预编译运行时对象以 C 数组形式内嵌
- **零 Homebrew 依赖**: 不依赖 `/opt/homebrew` 路径下的任何库
- **系统依赖**: 仅依赖 macOS 系统库
  - `/usr/lib/libz.1.dylib`
  - `/usr/lib/libc++.1.dylib`
  - `/usr/lib/libSystem.B.dylib`
  - `CoreFoundation.framework`

#### 🏗️ 构建系统

```bash
# 完整构建流程
cmake -B build              # 配置 CMake
cmake --build build         # 编译（自动处理运行时内嵌）

# 生成 76MB 可执行文件: build/flyuxc
# - 静态链接所有 LLVM 库
# - 内嵌预编译运行时
# - 可直接拷贝到任何 macOS 系统运行
```

#### 📦 运行时系统

**运行时源文件**:
- `src/backend/runtime/value_runtime.c` - 值类型运行时实现

**构建流程** (自动化):
1. **编译运行时**: `clang -c value_runtime.c -o runtime.o`
2. **转换为数组**: `xxd -i runtime.o > runtime_embedded.h`
3. **嵌入编译器**: 链接时直接包含在二进制中
4. **运行时使用**: 编译用户代码时自动链接内嵌对象

**优势**:
- 单文件分发，无需额外文件
- 启动速度快（~500ms 后续编译）
- 完全可移植，拷贝即用

---

## 📖 文档维护

### 文档分类

- **用户文档**: README, QUICKSTART, FLYUX_SYNTAX
- **架构文档**: ARCHITECTURE, PROJECT_STRUCTURE, OPERATOR_PRECEDENCE
- **实现文档**: LEXER_ANALYSIS, PARSER_*, ARRAY_IMPLEMENTATION, RUNTIME_*
- **状态文档**: STATUS, PROGRESS, IMPLEMENTATION_PROGRESS

---

## 🎯 功能完成度

### ✅ 已完成功能 (100%)

- **编译流程**: 词法分析 → 语法分析 → 代码生成 → 二进制生成
- **语言特性**: 变量、函数、数组、对象、控制流、运算符
- **方法链**: `.>` 操作符（`arr.>len.>println`）
- **内置函数**: `print`, `println`, `len`（统一长度函数）
- **优化**: 静态链接 LLVM，运行时内嵌，快速启动

### 📋 规划功能

- 字符串类型支持
- 标准库扩展
- 错误恢复机制
- LSP 支持（IDE 集成）

---

## 🔗 快速链接

| 想要... | 查看文档 |
|--------|---------|
| 学习语法 | [FLYUX_SYNTAX.md](FLYUX_SYNTAX.md) |
| 快速上手 | [QUICKSTART.md](QUICKSTART.md) |
| 了解架构 | [ARCHITECTURE.md](ARCHITECTURE.md) |
| 查看进度 | [STATUS.md](STATUS.md) + [PROGRESS.md](PROGRESS.md) |
| 数组实现 | [ARRAY_IMPLEMENTATION.md](ARRAY_IMPLEMENTATION.md) |
| 运行时系统 | [RUNTIME_FILES.md](RUNTIME_FILES.md) |
| 词法分析 | [LEXER_ANALYSIS.md](LEXER_ANALYSIS.md) |
| 语法分析 | [PARSER_DESIGN.md](PARSER_DESIGN.md) |

---

**最后更新**: 2025-11-23  
**编译器版本**: v0.1  
**文档数量**: 15 个

---

<div align="center">

**FLYUXC v0.1** - 现代化 Unicode/Emoji 编程语言编译器

[⬆ 回到顶部](#flyuxc-文档索引)

</div>
