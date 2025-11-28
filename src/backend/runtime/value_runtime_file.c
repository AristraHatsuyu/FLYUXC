/*
 * Auto-generated fragment from value_runtime.c
 * Module: value_runtime_file.c
 */

/* ============================================================================
 * 文件I/O函数实现
 * ============================================================================ */

/* readFile(path) -> string | null - 读取文本文件 */
Value* value_read_file(Value *path) {
    if (!path || path->type != VALUE_STRING) {
        set_runtime_status(FLYUX_TYPE_ERROR, "(readFile) path must be a string");
        return box_null_typed(VALUE_STRING);
    }
    
    const char *filepath = (const char*)path->data.pointer;
    FILE *fp = fopen(filepath, "r");
    
    if (!fp) {
        set_runtime_status(FLYUX_IO_ERROR, "(readFile) cannot open file");
        return box_null_typed(VALUE_STRING);
    }
    
    // 获取文件大小
    fseek(fp, 0, SEEK_END);
    long size = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    
    // 读取文件内容
    char *content = (char*)malloc(size + 1);
    size_t read_size = fread(content, 1, size, fp);
    content[read_size] = '\0';
    fclose(fp);
    
    set_runtime_status(FLYUX_OK, NULL);
    return box_string(content);
}

/* writeFile(path, content) -> bool - 写入文本文件 */
Value* value_write_file(Value *path, Value *content) {
    if (!path || path->type != VALUE_STRING) {
        set_runtime_status(FLYUX_TYPE_ERROR, "(writeFile) path must be a string");
        return box_bool(0);
    }
    if (!content || content->type != VALUE_STRING) {
        set_runtime_status(FLYUX_TYPE_ERROR, "(writeFile) content must be a string");
        return box_bool(0);
    }
    
    const char *filepath = (const char*)path->data.pointer;
    const char *text = (const char*)content->data.pointer;
    
    FILE *fp = fopen(filepath, "w");
    if (!fp) {
        set_runtime_status(FLYUX_IO_ERROR, "(writeFile) cannot create file");
        return box_bool(0);
    }
    
    fputs(text, fp);
    fclose(fp);
    
    set_runtime_status(FLYUX_OK, NULL);
    return box_bool(1);
}

/* appendFile(path, content) -> bool - 追加到文件 */
Value* value_append_file(Value *path, Value *content) {
    if (!path || path->type != VALUE_STRING) {
        set_runtime_status(FLYUX_TYPE_ERROR, "(appendFile) path must be a string");
        return box_bool(0);
    }
    if (!content || content->type != VALUE_STRING) {
        set_runtime_status(FLYUX_TYPE_ERROR, "(appendFile) content must be a string");
        return box_bool(0);
    }
    
    const char *filepath = (const char*)path->data.pointer;
    const char *text = (const char*)content->data.pointer;
    
    FILE *fp = fopen(filepath, "a");
    if (!fp) {
        set_runtime_status(FLYUX_IO_ERROR, "(appendFile) cannot open file");
        return box_bool(0);
    }
    
    fputs(text, fp);
    fclose(fp);
    
    set_runtime_status(FLYUX_OK, NULL);
    return box_bool(1);
}

/* fileExists(path) -> bool - 检查文件是否存在 */
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

/* deleteFile(path) -> bool - 删除文件 */
Value* value_delete_file(Value *path) {
    if (!path || path->type != VALUE_STRING) {
        set_runtime_status(FLYUX_TYPE_ERROR, "(deleteFile) path must be a string");
        return box_bool(0);
    }
    
    const char *filepath = (const char*)path->data.pointer;
    int result = remove(filepath);
    
    if (result == 0) {
        set_runtime_status(FLYUX_OK, NULL);
        return box_bool(1);
    } else {
        set_runtime_status(FLYUX_IO_ERROR, "(deleteFile) failed to delete file");
        return box_bool(0);
    }
}

/* getFileSize(path) -> num - 获取文件大小 */
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

