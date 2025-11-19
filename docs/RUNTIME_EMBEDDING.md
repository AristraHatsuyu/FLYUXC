# FLYUX Runtime 嵌入机制说明

## 核心问题解答

### ❓ Runtime 能不能合并进去？
**✅ 已经合并了！Runtime 完全嵌入在 flyuxc 编译器中。**

### ❓ 编译出来的程序还依赖 runtime 吗？
**✅ 完全独立！编译后的可执行文件不依赖任何外部 runtime 文件。**

---

## 工作原理详解

### 1. Runtime 嵌入到编译器

编译 FLYUXC 时，runtime 以两种形式嵌入：

```
构建时（编译 flyuxc 自己）:
  value_runtime.c
    ↓ clang -c
  runtime_object.o
    ↓ xxd -i
  runtime_object_embedded.h  ← 嵌入到 flyuxc 可执行文件中
```

**嵌入位置**: `src/backend/llvm_compiler.cpp`
```cpp
// 嵌入的预编译运行时对象文件
#include "runtime_object_embedded.h"

// runtime_object_embedded.h 包含:
static const unsigned char runtime_object_o[] = {
  0xcf, 0xfa, 0xed, 0xfe, ...  // 完整的 .o 文件二进制
};
static const unsigned int runtime_object_o_len = 12345;
```

### 2. 编译用户程序时的链接过程

当你运行 `./build/flyuxc yourcode.fx` 时：

```
用户程序编译流程:
┌─────────────────────────────────────────────────────┐
│ 1. yourcode.fx                                      │
│    ↓ FLYUXC 前端（词法、语法、语义）                │
│ 2. yourcode.ll (LLVM IR)                           │
│    ↓ LLVM 后端                                     │
│ 3. yourcode.main.o (对象文件)                      │
│    ↓                                               │
│ 4. 从 flyuxc 内存中提取 runtime_object.o          │
│    写入临时文件: /tmp/flyuxc_runtime_12345.o      │
│    ↓                                               │
│ 5. clang -o yourcode yourcode.main.o runtime.o    │
│    (链接成单个可执行文件)                          │
│    ↓                                               │
│ 6. 删除临时文件                                    │
│    ↓                                               │
│ 7. yourcode (完全独立的可执行文件)                 │
└─────────────────────────────────────────────────────┘
```

**关键代码**: `src/backend/llvm_compiler.cpp:280-300`
```cpp
// 使用嵌入的运行时对象文件
std::string embedded_runtime_obj;
if (!runtime_obj || strlen(runtime_obj) == 0) {
    // 从内存中写出 runtime 对象文件到临时位置
    embedded_runtime_obj = write_embedded_runtime_object();
    actual_runtime_obj = embedded_runtime_obj.c_str();
}

// 链接生成可执行文件
link_object_files(temp_main_obj.c_str(), actual_runtime_obj, output_file);

// 清理临时文件
unlink(temp_main_obj.c_str());
unlink(embedded_runtime_obj.c_str());
```

### 3. 编译后程序的独立性验证

#### 验证 1: 查看依赖
```bash
$ otool -L test_js_colors
test_js_colors:
    /usr/lib/libSystem.B.dylib  # 只依赖系统库
```

#### 验证 2: 查看符号表
```bash
$ nm test_js_colors | grep value_print
0000000100002c34 T _value_print      # ✅ 函数已嵌入
0000000100003658 T _value_println    # ✅ 函数已嵌入
```

#### 验证 3: 复制到其他目录运行
```bash
$ cp test_js_colors /tmp/
$ cd /tmp
$ ./test_js_colors
=== JS 风格颜色配置 ===  # ✅ 正常运行
```

---

## Runtime 的三种存在形式

### 在开发时（FLYUXC 源码）

```
src/backend/runtime/
└── value_runtime.c          ← 人类可读的 C 源码
```

### 在编译器中（flyuxc 可执行文件）

