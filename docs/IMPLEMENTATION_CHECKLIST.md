# FLYUX 文件I/O与扩展对象实现准备清单

**创建日期**: 2025-11-20  
**状态**: 准备阶段 - 待开始实现

## 📋 实现概览

本项目将为FLYUX添加完整的文件I/O系统和扩展对象类型支持,包括:
- ✅ 设计文档完成 (EXTENDED_OBJECT_TYPES.md + FILE_IO_DESIGN.md)
- ✅ 语法文档更新 (FLYUX_SYNTAX.md v3.0)
- ⏳ 代码实现准备
- ⏳ 测试用例准备

---

## 🎯 实现目标

### 核心功能
1. **扩展对象类型系统**
   - Buffer类型 - 二进制数据容器
   - FileHandle类型 - 文件句柄和流式操作
   - Error类型 - 错误对象
   
2. **文件I/O函数**
   - 文本文件: readFile/writeFile/appendFile
   - 二进制文件: readBytes/writeBytes
   - 流式操作: openFile + FileHandle方法
   - 文件系统: fileExists/deleteFile/getFileSize
   
3. **目录操作**
   - listDir/dirExists/makeDir/removeDir

---

## 📂 需要修改的文件清单

### 1. Runtime Value系统
**文件**: `src/backend/runtime/value_runtime.c`

#### 1.1 Value结构扩展
```c
// 在Value结构中添加扩展类型标识
typedef struct {
    int type;              // VALUE_OBJECT
    int declared_type;     // VALUE_OBJECT
    int ext_type;          // 新增: 扩展类型标识
    union {
        double number;
        char *string;
        void *pointer;     // 指向扩展对象结构
    } data;
    long array_size;
    size_t string_length;
} Value;

// 扩展类型常量
#define EXT_TYPE_NONE      0  // 普通obj
#define EXT_TYPE_BUFFER    1  // Buffer类型
#define EXT_TYPE_FILE      2  // FileHandle类型
#define EXT_TYPE_STREAM    3  // Stream类型
#define EXT_TYPE_ERROR     4  // Error类型
#define EXT_TYPE_DIRECTORY 5  // Directory类型
```

**位置**: 在现有Value定义后添加 (约第130行)

#### 1.2 扩展对象结构定义
```c
// Buffer对象结构
typedef struct {
    char *data;           // 原始二进制数据
    size_t size;          // 数据大小(字节)
    size_t capacity;      // 分配容量
    char *_type_tag;      // 固定为 "Buffer"
} BufferObject;

// FileHandle对象结构
typedef struct {
    FILE *fp;             // C文件指针
    char *path;           // 文件路径
    char *mode;           // 打开模式
    int is_open;          // 是否打开
    long position;        // 当前位置
    char *_type_tag;      // 固定为 "FileHandle"
} FileHandleObject;

// Error对象结构
typedef struct {
    char *message;        // 错误消息
    int code;             // 错误代码
    char *type;           // 错误类型
    char *_type_tag;      // 固定为 "Error"
} ErrorObject;
```

**位置**: 在ObjectEntry定义后添加 (约第140行)

#### 1.3 修改现有函数

**value_typeof() - 扩展类型识别**
```c
char* value_typeof(Value *v) {
    if (!v) return "undef";
    
    // 处理扩展对象类型
    if (v->type == VALUE_OBJECT && v->ext_type != EXT_TYPE_NONE) {
        switch (v->ext_type) {
            case EXT_TYPE_BUFFER: return "Buffer";
            case EXT_TYPE_FILE: return "FileHandle";
            case EXT_TYPE_ERROR: return "Error";
            // ...
        }
    }
    
    // 原有逻辑...
}
```

**位置**: 修改现有value_typeof函数 (约第280行)