/* readBytes(path) -> Buffer | null - 读取二进制文件 */
Value* value_read_bytes(Value *path) {
    if (!path || path->type != VALUE_STRING) {
        set_runtime_status(FLYUX_TYPE_ERROR, "(readBytes) path must be a string");
        return box_null_typed(VALUE_OBJECT);
    }
    
    const char *filepath = (const char*)path->data.pointer;
    FILE *fp = fopen(filepath, "rb");
    
    if (!fp) {
        set_runtime_status(FLYUX_IO_ERROR, "(readBytes) cannot open file");
        return box_null_typed(VALUE_OBJECT);
    }
    
    // 获取文件大小
    fseek(fp, 0, SEEK_END);
    long size = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    
    // 创建Buffer对象
    BufferObject *buffer = (BufferObject*)malloc(sizeof(BufferObject));
    buffer->data = (unsigned char*)malloc(size);
    buffer->size = size;
    buffer->capacity = size;
    
    size_t read_size = fread(buffer->data, 1, size, fp);
    buffer->size = read_size;
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

/* writeBytes(path, data) -> bool - 写入二进制文件 */
Value* value_write_bytes(Value *path, Value *data) {
    if (!path || path->type != VALUE_STRING) {
        set_runtime_status(FLYUX_TYPE_ERROR, "(writeBytes) path must be a string");
        return box_bool(0);
    }
    
    const char *filepath = (const char*)path->data.pointer;
    FILE *fp = fopen(filepath, "wb");
    
    if (!fp) {
        set_runtime_status(FLYUX_IO_ERROR, "(writeBytes) cannot create file");
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
        set_runtime_status(FLYUX_TYPE_ERROR, "(writeBytes) data must be Buffer or array");
        return box_bool(0);
    }
    
    fclose(fp);
    set_runtime_status(FLYUX_OK, NULL);
    return box_bool(1);
}

/* ============================================================================
 * Phase 1: 核心文件I/O扩展函数
 * ============================================================================ */

/* readLines(path) -> str[] - 逐行读取文本文件 */
Value* value_read_lines(Value *path) {
    if (!path || path->type != VALUE_STRING) {
        set_runtime_status(FLYUX_TYPE_ERROR, "(readLines) path must be a string");
        return box_null_typed(VALUE_ARRAY);
    }
    
    const char *filepath = (const char*)path->data.pointer;
    FILE *fp = fopen(filepath, "r");
    
    if (!fp) {
        set_runtime_status(FLYUX_IO_ERROR, "(readLines) cannot open file");
        return box_null_typed(VALUE_ARRAY);
    }
    
    // 动态数组存储行
    size_t capacity = 16;
    size_t count = 0;
    Value **lines = (Value **)malloc(capacity * sizeof(Value *));
    
    char *line = NULL;
    size_t len = 0;
    ssize_t read;
    
    while ((read = getline(&line, &len, fp)) != -1) {
        // 移除换行符
        if (read > 0 && line[read - 1] == '\n') {
            line[read - 1] = '\0';
            read--;
        }
        if (read > 0 && line[read - 1] == '\r') {
            line[read - 1] = '\0';
            read--;
        }
        
        // 扩容
        if (count >= capacity) {
            capacity *= 2;
            Value **new_lines = (Value **)realloc(lines, capacity * sizeof(Value *));
            if (!new_lines) {
                // 清理已分配的内存
                for (size_t i = 0; i < count; i++) {
                    free(lines[i]);
                }
                free(lines);
                free(line);
                fclose(fp);
                set_runtime_status(FLYUX_ERROR, "(readLines) memory allocation failed");
                return box_null_typed(VALUE_ARRAY);
            }
            lines = new_lines;
        }
        
        // 复制行内容（需要strdup因为line会被重用）
        char *line_copy = strdup(line);
        lines[count++] = box_string(line_copy);
    }
    
    free(line);
    fclose(fp);
    
    // 创建数组Value
    char *arr_ptr = (char*)lines;
    Value *result = box_array(arr_ptr, count);
    
    set_runtime_status(FLYUX_OK, NULL);
    return result;
}

/* renameFile(oldPath, newPath) -> bool - 重命名/移动文件 */
Value* value_rename_file(Value *old_path, Value *new_path) {
    if (!old_path || old_path->type != VALUE_STRING) {
        set_runtime_status(FLYUX_TYPE_ERROR, "(renameFile) oldPath must be a string");
        return box_bool(0);
    }
    
    if (!new_path || new_path->type != VALUE_STRING) {
        set_runtime_status(FLYUX_TYPE_ERROR, "(renameFile) newPath must be a string");
        return box_bool(0);
    }
    
    const char *old_filepath = (const char*)old_path->data.pointer;
    const char *new_filepath = (const char*)new_path->data.pointer;
    
    int result = rename(old_filepath, new_filepath);
    
    if (result == 0) {
        set_runtime_status(FLYUX_OK, NULL);
        return box_bool(1);
    } else {
        set_runtime_status(FLYUX_IO_ERROR, "(renameFile) failed to rename file");
        return box_bool(0);
    }
}

/* copyFile(src, dest) -> bool - 复制文件 */
Value* value_copy_file(Value *src_path, Value *dest_path) {
    if (!src_path || src_path->type != VALUE_STRING) {
        set_runtime_status(FLYUX_TYPE_ERROR, "(copyFile) src must be a string");
        return box_bool(0);
    }
    
    if (!dest_path || dest_path->type != VALUE_STRING) {
        set_runtime_status(FLYUX_TYPE_ERROR, "(copyFile) dest must be a string");
        return box_bool(0);
    }
    
    const char *src = (const char*)src_path->data.pointer;
    const char *dest = (const char*)dest_path->data.pointer;
    
    FILE *src_fp = fopen(src, "rb");
    if (!src_fp) {
        set_runtime_status(FLYUX_IO_ERROR, "(copyFile) cannot open source file");
        return box_bool(0);
    }
    
    FILE *dest_fp = fopen(dest, "wb");
    if (!dest_fp) {
        fclose(src_fp);
        set_runtime_status(FLYUX_IO_ERROR, "(copyFile) cannot create destination file");
        return box_bool(0);
    }
    
    // 缓冲区复制
    char buffer[8192];
    size_t bytes;
    
    while ((bytes = fread(buffer, 1, sizeof(buffer), src_fp)) > 0) {
        if (fwrite(buffer, 1, bytes, dest_fp) != bytes) {
            fclose(src_fp);
            fclose(dest_fp);
            set_runtime_status(FLYUX_IO_ERROR, "(copyFile) failed to write to destination file");
            return box_bool(0);
        }
    }
    
    fclose(src_fp);
    fclose(dest_fp);
    
    // 复制文件权限
    struct stat st;
    if (stat(src, &st) == 0) {
        chmod(dest, st.st_mode);
    }
    
    set_runtime_status(FLYUX_OK, NULL);
    return box_bool(1);
}

/* createDir(path) -> bool - 创建目录 */
Value* value_create_dir(Value *path) {
    if (!path || path->type != VALUE_STRING) {
        set_runtime_status(FLYUX_TYPE_ERROR, "(createDir) path must be a string");
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
        if (errno == EEXIST) {
            set_runtime_status(FLYUX_IO_ERROR, "(createDir) directory already exists");
        } else {
            set_runtime_status(FLYUX_IO_ERROR, "(createDir) failed to create directory");
        }
        return box_bool(0);
    }
}

/* removeDir(path) -> bool - 删除空目录 */
Value* value_remove_dir(Value *path) {
    if (!path || path->type != VALUE_STRING) {
        set_runtime_status(FLYUX_TYPE_ERROR, "(removeDir) path must be a string");
        return box_bool(0);
    }
    
    const char *dirpath = (const char*)path->data.pointer;
    
#ifdef _WIN32
    int result = _rmdir(dirpath);
#else
    int result = rmdir(dirpath);
#endif
    
    if (result == 0) {
        set_runtime_status(FLYUX_OK, NULL);
        return box_bool(1);
    } else {
        if (errno == ENOTEMPTY) {
            set_runtime_status(FLYUX_IO_ERROR, "(removeDir) directory not empty");
        } else if (errno == ENOENT) {
            set_runtime_status(FLYUX_IO_ERROR, "(removeDir) directory does not exist");
        } else {
            set_runtime_status(FLYUX_IO_ERROR, "(removeDir) failed to remove directory");
        }
        return box_bool(0);
    }
}

/* listDir(path) -> str[] - 列出目录内容 */
Value* value_list_dir(Value *path) {
    if (!path || path->type != VALUE_STRING) {
        set_runtime_status(FLYUX_TYPE_ERROR, "(listDir) path must be a string");
        return box_null_typed(VALUE_ARRAY);
    }
    
    const char *dirpath = (const char*)path->data.pointer;
    DIR *dir = opendir(dirpath);
    
    if (!dir) {
        set_runtime_status(FLYUX_IO_ERROR, "(listDir) cannot open directory");
        return box_null_typed(VALUE_ARRAY);
    }
    
    // 动态数组存储文件名
    size_t capacity = 16;
    size_t count = 0;
    Value **entries = (Value **)malloc(capacity * sizeof(Value *));
    
    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        // 跳过 "." 和 ".."
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }
        
        // 扩容
        if (count >= capacity) {
            capacity *= 2;
            Value **new_entries = (Value **)realloc(entries, capacity * sizeof(Value *));
            if (!new_entries) {
                for (size_t i = 0; i < count; i++) {
                    free(entries[i]);
                }
                free(entries);
                closedir(dir);
                set_runtime_status(FLYUX_ERROR, "(listDir) memory allocation failed");
                return box_null_typed(VALUE_ARRAY);
            }
            entries = new_entries;
        }
        
        // 复制文件名（entry->d_name会被重用）
        char *name_copy = strdup(entry->d_name);
        entries[count++] = box_string(name_copy);
    }
    
    closedir(dir);
    
    // 创建数组Value
    char *arr_ptr = (char*)entries;
    Value *result = box_array(arr_ptr, count);
    
    set_runtime_status(FLYUX_OK, NULL);
    return result;
}

/* dirExists(path) -> bool - 检查目录是否存在 */
Value* value_dir_exists(Value *path) {
    if (!path || path->type != VALUE_STRING) {
        return box_bool(0);
    }
    
    const char *dirpath = (const char*)path->data.pointer;
    struct stat st;
    
    if (stat(dirpath, &st) == 0 && S_ISDIR(st.st_mode)) {
        return box_bool(1);
    }
    
    return box_bool(0);
}

