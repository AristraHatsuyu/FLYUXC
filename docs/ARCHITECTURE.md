# FLYUX 编译器架构设计

## 架构概览

FLYUXC 采用经典的多阶段编译器架构，分为前端、中间层和后端三大模块：

```
┌─────────────────────────────────────────────────────────────┐
│                    FLYUX 编译器流水线                         │
└─────────────────────────────────────────────────────────────┘

┌──────────────┐    ┌──────────────┐    ┌──────────────┐
│   源代码      │ →  │  规范化处理   │ →  │  变量映射     │
│   (.fx)      │    │  Normalize   │    │  VarMap      │
└──────────────┘    └──────────────┘    └──────────────┘
                           ↓                    ↓
                    移除注释、合并行       Unicode → ASCII
                    
┌──────────────┐    ┌──────────────┐    ┌──────────────┐
│   词法分析    │ →  │  语法分析     │ →  │  语义分析     │
│   Lexer      │    │  Parser      │    │  Semantic    │
└──────────────┘    └──────────────┘    └──────────────┘
       ↓                   ↓                    ↓
   Token流              AST树            类型检查、作用域

┌──────────────┐    ┌──────────────┐    ┌──────────────┐
│  IR 生成     │ →  │  优化器       │ →  │  代码生成     │
│  IR Gen      │    │  Optimizer   │    │  CodeGen     │
└──────────────┘    └──────────────┘    └──────────────┘
       ↓                   ↓                    ↓
   LLVM IR           优化后IR             目标机器码
```

## 模块职责划分

### 前端模块 (Frontend)

前端负责将源代码转换为抽象语法树(AST)，包含三个子模块：

#### 1. 词法分析器 (Lexer)
**位置**: `src/frontend/lexer/`

**职责**:
- 将源代码分解为Token流
- 处理多字节UTF-8字符
- 维护精确的位置信息（行、列、原始长度）
- 支持Emoji变量名

**核心文件**:
- `lexer.c`: 主词法分析逻辑
- `normalize.c`: 代码规范化入口
- `normalize_comments.c`: 移除注释
- `normalize_filter.c`: 过滤无效字符
- `normalize_format.c`: 格式化处理
- `normalize_split.c`: 分割处理
- `varmap.c`: Unicode变量名映射

**数据流**:
```
源代码 → normalize() → VarMap → tokenize() → Token[]
```

**关键数据结构**:
```c
typedef struct {
    TokenKind kind;          // Token类型
    const char* lexeme;      // 字面值
    SourceLocation loc;      // 位置信息
} Token;

typedef struct {
    int line;                // 当前行号
    int column;              // 当前列号
    int orig_line;           // 原始行号
    int orig_column;         // 原始列号
    int orig_length;         // 原始字符长度（字节）
} SourceLocation;
```

#### 2. 语法分析器 (Parser)
**位置**: `src/frontend/parser/` ⏳ 待实现

**职责**:
- 将Token流构建为AST
- 检查语法正确性
- 处理运算符优先级
- 错误恢复

**目标AST节点** (已定义38种):
```c
typedef enum {
    AST_PROGRAM,          // 程序根节点
    AST_FUNC_DECL,        // 函数声明
    AST_VAR_DECL,         // 变量声明
    AST_IF_STMT,          // if语句
    AST_LOOP_STMT,        // loop语句
    AST_RETURN_STMT,      // return语句
    AST_BINARY_EXPR,      // 二元表达式
    AST_UNARY_EXPR,       // 一元表达式
    AST_CALL_EXPR,        // 函数调用
    AST_ARRAY_LITERAL,    // 数组字面量
    AST_OBJECT_LITERAL,   // 对象字面量
    // ... 等38种
} ASTNodeKind;
```

#### 3. 语义分析器 (Semantic)
**位置**: `src/frontend/semantic/` ⏳ 待实现

**职责**:
- 类型检查与推断
- 作用域分析
- 符号表管理
- 类型兼容性检查
- 控制流分析