**value_print() - 扩展对象安全输出**
```c
void value_print(Value *v) {
    // ...
    case VALUE_OBJECT: {
        // 检查是否为扩展对象类型
        if (v->ext_type != EXT_TYPE_NONE) {
            print_extended_object_meta(v);
        } else {
            // 普通对象,正常输出
            ObjectEntry *entries = (ObjectEntry *)v->data.pointer;
            if (!entries || v->array_size == 0) {
                // 空对象输出 (已修复颜色)
            } else {
                print_object_json_depth(entries, v->array_size, 0);
            }
        }
        break;
    }
}

// 新增: 扩展对象元信息输出
static void print_extended_object_meta(Value *v) {
    int use_colors = should_use_colors();
    const char* type_color = use_colors ? BRACKET_GOLD : "";
    const char* reset = use_colors ? COLOR_RESET : "";
    
    switch (v->ext_type) {
        case EXT_TYPE_BUFFER: {
            BufferObject *buf = (BufferObject*)v->data.pointer;
            printf("%sBuffer%s { size: %zu, type: \"Buffer\" }", 
                   type_color, reset, buf->size);
            break;
        }
        case EXT_TYPE_FILE: {
            FileHandleObject *file = (FileHandleObject*)v->data.pointer;
            printf("%sFileHandle%s { path: \"%s\", mode: \"%s\", position: %ld, isOpen: %s }", 
                   type_color, reset, file->path, file->mode, file->position,
                   file->is_open ? "true" : "false");
            break;
        }
        // ...
    }
}
```

**位置**: 修改value_print函数 (约第570行)

**value_get_field() - 扩展对象属性访问**
```c
Value* value_get_field(Value *obj, Value *key) {
    if (!obj || obj->type != VALUE_OBJECT) return box_null();
    
    const char *field = (const char*)key->data.pointer;
    
    // 处理扩展对象类型的虚拟属性
    if (obj->ext_type == EXT_TYPE_BUFFER) {
        BufferObject *buf = (BufferObject*)obj->data.pointer;
        if (strcmp(field, "size") == 0) {
            return box_number((double)buf->size);
        } else if (strcmp(field, "type") == 0) {
            return box_string("Buffer");
        }
    } else if (obj->ext_type == EXT_TYPE_FILE) {
        FileHandleObject *file = (FileHandleObject*)obj->data.pointer;
        if (strcmp(field, "path") == 0) {
            return box_string(file->path);
        } else if (strcmp(field, "mode") == 0) {
            return box_string(file->mode);
        } else if (strcmp(field, "position") == 0) {
            return box_number((double)file->position);
        } else if (strcmp(field, "isOpen") == 0) {
            return box_bool(file->is_open);
        } else if (strcmp(field, "type") == 0) {
            return box_string("FileHandle");
        }
    }
    
    // 普通对象,正常查找
    ObjectEntry *entries = (ObjectEntry*)obj->data.pointer;
    // ... 原有逻辑
}
```

**位置**: 修改value_get_field函数 (约第1440行)

**value_index() - Buffer索引访问**
```c
Value* value_index(Value *obj, Value *index) {
    if (!obj) return box_null();
    
    // Buffer索引访问
    if (obj->type == VALUE_OBJECT && obj->ext_type == EXT_TYPE_BUFFER) {
        BufferObject *buf = (BufferObject*)obj->data.pointer;
        int idx = (int)unbox_number(index);
        if (idx >= 0 && idx < buf->size) {
            unsigned char byte = (unsigned char)buf->data[idx];
            return box_number((double)byte);
        }
        return box_null();
    }
    
    // 原有逻辑 (数组和对象索引)...
}
```

**位置**: 修改value_index函数 (约第695行)

#### 1.4 新增文件I/O函数

