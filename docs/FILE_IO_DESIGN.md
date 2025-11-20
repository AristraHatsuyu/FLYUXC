# FLYUX 文件输入输出系统设计

## 📋 概述

设计一套完整的文件I/O API,支持文本文件读写、二进制文件操作、目录管理等功能。

## 🎯 设计目标

1. **易用性**: 简洁的API,类似JavaScript/Python的文件操作
2. **安全性**: 错误处理机制,避免资源泄漏
3. **完整性**: 涵盖常见文件操作需求
4. **性能**: 高效的文件读写,支持大文件
5. **跨平台**: 兼容Unix/Linux/macOS/Windows

## 📚 核心API设计

### 1. 基础文件读写

#### `readFile(path: string) -> string | null`
读取整个文件内容为字符串

**特性**:
- 返回文件完整内容
- 自动处理UTF-8编码
- 失败返回null并设置lastError()
- 适合中小型文本文件

**示例**:
```flyux
content := readFile("data.txt")
if content != null {
    print("文件内容:", content)
} else {
    print("读取失败:", lastError())
}
```

#### `writeFile(path: string, content: string) -> bool`
写入字符串到文件(覆盖模式)

**特性**:
- 覆盖已存在的文件
- 自动创建不存在的文件
- UTF-8编码写入
- 成功返回true,失败返回false

**示例**:
```flyux
success := writeFile("output.txt", "Hello, FLYUX!")
if success {
    print("写入成功")
} else {
    print("写入失败:", lastError())
}
```

#### `appendFile(path: string, content: string) -> bool`
追加内容到文件末尾

**特性**:
- 保留原有内容
- 文件不存在则创建
- 适合日志写入

**示例**:
```flyux
appendFile("log.txt", "2024-01-15: 系统启动\n")
```

### 2. 行级文件操作

#### `readLines(path: string) -> array<string> | null`
按行读取文件,返回字符串数组

**特性**:
- 自动处理换行符(\n, \r\n)
- 每行作为数组一个元素
- 保留空行
- 去除行尾换行符

**示例**:
```flyux
lines := readLines("config.txt")
if lines != null {
    for i := 0; i < len(lines); i++ {
        print("行", i + 1, ":", lines[i])
    }
}
```

#### `writeLines(path: string, lines: array<string>) -> bool`
将字符串数组按行写入文件

**特性**:
- 每个元素写为一行
- 自动添加换行符
- 覆盖模式

**示例**:
```flyux
lines := ["第一行", "第二行", "第三行"]
writeLines("output.txt", lines)
```

### 3. 二进制文件操作

#### `readBytes(path: string) -> array<number> | null`
读取文件为字节数组

**特性**:
- 支持任意二进制文件
- 每个字节转换为0-255的数字
- 适合图片、音频等二进制数据

**示例**:
```flyux
bytes := readBytes("image.png")
if bytes != null {
    print("文件大小:", len(bytes), "字节")
}
```

#### `writeBytes(path: string, bytes: array<number>) -> bool`
写入字节数组到文件

**特性**:
- 支持二进制数据写入
- 数组元素应为0-255范围
- 超出范围自动截断

**示例**:
```flyux
# 创建简单的位图文件头
header := [0x42, 0x4D, 0x36, 0x00, 0x00, 0x00]
writeBytes("test.bmp", header)
```

### 4. 文件系统操作

#### `fileExists(path: string) -> bool`
检查文件是否存在

**示例**:
```flyux
if fileExists("config.json") {
    content := readFile("config.json")
} else {
    print("配置文件不存在")
}
```

#### `deleteFile(path: string) -> bool`
删除文件

**特性**:
- 成功返回true
- 文件不存在或权限不足返回false
- 设置lastError()

**示例**:
```flyux
if deleteFile("temp.txt") {
    print("删除成功")
}
```

#### `copyFile(src: string, dest: string) -> bool`
复制文件

**示例**:
```flyux
copyFile("data.txt", "backup/data.bak")
```

#### `moveFile(src: string, dest: string) -> bool`
移动/重命名文件

**示例**:
```flyux
moveFile("old_name.txt", "new_name.txt")
```

#### `getFileSize(path: string) -> number`
获取文件大小(字节)

**示例**:
```flyux
size := getFileSize("data.txt")
print("文件大小:", size, "字节")
```

### 5. 目录操作

#### `dirExists(path: string) -> bool`
检查目录是否存在

#### `makeDir(path: string) -> bool`
创建目录

**特性**:
- 仅创建单级目录
- 已存在返回false

**示例**:
```flyux
makeDir("output")
```

#### `makeDirs(path: string) -> bool`
递归创建多级目录

**示例**:
```flyux
makeDirs("data/2024/01/logs")
```

#### `listDir(path: string) -> array<string> | null`
列出目录内容

**特性**:
- 返回文件和子目录名称数组
- 不包含 "." 和 ".."
- 仅返回名称,不含路径

**示例**:
```flyux
files := listDir("./testfx")
if files != null {
    for i := 0; i < len(files); i++ {
        print("文件:", files[i])
    }
}
```

