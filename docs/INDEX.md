# FLYUXC 文档索引

> 所有项目文档的总索引，按类别组织

---

## 📚 快速导航

### 🚀 入门指南

- **[README.md](README.md)** - 项目概述和快速开始
- **[QUICKSTART.md](QUICKSTART.md)** - 快速上手教程
- **[FLYUX_SYNTAX.md](FLYUX_SYNTAX.md)** - 语言语法完整参考

---

## 🏗️ 架构文档

### 核心设计

- **[ARCHITECTURE.md](ARCHITECTURE.md)** - 编译器整体架构
- **[PROJECT_STRUCTURE.md](PROJECT_STRUCTURE.md)** - 项目目录结构
- **[OPERATOR_PRECEDENCE.md](OPERATOR_PRECEDENCE.md)** - 运算符优先级设计

### 编译器组件

#### 前端 (Frontend)
- **[LEXER_ANALYSIS.md](LEXER_ANALYSIS.md)** - 词法分析器设计分析
- **[LEXER_COMPLETION_REPORT.md](LEXER_COMPLETION_REPORT.md)** - 词法分析器完成报告
- **[PARSER_DESIGN.md](PARSER_DESIGN.md)** - 语法分析器设计 v1
- **[PARSER_DESIGN_V2.md](PARSER_DESIGN_V2.md)** - 语法分析器设计 v2 (当前版本)
- **[PARSER_IMPLEMENTATION_PLAN.md](PARSER_IMPLEMENTATION_PLAN.md)** - 语法分析器实现计划 v1
- **[PARSER_IMPLEMENTATION_V2.md](PARSER_IMPLEMENTATION_V2.md)** - 语法分析器实现 v2

#### 类型系统
- **[MIXED_TYPE_PLAN.md](MIXED_TYPE_PLAN.md)** - 混合类型系统设计方案
- **[MIXED_TYPE_SUMMARY.md](MIXED_TYPE_SUMMARY.md)** - 混合类型实现总结
- **[MIXED_TYPE_COMPLETE.md](MIXED_TYPE_COMPLETE.md)** - 混合类型完整文档

#### 后端 (Backend)
- **[ARRAY_IMPLEMENTATION.md](ARRAY_IMPLEMENTATION.md)** - 数组实现详解
- **LLVM 集成** - 编译器内嵌 LLVM，生成 155MB 自包含可执行文件

---

## 📈 开发文档

### 进度跟踪

- **[PROGRESS.md](PROGRESS.md)** - 总体开发进度
- **[STATUS.md](STATUS.md)** - 当前开发状态
- **[IMPLEMENTATION_PROGRESS.md](IMPLEMENTATION_PROGRESS.md)** - 实现进度详情

### 重构记录

- **[REFACTORING.md](REFACTORING.md)** - 重构计划和理由
- **[REFACTORING_COMPLETE.md](REFACTORING_COMPLETE.md)** - 重构完成报告

---

## 🧪 测试与验证

- **[PARSER_ACCEPTANCE_CRITERIA.md](PARSER_ACCEPTANCE_CRITERIA.md)** - 语法分析器验收标准
- **[POSITION_VERIFICATION.md](POSITION_VERIFICATION.md)** - 位置信息验证
- **[分号添加逻辑分析.md](分号添加逻辑分析.md)** - 分号自动插入逻辑分析

---

## 🔧 技术细节

### 当前架构特点

#### ✅ LLVM 完全静态链接
- **二进制大小**: 155MB (包含完整 LLVM 工具链)
- **运行时嵌入**: 预编译运行时对象以 C 数组形式嵌入
- **零外部依赖**: 除系统 clang 外不依赖任何外部组件
- **完全可移植**: 单文件分发，拷贝即用

#### 🏗️ 构建系统
```bash
# 构建编译器 (自动处理所有依赖)
cmake -B build
cmake --build build

# 生成 155MB 可执行文件: build/flyuxc
# 运行时已自动内嵌，无需额外文件
```

#### 📦 嵌入式运行时
- **源文件**: `src/backend/runtime/value_runtime.c`
- **预编译**: 构建时自动编译为 `.o` 文件
- **嵌入方式**: 转换为 C 数组存储在 `src/backend/runtime_object_embedded.h`
- **使用**: 编译用户代码时自动链接内嵌运行时

**注意**: 项目根目录不再保留 `value_runtime.o` 文件，运行时对象完全内嵌于编译器中。

---

## 📖 文档维护

### 文档目录结构

```
docs/
├── INDEX.md                     # 本文件 - 文档总索引
├── README.md                    # 项目主文档
├── QUICKSTART.md                # 快速开始
├── FLYUX_SYNTAX.md              # 语法参考
├── ARCHITECTURE.md              # 架构设计
├── PROJECT_STRUCTURE.md         # 目录结构
└── [其他各类文档...]
```

### 文档更新规范

- 新增功能: 更新 `PROGRESS.md` 和 `STATUS.md`
- 架构变更: 更新 `ARCHITECTURE.md` 和本索引
- API 变更: 更新相关组件文档和 `FLYUX_SYNTAX.md`
- 重构完成: 添加记录到 `REFACTORING_COMPLETE.md`

---

## 🔗 快速链接

| 想要... | 查看文档 |
|--------|---------|
| 学习语法 | [FLYUX_SYNTAX.md](FLYUX_SYNTAX.md) |
| 快速上手 | [QUICKSTART.md](QUICKSTART.md) |
| 了解架构 | [ARCHITECTURE.md](ARCHITECTURE.md) |
| 查看进度 | [PROGRESS.md](PROGRESS.md) + [STATUS.md](STATUS.md) |
| 理解类型系统 | [MIXED_TYPE_COMPLETE.md](MIXED_TYPE_COMPLETE.md) |
| 数组实现细节 | [ARRAY_IMPLEMENTATION.md](ARRAY_IMPLEMENTATION.md) |
| 词法分析器 | [LEXER_COMPLETION_REPORT.md](LEXER_COMPLETION_REPORT.md) |
| 语法分析器 | [PARSER_DESIGN_V2.md](PARSER_DESIGN_V2.md) |

---

**最后更新**: 2024-11-18  
**编译器版本**: v0.1.0  
**文档数量**: 24 个