**基础文本文件操作**
```c
// readFile(path) -> string | null
Value* value_read_file(Value *path) {
    if (!path || path->type != VALUE_STRING) {
        set_runtime_status(FLYUX_TYPE_ERROR, "readFile: path必须是字符串");
        return box_null();
    }
    
    const char *filepath = (const char*)path->data.pointer;
    FILE *fp = fopen(filepath, "r");
    
    if (!fp) {
        set_runtime_status(FLYUX_IO_ERROR, "无法打开文件");
        return box_null();
    }
    
    // 获取文件大小
    fseek(fp, 0, SEEK_END);
    long size = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    
    // 读取文件内容
    char *content = (char*)malloc(size + 1);
    fread(content, 1, size, fp);
    content[size] = '\0';
    fclose(fp);
    
    set_runtime_status(FLYUX_OK, NULL);
    return box_string(content);
}

// writeFile(path, content) -> bool
Value* value_write_file(Value *path, Value *content) {
    if (!path || path->type != VALUE_STRING) {
        set_runtime_status(FLYUX_TYPE_ERROR, "writeFile: path必须是字符串");
        return box_bool(0);
    }
    if (!content || content->type != VALUE_STRING) {
        set_runtime_status(FLYUX_TYPE_ERROR, "writeFile: content必须是字符串");
        return box_bool(0);
    }
    
    const char *filepath = (const char*)path->data.pointer;
    const char *text = (const char*)content->data.pointer;
    
    FILE *fp = fopen(filepath, "w");
    if (!fp) {
        set_runtime_status(FLYUX_IO_ERROR, "无法创建文件");
        return box_bool(0);
    }
    
    fputs(text, fp);
    fclose(fp);
    
    set_runtime_status(FLYUX_OK, NULL);
    return box_bool(1);
}

// appendFile(path, content) -> bool
Value* value_append_file(Value *path, Value *content) {
    // 类似writeFile,使用 "a" 模式
}
```

**位置**: 文件末尾添加 (约第2240行后)

**二进制文件操作**
```c
// readBytes(path) -> Buffer | null
Value* value_read_bytes(Value *path) {
    if (!path || path->type != VALUE_STRING) {
        set_runtime_status(FLYUX_TYPE_ERROR, "readBytes: path必须是字符串");
        return box_null();
    }
    
    const char *filepath = (const char*)path->data.pointer;
    FILE *fp = fopen(filepath, "rb");
    
    if (!fp) {
        set_runtime_status(FLYUX_IO_ERROR, "无法打开文件");
        return box_null();
    }
    
    // 获取文件大小
    fseek(fp, 0, SEEK_END);
    long size = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    
    // 创建Buffer对象
    BufferObject *buffer = (BufferObject*)malloc(sizeof(BufferObject));
    buffer->data = (char*)malloc(size);
    buffer->size = size;
    buffer->capacity = size;
    buffer->_type_tag = "Buffer";
    
    fread(buffer->data, 1, size, fp);
    fclose(fp);
    
    // 创建Value
    Value *v = (Value*)malloc(sizeof(Value));
    v->type = VALUE_OBJECT;
    v->declared_type = VALUE_OBJECT;
    v->ext_type = EXT_TYPE_BUFFER;
    v->data.pointer = buffer;
    v->array_size = 0;
    
    set_runtime_status(FLYUX_OK, NULL);
    return v;
}

// writeBytes(path, data) -> bool
Value* value_write_bytes(Value *path, Value *data) {
    if (!path || path->type != VALUE_STRING) {
        set_runtime_status(FLYUX_TYPE_ERROR, "writeBytes: path必须是字符串");
        return box_bool(0);
    }
    
    const char *filepath = (const char*)path->data.pointer;
    FILE *fp = fopen(filepath, "wb");
    
    if (!fp) {
        set_runtime_status(FLYUX_IO_ERROR, "无法创建文件");
        return box_bool(0);
    }
    
    if (data->type == VALUE_OBJECT && data->ext_type == EXT_TYPE_BUFFER) {
        // Buffer对象
        BufferObject *buf = (BufferObject*)data->data.pointer;
        fwrite(buf->data, 1, buf->size, fp);
    } else if (data->type == VALUE_ARRAY) {
        // 数字数组
        Value **arr = (Value **)data->data.pointer;
        for (long i = 0; i < data->array_size; i++) {
            unsigned char byte = (unsigned char)unbox_number(arr[i]);
            fputc(byte, fp);
        }
    } else {
        fclose(fp);
        set_runtime_status(FLYUX_TYPE_ERROR, "writeBytes: data必须是Buffer或数组");
        return box_bool(0);
    }
    
    fclose(fp);
    set_runtime_status(FLYUX_OK, NULL);
    return box_bool(1);
}
```