#### `removeDir(path: string) -> bool`
删除空目录

**示例**:
```flyux
removeDir("temp")
```

### 6. 路径操作

#### `joinPath(...parts: string) -> string`
拼接路径

**特性**:
- 自动处理路径分隔符
- 跨平台兼容(Unix用/,Windows用\)
- 移除重复分隔符

**示例**:
```flyux
path := joinPath("data", "2024", "logs", "app.log")
# 结果: "data/2024/logs/app.log" (Unix)
```

#### `baseName(path: string) -> string`
获取文件名(不含路径)

**示例**:
```flyux
name := baseName("/home/user/data.txt")  # "data.txt"
```

#### `dirName(path: string) -> string`
获取目录部分

**示例**:
```flyux
dir := dirName("/home/user/data.txt")  # "/home/user"
```

#### `extName(path: string) -> string`
获取文件扩展名

**示例**:
```flyux
ext := extName("document.pdf")  # ".pdf"
```

#### `absPath(path: string) -> string`
转换为绝对路径

**示例**:
```flyux
abs := absPath("./data.txt")
# 结果: "/Users/user/project/data.txt"
```

## 🔧 实现计划

### Phase 1: 基础文件读写 (优先级: 高)
- ✅ 已在lexer中识别关键字
- [ ] 实现 `readFile()` - C实现使用fopen/fread
- [ ] 实现 `writeFile()` - C实现使用fopen/fwrite
- [ ] 实现 `appendFile()` - 追加模式打开文件
- [ ] 在codegen.c中添加函数声明
- [ ] 编写测试用例

**C函数签名**:
```c
Value* value_read_file(Value *path);
Value* value_write_file(Value *path, Value *content);
Value* value_append_file(Value *path, Value *content);
```

### Phase 2: 文件系统查询 (优先级: 高)
- [ ] 实现 `fileExists()` - stat()或access()
- [ ] 实现 `getFileSize()` - stat()
- [ ] 实现 `deleteFile()` - remove()

**C函数签名**:
```c
Value* value_file_exists(Value *path);
Value* value_get_file_size(Value *path);
Value* value_delete_file(Value *path);
```

### Phase 3: 行级操作 (优先级: 中)
- [ ] 实现 `readLines()` - 逐行解析
- [ ] 实现 `writeLines()` - 遍历数组写入

**C函数签名**:
```c
Value* value_read_lines(Value *path);
Value* value_write_lines(Value *path, Value *lines);
```

### Phase 4: 目录操作 (优先级: 中)
- [ ] 实现 `dirExists()`
- [ ] 实现 `makeDir()` - mkdir()
- [ ] 实现 `listDir()` - opendir/readdir
- [ ] 实现 `removeDir()` - rmdir()

**C函数签名**:
```c
Value* value_dir_exists(Value *path);
Value* value_make_dir(Value *path);
Value* value_list_dir(Value *path);
Value* value_remove_dir(Value *path);
```

### Phase 5: 高级操作 (优先级: 低)
- [ ] 实现 `copyFile()`
- [ ] 实现 `moveFile()` - rename()
- [ ] 实现路径操作函数
- [ ] 实现二进制文件操作

## 🛡️ 错误处理策略

### 统一错误处理
所有文件I/O函数失败时:
1. 返回 `null` 或 `false`
2. 设置 `lastError()` 消息
3. 设置 `lastStatus()` 错误码

### 错误码定义
```c
#define FILE_ERROR_NOT_FOUND     1001  // 文件不存在
#define FILE_ERROR_PERMISSION    1002  // 权限不足
#define FILE_ERROR_IO            1003  // I/O错误
#define FILE_ERROR_INVALID_PATH  1004  // 无效路径
#define FILE_ERROR_IS_DIRECTORY  1005  // 是目录不是文件
#define FILE_ERROR_ALREADY_EXISTS 1006 // 文件已存在
```

### 使用示例
```flyux
content := readFile("missing.txt")
if content == null {
    code := lastStatus()
    if code == 1001 {
        print("文件不存在")
    } else if code == 1002 {
        print("权限不足")
    } else {
        print("其他错误:", lastError())
    }
}
```

## 📊 性能考虑

### 内存管理
- 大文件读取考虑流式处理
- 及时释放文件句柄
- 避免内存泄漏

### 缓冲策略
- 使用系统缓冲区
- 批量写入优化
- 减少系统调用次数

### 建议限制
- 单次读取文件大小 < 100MB
- 行数组元素 < 100,000行
- 超大文件建议分块处理

## 🧪 测试计划