**当前实现** (在main.c中):
- ✅ 函数声明检测
- ✅ 变量声明统计
- ✅ 控制流语句识别
- ✅ 类型推断（字面量）
- ✅ 作用域深度追踪（最深4层）
- ✅ 括号匹配检查

### 工具模块 (Utils)

提供通用基础设施，被所有模块使用。

#### 1. 内存管理 (Memory)
**位置**: `src/utils/memory/`

**Arena分配器特性**:
```c
typedef struct Arena {
    char* current;           // 当前分配位置
    size_t remaining;        // 剩余空间
    ArenaBlock* blocks;      // 内存块链表
} Arena;

// 性能参数
#define ARENA_BLOCK_SIZE (64 * 1024)  // 64KB初始块
#define ARENA_ALIGNMENT 8              // 8字节对齐
```

**优势**:
- ⚡ O(1)分配速度
- 🗑️ 批量释放（零碎片）
- 📈 自动扩展（倍增策略）
- 🔒 线程安全（单线程优化）

#### 2. 字符串池 (String)
**位置**: `src/utils/string/`

**String Pool特性**:
```c
typedef struct StringPool {
    StringEntry* buckets[STRING_POOL_BUCKETS];  // 4096个桶
    Arena* arena;                                 // 共享Arena
} StringPool;

// 哈希算法: FNV-1a
uint32_t hash = 2166136261u;
for (each byte) {
    hash ^= byte;
    hash *= 16777619u;
}
```

**优势**:
- 🔍 O(1)查找和插入
- 💾 字符串去重（节省内存）
- ⚖️ 指针比较代替strcmp
- 🎯 低冲突率（FNV-1a）

#### 3. IO模块
**位置**: `src/utils/io/`

**职责**:
- 文件读写
- 错误处理
- 编码检测（UTF-8）

#### 4. CLI模块
**位置**: `src/utils/cli/`

**职责**:
- 命令行参数解析
- 帮助信息显示
- 版本信息

### 中间层 (Middle) ⏳ 规划中

**位置**: `src/middle/`

#### IR生成器
- AST → LLVM IR转换
- 类型标注
- 控制流图构建

#### 优化器
- 常量折叠
- 死代码消除
- 循环优化
- 内联优化

### 后端 (Backend) ⏳ 规划中

**位置**: `src/backend/`

#### 代码生成器
- LLVM IR → 机器码
- 寄存器分配
- 指令选择
- 目标平台适配

## 数据流详解

### 完整编译流程

```
1. 源文件读取
   ├─ read_file() → char* source
   └─ UTF-8验证

2. 规范化处理
   ├─ normalize_remove_comments()   // 移除注释
   ├─ normalize_merge_lines()       // 合并多行
   └─ 位置映射表生成

3. 变量映射
   ├─ varmap_scan()                 // 扫描Unicode标识符
   ├─ 🤪🫵 → _00001
   └─ varmap_apply()                // 应用映射

4. 词法分析
   ├─ tokenize() → Token[]
   ├─ Token数量: ~183 (demo.fx)
   └─ 内存消耗: ~8KB

5. 语法分析 ⏳
   ├─ parse() → AST
   └─ 错误恢复

6. 语义分析 ⏳
   ├─ 符号表构建
   ├─ 类型检查
   └─ 作用域验证

7. IR生成 ⏳
   ├─ AST → LLVM IR
   └─ 优化Pass

8. 代码生成 ⏳
   ├─ LLVM → 机器码
   └─ 输出可执行文件
```

### 内存布局

```
┌─────────────────────────────────────┐
│         Arena Memory Layout          │
├─────────────────────────────────────┤
│ Block 1 (64KB)                       │
│ ├─ String Pool                       │
│ │  ├─ "_00001" (7B)                  │
│ │  ├─ "_00002" (7B)                  │
│ │  └─ ...                            │
│ ├─ Normalized Source (2KB)          │
│ ├─ Token Array (8KB)                │
│ └─ VarMap Entries (1KB)             │
├─────────────────────────────────────┤
│ Block 2 (128KB) - 按需分配            │
│ └─ AST Nodes                        │
└─────────────────────────────────────┘
```