**文件系统查询**
```c
// fileExists(path) -> bool
Value* value_file_exists(Value *path) {
    if (!path || path->type != VALUE_STRING) {
        return box_bool(0);
    }
    
    const char *filepath = (const char*)path->data.pointer;
    FILE *fp = fopen(filepath, "r");
    
    if (fp) {
        fclose(fp);
        return box_bool(1);
    }
    return box_bool(0);
}

// deleteFile(path) -> bool
Value* value_delete_file(Value *path) {
    if (!path || path->type != VALUE_STRING) {
        set_runtime_status(FLYUX_TYPE_ERROR, "deleteFile: path必须是字符串");
        return box_bool(0);
    }
    
    const char *filepath = (const char*)path->data.pointer;
    int result = remove(filepath);
    
    if (result == 0) {
        set_runtime_status(FLYUX_OK, NULL);
        return box_bool(1);
    } else {
        set_runtime_status(FLYUX_IO_ERROR, "删除文件失败");
        return box_bool(0);
    }
}

// getFileSize(path) -> num
Value* value_get_file_size(Value *path) {
    if (!path || path->type != VALUE_STRING) {
        return box_number(-1);
    }
    
    const char *filepath = (const char*)path->data.pointer;
    FILE *fp = fopen(filepath, "r");
    
    if (!fp) {
        return box_number(-1);
    }
    
    fseek(fp, 0, SEEK_END);
    long size = ftell(fp);
    fclose(fp);
    
    return box_number((double)size);
}
```

**流式文件操作**
```c
// openFile(path, mode) -> FileHandle | null
Value* value_open_file(Value *path, Value *mode) {
    if (!path || path->type != VALUE_STRING) {
        set_runtime_status(FLYUX_TYPE_ERROR, "openFile: path必须是字符串");
        return box_null();
    }
    if (!mode || mode->type != VALUE_STRING) {
        set_runtime_status(FLYUX_TYPE_ERROR, "openFile: mode必须是字符串");
        return box_null();
    }
    
    const char *filepath = (const char*)path->data.pointer;
    const char *open_mode = (const char*)mode->data.pointer;
    
    FILE *fp = fopen(filepath, open_mode);
    if (!fp) {
        set_runtime_status(FLYUX_IO_ERROR, "无法打开文件");
        return box_null();
    }
    
    // 创建FileHandle对象
    FileHandleObject *file = (FileHandleObject*)malloc(sizeof(FileHandleObject));
    file->fp = fp;
    file->path = strdup(filepath);
    file->mode = strdup(open_mode);
    file->is_open = 1;
    file->position = 0;
    file->_type_tag = "FileHandle";
    
    // 创建Value
    Value *v = (Value*)malloc(sizeof(Value));
    v->type = VALUE_OBJECT;
    v->declared_type = VALUE_OBJECT;
    v->ext_type = EXT_TYPE_FILE;
    v->data.pointer = file;
    v->array_size = 0;
    
    set_runtime_status(FLYUX_OK, NULL);
    return v;
}

// FileHandle.readLine() -> string | null
Value* value_file_read_line(Value *handle) {
    if (!handle || handle->type != VALUE_OBJECT || handle->ext_type != EXT_TYPE_FILE) {
        set_runtime_status(FLYUX_TYPE_ERROR, "readLine: 需要FileHandle对象");
        return box_null();
    }
    
    FileHandleObject *file = (FileHandleObject*)handle->data.pointer;
    if (!file->is_open || !file->fp) {
        set_runtime_status(FLYUX_IO_ERROR, "文件未打开");
        return box_null();
    }
    
    char buffer[4096];
    if (fgets(buffer, sizeof(buffer), file->fp)) {
        // 移除行尾换行符
        size_t len = strlen(buffer);
        if (len > 0 && buffer[len-1] == '\n') {
            buffer[len-1] = '\0';
            if (len > 1 && buffer[len-2] == '\r') {
                buffer[len-2] = '\0';
            }
        }
        
        file->position = ftell(file->fp);
        return box_string(strdup(buffer));
    }
    
    return box_null();  // EOF
}

// FileHandle.close() -> bool
Value* value_file_close(Value *handle) {
    if (!handle || handle->type != VALUE_OBJECT || handle->ext_type != EXT_TYPE_FILE) {
        return box_bool(0);
    }
    
    FileHandleObject *file = (FileHandleObject*)handle->data.pointer;
    if (file->is_open && file->fp) {
        fclose(file->fp);
        file->fp = NULL;
        file->is_open = 0;
    }
    
    return box_bool(1);
}
```

