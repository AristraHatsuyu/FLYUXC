# FLYUX 编译器项目结构

## 目录组织

```
FLYUXC/
├── include/flyuxc/              # 头文件目录
│   ├── flyuxc.h                 # 主头文件
│   ├── frontend.h               # 前端模块入口
│   ├── utils.h                  # 工具模块入口
│   ├── frontend/                # 前端头文件
│   │   ├── lexer.h              # 词法分析器
│   │   ├── normalize.h          # 代码规范化
│   │   ├── varmap.h             # 变量映射
│   │   ├── parser.h             # 语法分析器
│   │   └── ast.h                # 抽象语法树
│   └── utils/                   # 工具头文件
│       ├── arena.h              # Arena内存分配器
│       ├── string_pool.h        # 字符串池
│       ├── io.h                 # 文件IO
│       └── cli.h                # 命令行接口
│
├── src/                         # 源代码目录
│   ├── main.c                   # 主程序入口
│   │
│   ├── frontend/                # 编译器前端
│   │   ├── lexer/               # 词法分析模块
│   │   │   ├── lexer.c          # 词法分析器实现
│   │   │   ├── normalize.c      # 规范化主逻辑
│   │   │   ├── normalize_comments.c  # 注释处理
│   │   │   ├── normalize_filter.c    # 过滤器
│   │   │   ├── normalize_format.c    # 格式化
│   │   │   ├── normalize_split.c     # 分割处理
│   │   │   └── varmap.c         # 变量名映射
│   │   ├── parser/              # 语法分析模块（待实现）
│   │   └── semantic/            # 语义分析模块（待实现）
│   │
│   ├── middle/                  # 中间表示和优化（待实现）
│   │   ├── ir/                  # 中间表示
│   │   └── optimizer/           # 优化器
│   │
│   ├── backend/                 # 后端代码生成（待实现）
│   │   └── codegen/             # 代码生成器
│   │
│   └── utils/                   # 通用工具模块
│       ├── memory/              # 内存管理
│       │   └── arena.c          # Arena分配器
│       ├── string/              # 字符串处理
│       │   └── string_pool.c    # 字符串池
│       ├── io/                  # 输入输出
│       │   └── io.c             # 文件读写
│       └── cli/                 # 命令行界面
│           └── cli.c            # CLI参数解析
│
├── testfx/                      # 测试文件
│   ├── demo.fx                  # 综合测试
│   ├── simple_obj.fx            # 对象测试
│   ├── types_test.fx            # 类型测试
│   └── ...
│
├── build/                       # 构建输出目录
├── CMakeLists.txt              # CMake配置
└── *.md                        # 文档

## 模块说明

### 前端模块 (Frontend)

#### 词法分析 (Lexer)
- **lexer.c**: 将源代码转换为Token流
- **normalize.c**: 代码规范化主逻辑
- **normalize_*.c**: 各种规范化子模块
- **varmap.c**: Unicode变量名映射到ASCII标识符

#### 语法分析 (Parser)
- 待实现：构建抽象语法树(AST)

#### 语义分析 (Semantic)
- 待实现：类型检查、作用域分析等

### 工具模块 (Utils)

#### 内存管理 (Memory)
- **arena.c**: 高性能Arena内存分配器
  - 64KB初始块，倍增扩展
  - 8字节对齐
  - 零碎片，快速释放

#### 字符串处理 (String)
- **string_pool.c**: 字符串去重池
  - FNV-1a哈希
  - O(1)字符串比较

#### IO处理
- **io.c**: 文件读写操作

#### CLI接口
- **cli.c**: 命令行参数解析

### 中间层 (Middle)
- 待实现：IR生成和优化

### 后端 (Backend)
- 待实现：LLVM IR生成或直接代码生成

## 编译流程

```
源代码 (*.fx)
    ↓
[1] Normalize (规范化)
    ├─ 移除注释
    ├─ 合并多行
    └─ 位置映射
    ↓
[2] VarMap (变量映射)
    ├─ Emoji → ASCII
    └─ Unicode支持
    ↓
[3] Lexer (词法分析)
    ├─ Token生成
    └─ 位置追踪
    ↓
[4] Parser (语法分析)
    ├─ AST构建
    └─ 语法检查
    ↓
[5] Semantic (语义分析)
    ├─ 类型检查
    ├─ 作用域分析
    └─ 错误检测
    ↓
[6] IR Generation (中间表示)
    └─ 优化
    ↓
[7] Code Generation (代码生成)
    └─ 目标代码
```

## 使用方法

### 编译
```bash
cmake -B build
cmake --build build
```

### 运行
```bash
./build/flyuxc testfx/demo.fx
```

### 输出
- 规范化源码
- 变量映射表
- Token列表
- Token AST (JSON)
- 语义分析结果
- 编译总结

## 开发状态

### 已完成 ✅
- 词法分析完整实现
- 代码规范化
- 变量名映射
- Unicode/Emoji支持
- 内存管理(Arena + String Pool)
- JSON AST输出
- 基础语义分析
- 错误检测

### 进行中 🚧
- 完整AST构建
- 高级语义分析

### 计划中 📋
- LLVM IR生成
- 优化器
- 代码生成
- 标准库

## 性能指标

- 解析速度: >100 KB/s
- 内存使用: ~74KB for demo.fx
- 错误恢复: 括号匹配检查
- 位置追踪: 精确到字节

## 代码风格

- C11标准
- 模块化设计
- 清晰的职责分离
- 完整的错误处理
- 详细的注释文档