### 单元测试
```flyux
# test_file_io.fx

# 1. 基础读写测试
writeFile("test1.txt", "Hello, World!")
content := readFile("test1.txt")
assert(content == "Hello, World!")

# 2. 追加测试
appendFile("test1.txt", "\nNew Line")
content = readFile("test1.txt")
assert(indexOf(content, "New Line") > 0)

# 3. 不存在文件测试
result := readFile("nonexistent.txt")
assert(result == null)
assert(lastStatus() == 1001)

# 4. 行级操作测试
lines := ["Line 1", "Line 2", "Line 3"]
writeLines("test2.txt", lines)
read_lines := readLines("test2.txt")
assert(len(read_lines) == 3)
assert(read_lines[0] == "Line 1")

# 5. 文件存在测试
assert(fileExists("test1.txt") == true)
assert(fileExists("missing.txt") == false)

# 6. 文件大小测试
size := getFileSize("test1.txt")
assert(size > 0)

# 7. 删除测试
assert(deleteFile("test1.txt") == true)
assert(fileExists("test1.txt") == false)

# 8. 目录测试
assert(makeDir("test_dir") == true)
assert(dirExists("test_dir") == true)
assert(removeDir("test_dir") == true)

print("所有文件I/O测试通过!")
```

## 🔄 与现有系统集成

### 更新内置函数列表
在 `src/frontend/lexer/varmap.c` 中添加:
```c
"readFile", "writeFile", "appendFile", 
"readLines", "writeLines",
"fileExists", "deleteFile", "getFileSize",
"dirExists", "makeDir", "listDir", "removeDir",
"copyFile", "moveFile",
"joinPath", "baseName", "dirName", "extName", "absPath"
```

### 更新codegen声明
在 `src/backend/codegen/codegen.c` 中添加:
```c
fprintf(gen->output, ";; File I/O functions\n");
fprintf(gen->output, "declare %%struct.Value* @value_read_file(%%struct.Value*)\n");
fprintf(gen->output, "declare %%struct.Value* @value_write_file(%%struct.Value*, %%struct.Value*)\n");
// ... 其他声明
```

### 实现runtime函数
在 `src/backend/runtime/value_runtime.c` 中实现所有C函数

## 📝 文档更新

更新以下文档:
- [x] `FILE_IO_DESIGN.md` - 本文档
- [ ] `BUILTIN_FUNCTIONS_STATUS.md` - 添加实现状态
- [ ] `FLYUX_SYNTAX.md` - 更新API文档和示例
- [ ] `README.md` - 添加特性说明

## 🎯 里程碑

### Milestone 1: 基础可用 (1-2天)
- [x] 完成设计文档
- [ ] 实现 readFile/writeFile/appendFile
- [ ] 实现 fileExists/deleteFile
- [ ] 基础测试通过

### Milestone 2: 完整功能 (3-5天)
- [ ] 实现行级操作
- [ ] 实现目录操作
- [ ] 实现文件管理(copy/move)
- [ ] 完整测试套件

### Milestone 3: 优化增强 (可选)
- [ ] 二进制文件支持
- [ ] 路径操作函数
- [ ] 性能优化
- [ ] 跨平台测试

## 💡 使用场景示例

### 场景1: 配置文件读取
```flyux
func loadConfig(path) {
    if !fileExists(path) {
        print("配置文件不存在,使用默认配置")
        return { host: "localhost", port: 8080 }
    }
    
    content := readFile(path)
    # 假设有JSON解析函数
    config := parseJSON(content)
    return config
}

config := loadConfig("config.json")
print("服务器:", config.host, ":", config.port)
```

### 场景2: 日志记录
```flyux
func log(level, message) {
    timestamp := currentTime()  # 假设有时间函数
    entry := timestamp + " [" + level + "] " + message + "\n"
    appendFile("app.log", entry)
}

log("INFO", "应用启动")
log("ERROR", "连接失败")
```

### 场景3: 批量文件处理
```flyux
func processAllFiles(dir) {
    files := listDir(dir)
    if files == null {
        print("无法读取目录")
        return
    }
    
    for i := 0; i < len(files); i++ {
        filename := files[i]
        if endsWith(filename, ".txt") {
            path := joinPath(dir, filename)
            content := readFile(path)
            # 处理内容...
            print("处理:", filename)
        }
    }
}

processAllFiles("./data")
```

### 场景4: 数据导出
```flyux
func exportData(data, filename) {
    lines := []
    push(lines, "ID,Name,Score")
    
    for i := 0; i < len(data); i++ {
        row := data[i]
        line := str(row.id) + "," + row.name + "," + str(row.score)
        push(lines, line)
    }
    
    if writeLines(filename, lines) {
        print("导出成功:", filename)
    } else {
        print("导出失败:", lastError())
    }
}

students := [
    { id: 1, name: "Alice", score: 95 },
    { id: 2, name: "Bob", score: 87 }
]
exportData(students, "students.csv")
```

## 🔗 参考资料

- Node.js fs模块: https://nodejs.org/api/fs.html
- Python pathlib: https://docs.python.org/3/library/pathlib.html
- C stdio库: https://en.cppreference.com/w/c/io
- POSIX文件系统API: https://pubs.opengroup.org/onlinepubs/9699919799/

---

**设计版本**: 1.0  
**创建日期**: 2024-01-15  
**状态**: 待实现  
**负责人**: FLYUX Team