**目录操作**
```c
#include <dirent.h>

// listDir(path) -> array<string> | null
Value* value_list_dir(Value *path) {
    if (!path || path->type != VALUE_STRING) {
        set_runtime_status(FLYUX_TYPE_ERROR, "listDir: path必须是字符串");
        return box_null();
    }
    
    const char *dirpath = (const char*)path->data.pointer;
    DIR *dir = opendir(dirpath);
    
    if (!dir) {
        set_runtime_status(FLYUX_IO_ERROR, "无法打开目录");
        return box_null();
    }
    
    // 第一遍: 计数
    int count = 0;
    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) {
            count++;
        }
    }
    
    // 创建数组
    Value **arr = (Value**)malloc(count * sizeof(Value*));
    
    // 第二遍: 填充
    rewinddir(dir);
    int i = 0;
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) {
            arr[i++] = box_string(strdup(entry->d_name));
        }
    }
    
    closedir(dir);
    
    set_runtime_status(FLYUX_OK, NULL);
    return box_array((char*)arr, count);
}

// dirExists(path) -> bool
Value* value_dir_exists(Value *path) {
    if (!path || path->type != VALUE_STRING) {
        return box_bool(0);
    }
    
    const char *dirpath = (const char*)path->data.pointer;
    DIR *dir = opendir(dirpath);
    
    if (dir) {
        closedir(dir);
        return box_bool(1);
    }
    return box_bool(0);
}

// makeDir(path) -> bool
Value* value_make_dir(Value *path) {
    if (!path || path->type != VALUE_STRING) {
        set_runtime_status(FLYUX_TYPE_ERROR, "makeDir: path必须是字符串");
        return box_bool(0);
    }
    
    const char *dirpath = (const char*)path->data.pointer;
    
#ifdef _WIN32
    int result = _mkdir(dirpath);
#else
    int result = mkdir(dirpath, 0755);
#endif
    
    if (result == 0) {
        set_runtime_status(FLYUX_OK, NULL);
        return box_bool(1);
    } else {
        set_runtime_status(FLYUX_IO_ERROR, "创建目录失败");
        return box_bool(0);
    }
}

// removeDir(path) -> bool
Value* value_remove_dir(Value *path) {
    if (!path || path->type != VALUE_STRING) {
        set_runtime_status(FLYUX_TYPE_ERROR, "removeDir: path必须是字符串");
        return box_bool(0);
    }
    
    const char *dirpath = (const char*)path->data.pointer;
    int result = rmdir(dirpath);
    
    if (result == 0) {
        set_runtime_status(FLYUX_OK, NULL);
        return box_bool(1);
    } else {
        set_runtime_status(FLYUX_IO_ERROR, "删除目录失败");
        return box_bool(0);
    }
}
```

**Error对象支持**
```c
// lastErrorObj() -> Error | null
Value* value_last_error_obj() {
    if (g_runtime_state.last_status == FLYUX_OK) {
        return box_null();
    }
    
    // 创建Error对象
    ErrorObject *err = (ErrorObject*)malloc(sizeof(ErrorObject));
    err->message = strdup(g_runtime_state.error_msg);
    err->code = g_runtime_state.last_status;
    
    // 错误类型映射
    switch (g_runtime_state.last_status) {
        case FLYUX_TYPE_ERROR:
            err->type = "TypeError";
            break;
        case FLYUX_IO_ERROR:
            err->type = "IOError";
            break;
        default:
            err->type = "Error";
    }
    err->_type_tag = "Error";
    
    // 创建Value
    Value *v = (Value*)malloc(sizeof(Value));
    v->type = VALUE_OBJECT;
    v->declared_type = VALUE_OBJECT;
    v->ext_type = EXT_TYPE_ERROR;
    v->data.pointer = err;
    v->array_size = 0;
    
    return v;
}
```

---

### 2. 代码生成器更新
**文件**: `src/backend/codegen/codegen.c`

#### 2.1 添加函数声明
在 `codegen_generate()` 函数的运行时函数声明部分添加:

```c
fprintf(gen->output, ";; File I/O functions\n");
fprintf(gen->output, "declare %%struct.Value* @value_read_file(%%struct.Value*)\n");
fprintf(gen->output, "declare %%struct.Value* @value_write_file(%%struct.Value*, %%struct.Value*)\n");
fprintf(gen->output, "declare %%struct.Value* @value_append_file(%%struct.Value*, %%struct.Value*)\n");
fprintf(gen->output, "declare %%struct.Value* @value_read_bytes(%%struct.Value*)\n");
fprintf(gen->output, "declare %%struct.Value* @value_write_bytes(%%struct.Value*, %%struct.Value*)\n");
fprintf(gen->output, "declare %%struct.Value* @value_open_file(%%struct.Value*, %%struct.Value*)\n");
fprintf(gen->output, "declare %%struct.Value* @value_file_exists(%%struct.Value*)\n");
fprintf(gen->output, "declare %%struct.Value* @value_delete_file(%%struct.Value*)\n");
fprintf(gen->output, "declare %%struct.Value* @value_get_file_size(%%struct.Value*)\n\n");

fprintf(gen->output, ";; Directory operations\n");
fprintf(gen->output, "declare %%struct.Value* @value_list_dir(%%struct.Value*)\n");
fprintf(gen->output, "declare %%struct.Value* @value_dir_exists(%%struct.Value*)\n");
fprintf(gen->output, "declare %%struct.Value* @value_make_dir(%%struct.Value*)\n");
fprintf(gen->output, "declare %%struct.Value* @value_remove_dir(%%struct.Value*)\n\n");

fprintf(gen->output, ";; FileHandle methods\n");
fprintf(gen->output, "declare %%struct.Value* @value_file_read_line(%%struct.Value*)\n");
fprintf(gen->output, "declare %%struct.Value* @value_file_close(%%struct.Value*)\n\n");

fprintf(gen->output, ";; Error object\n");
fprintf(gen->output, "declare %%struct.Value* @value_last_error_obj()\n\n");
```

**位置**: 在现有声明后添加 (约第220行附近)

---

### 3. Lexer更新
**文件**: `src/frontend/lexer/varmap.c`

#### 3.1 添加内置函数标识
在内置函数列表中添加:

```c
static const char* builtin_functions[] = {
    // ... 现有函数
    "readFile", "writeFile", "appendFile",
    "readBytes", "writeBytes", 
    "openFile", "fileExists", "deleteFile", "getFileSize",
    "listDir", "dirExists", "makeDir", "removeDir",
    "lastErrorObj",
    NULL
};
```

**位置**: 更新现有builtin_functions数组 (约第79行)

---

### 4. 代码生成 - 函数调用处理
**文件**: `src/backend/codegen/codegen_expr.c`

#### 4.1 添加FileHandle方法调用处理
在函数调用生成中添加对FileHandle方法的支持:

```c
// 在codegen_expr() 的 AST_CALL_EXPR 处理中
// 检查是否为FileHandle方法调用
if (strcmp(call->callee, "readLine") == 0 && call->arg_count == 1) {
    // file.readLine() 形式
    // 生成 value_file_read_line 调用
}
// 类似处理 close(), read(), write() 等
```

**位置**: 修改AST_CALL_EXPR处理逻辑 (需要进一步分析现有代码结构)

---

## 🧪 测试用例准备

### 测试文件结构
```
testfx/file_io/
├── test_text_file.fx          # 文本文件读写
├── test_binary_file.fx         # 二进制文件读写
├── test_file_handle.fx         # 流式操作
├── test_buffer_type.fx         # Buffer类型操作
├── test_error_handling.fx      # 错误处理
├── test_directory.fx           # 目录操作
├── test_extended_print.fx      # 扩展对象输出
└── data/
    ├── sample.txt
    ├── sample.bin
    └── test_image.png
```

### 测试数据准备
需要创建测试数据文件:
1. `sample.txt` - 包含多行文本的测试文件
2. `sample.bin` - 二进制测试文件
3. `test_image.png` - 小型PNG图片(用于测试Buffer)

---

## 📝 实现步骤建议

### Phase 1: 基础设施 (1-2天)
1. ✅ 完成设计文档
2. ✅ 更新语法文档
3. ⏳ 修改Value结构,添加ext_type字段
4. ⏳ 定义扩展对象结构(Buffer/FileHandle/Error)
5. ⏳ 修改value_typeof()支持扩展类型
6. ⏳ 修改value_print()支持元信息输出
7. ⏳ 测试基础设施