```
build/flyuxc 内存中:
  - runtime_object_o[] 数组    ← 嵌入的 .o 文件二进制
  - runtime_object_o_len       ← 大小
```

### 在用户程序中（编译后的 .fx 程序）

```
yourcode (可执行文件):
  - main()                     ← 你的代码
  - value_print()              ← runtime 函数
  - box_number()               ← runtime 函数
  - ... 所有 runtime 函数      ← 全部嵌入
```

---

## 常见问题

### Q1: 为什么需要临时文件？
**A**: 因为 clang 链接器需要 `.o` 文件作为输入。我们：
1. 从 flyuxc 内存中提取嵌入的 runtime 二进制
2. 写入临时 `.o` 文件
3. 调用 `clang -o output main.o runtime.o`
4. 链接完成后立即删除临时文件

这个过程对用户透明，用户看不到临时文件。

### Q2: 能否完全避免临时文件？
**A**: 可以，但需要重写链接逻辑：
- 方案 1: 使用 LLVM 的链接 API（复杂）
- 方案 2: 直接生成完整的可执行文件（需要处理 ELF/Mach-O 格式）
- 当前方案: 使用系统 clang（最简单可靠）

### Q3: 修改 runtime 后需要重新编译用户程序吗？
**A**: 是的，因为：
1. 修改 `value_runtime.c`
2. 运行 `./rebuild_runtime.sh` 重新生成嵌入文件
3. 运行 `cmake --build build` 重新编译 flyuxc
4. 再次编译用户程序时，新的 runtime 会被嵌入

### Q4: 为什么有 4 个 runtime 文件？
**A**: 
- `value_runtime.c` - 源码（手动编辑）
- `runtime_object.o` - 编译后对象（中间产物）
- `runtime_object_embedded.h` - 嵌入到 flyuxc 的二进制数组
- `runtime_embedded.h` - 嵌入到 flyuxc 的源码字符串

后三个都是自动生成的。

### Q5: 能否查看嵌入的 runtime 代码？
**A**: 可以从编译后的程序反汇编：
```bash
# 查看符号
nm yourcode | grep value_

# 反汇编函数
objdump -d yourcode | grep -A 50 'value_print'

# 或使用 Hopper/IDA 等工具
```

---

## 性能考虑

### 优点
✅ **零运行时依赖**: 单个可执行文件，方便分发
✅ **内联优化**: 链接时可以优化 runtime 调用
✅ **快速启动**: 无需加载外部动态库

### 缺点
⚠️ **文件体积**: 每个程序都包含完整 runtime（约 12KB）
⚠️ **更新困难**: 修复 runtime bug 需要重新编译所有程序

### 对比其他语言

| 语言 | Runtime 方式 | 文件大小 |
|-----|-------------|---------|
| Go | 静态链接 | 大（2MB+）|
| Rust | 静态链接 | 中（几百KB）|
| Python | 解释器 + 动态库 | 小（源码几KB）|
| **FLYUX** | **静态链接** | **小（50KB）** |

---

## 分发建议

### 单文件分发 ✅ 推荐
```bash
# 编译
./build/flyuxc myapp.fx

# 分发（只需一个文件）
scp myapp user@server:/usr/local/bin/
```

### 优化体积
```bash
# 启用优化
./build/flyuxc -O3 myapp.fx

# Strip 符号（可选）
strip myapp
```

### 查看最终大小
```bash
$ ls -lh myapp
-rwxr-xr-x  1 user  staff   51K  myapp  # 包含 runtime 的完整程序
```

---

## 总结

✅ **Runtime 已完全嵌入**: 在 `flyuxc` 编译器中
✅ **用户程序完全独立**: 不依赖外部 runtime 文件
✅ **分发简单**: 单个可执行文件即可
✅ **修改 runtime**: 需要重新编译 flyuxc 和用户程序

**FLYUX 采用的是静态链接方式，与 Go/Rust 类似，确保分发的便利性和运行时的独立性。**