## 错误处理策略

### 词法错误
```c
ERROR: Invalid character '`' at line 5, column 12
       some_var := `invalid;
                   ^
```

### 语法错误
```c
ERROR: Expected '}' but got ';' at line 10, column 5
       { x := 5; ;
                 ^
```

### 语义错误
```c
ERROR: Type mismatch at line 15, column 10
       x:<num> := "string";
                  ^^^^^^^^^
       Expected: num
       Got:      str
```

## 性能指标

| 指标 | 目标 | 当前 (demo.fx) |
|------|------|---------------|
| 词法分析速度 | >100 KB/s | ✅ ~4KB/5ms |
| 内存使用 | <1MB/100KB源码 | ✅ 74KB/2KB |
| Token生成 | <10ms/1000行 | ✅ 183 tokens/5ms |
| AST构建 | <20ms/1000行 | ⏳ 待测试 |
| 总编译时间 | <100ms/1000行 | ⏳ 待测试 |

## 扩展性设计

### 模块解耦
- 每个模块有独立的头文件
- 通过聚合头文件(`frontend.h`, `utils.h`)简化引用
- 清晰的接口边界

### 可插拔后端
```c
// 支持多目标
typedef enum {
    TARGET_LLVM_IR,      // LLVM中间表示
    TARGET_C,            // 转译为C
    TARGET_WASM,         // WebAssembly
    TARGET_NATIVE_X64,   // 直接生成x64
} CodeGenTarget;
```

### 测试策略
```
tests/
├── unit/              # 单元测试
│   ├── lexer_test.c
│   ├── parser_test.c
│   └── semantic_test.c
├── integration/       # 集成测试
│   └── e2e_test.c
└── fixtures/          # 测试用例
    ├── valid/
    └── invalid/
```

## 未来路线图

### Phase 1: 完善前端 (当前)
- ✅ 词法分析器
- ✅ 规范化处理
- ✅ 变量映射
- ⏳ 语法分析器（AST构建）
- ⏳ 语义分析器（完整实现）

### Phase 2: 中间层
- ⏳ LLVM IR生成
- ⏳ 类型系统完善
- ⏳ 优化Pass

### Phase 3: 后端
- ⏳ 代码生成
- ⏳ 运行时支持
- ⏳ 标准库

### Phase 4: 高级特性
- ⏳ 泛型支持
- ⏳ 模块系统
- ⏳ 包管理器
- ⏳ LSP支持（IDE集成）

## 依赖关系

```
main.c
  ↓
┌─────────────┐    ┌─────────────┐
│  Frontend   │ ←→ │    Utils    │
│  ┌────────┐ │    │  ┌────────┐ │
│  │ Lexer  │ │    │  │ Arena  │ │
│  │ Parser │ │    │  │ String │ │
│  │Semantic│ │    │  │  Pool  │ │
│  └────────┘ │    │  │   IO   │ │
└─────────────┘    │  │   CLI  │ │
       ↓           │  └────────┘ │
┌─────────────┐    └─────────────┘
│   Middle    │           ↑
│  ┌────────┐ │           │
│  │   IR   │ │───────────┘
│  │Optimize│ │
│  └────────┘ │
└─────────────┘
       ↓
┌─────────────┐
│   Backend   │
│  ┌────────┐ │
│  │CodeGen │ │
│  └────────┘ │
└─────────────┘
```

## 开发准则

1. **模块独立**: 每个模块只依赖utils，不跨越前端/中间层/后端边界
2. **内存安全**: 所有堆分配通过Arena，避免内存泄漏
3. **错误传播**: 使用返回值传递错误，不使用全局状态
4. **文档优先**: 每个模块必须有README说明
5. **测试驱动**: 新功能必须有对应测试用例

---

**版本**: 0.1  
**最后更新**: 2024  
**维护者**: FLYUX Team