### Phase 2: 文本文件I/O (1天)
1. ⏳ 实现readFile()
2. ⏳ 实现writeFile()
3. ⏳ 实现appendFile()
4. ⏳ 实现fileExists()
5. ⏳ 添加codegen声明
6. ⏳ 编写测试用例
7. ⏳ 测试通过

### Phase 3: Buffer和二进制I/O (1-2天)
1. ⏳ 完整实现BufferObject
2. ⏳ 实现readBytes()
3. ⏳ 实现writeBytes()
4. ⏳ 修改value_get_field()支持Buffer属性
5. ⏳ 修改value_index()支持Buffer索引
6. ⏳ 编写测试用例
7. ⏳ 测试通过

### Phase 4: FileHandle流式操作 (1-2天)
1. ⏳ 完整实现FileHandleObject
2. ⏳ 实现openFile()
3. ⏳ 实现readLine()
4. ⏳ 实现close()
5. ⏳ 修改value_get_field()支持FileHandle属性
6. ⏳ 编写测试用例
7. ⏳ 测试通过

### Phase 5: 目录操作 (0.5-1天)
1. ⏳ 实现listDir()
2. ⏳ 实现dirExists()
3. ⏳ 实现makeDir()
4. ⏳ 实现removeDir()
5. ⏳ 编写测试用例
6. ⏳ 测试通过

### Phase 6: 错误对象 (0.5天)
1. ⏳ 实现ErrorObject
2. ⏳ 实现lastErrorObj()
3. ⏳ 集成到文件I/O错误处理
4. ⏳ 测试错误处理

### Phase 7: 集成测试和文档 (0.5-1天)
1. ⏳ 完整的集成测试
2. ⏳ 性能测试(大文件处理)
3. ⏳ 更新BUILTIN_FUNCTIONS_STATUS.md
4. ⏳ 编写使用示例
5. ⏳ 代码审查

---

## ⚠️ 注意事项

### 内存管理
1. **Buffer数据**: 使用malloc分配,需要在适当时机free
2. **FileHandle**: 确保文件关闭时释放资源
3. **字符串复制**: strdup创建的字符串需要释放
4. **垃圾回收**: 考虑是否需要引用计数或GC机制

### 错误处理
1. **文件操作**: 所有文件操作都要检查返回值
2. **路径验证**: 防止路径遍历攻击
3. **权限检查**: 处理权限不足的情况
4. **资源泄漏**: 确保失败路径也释放资源

### 跨平台兼容性
1. **路径分隔符**: Unix用`/`, Windows用`\`
2. **换行符**: Unix用`\n`, Windows用`\r\n`
3. **目录操作**: Windows需要`_mkdir`, Unix用`mkdir`
4. **大小写敏感**: macOS默认不敏感,Linux敏感

### 性能考虑
1. **大文件**: 使用流式处理,避免一次性加载
2. **缓冲区**: 使用合适的缓冲区大小(4KB-64KB)
3. **文件锁**: 考虑是否需要文件锁机制
4. **异步I/O**: 暂不支持,未来可扩展

---

## 📚 参考资料

### C标准库
- `<stdio.h>` - fopen/fread/fwrite/fclose
- `<stdlib.h>` - malloc/free
- `<string.h>` - strcmp/strdup/strlen
- `<dirent.h>` - opendir/readdir/closedir
- `<sys/stat.h>` - mkdir/rmdir

### 设计文档
- `docs/EXTENDED_OBJECT_TYPES.md` - 扩展类型完整设计
- `docs/FILE_IO_DESIGN.md` - 文件I/O详细设计
- `docs/FLYUX_SYNTAX.md` - 语法文档v3.0

---

## ✅ 准备完成检查清单

- [x] 设计文档完成
- [x] 语法文档更新
- [x] API设计评审
- [ ] 测试用例规划
- [ ] 性能基准确定
- [ ] 代码审查流程
- [ ] 文档同步计划

---

**准备状态**: ✅ 设计完成,可以开始实现  
**预计工期**: 5-7天  
**开始日期**: 待定

