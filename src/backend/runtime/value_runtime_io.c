/*
 * Auto-generated fragment from value_runtime.c
 * Module: value_runtime_io.c
 */

/* ============================================================================
 * 输入输出函数
 * ============================================================================
 */

/* 
 * input(prompt) - 从标准输入读取一行
 * 
 * 参数：
 *   prompt: 提示字符串（可选，可以为 null）
 * 
 * 返回：
 *   成功: 返回输入的字符串（不包含换行符）
 *   失败/EOF: 返回 null，并设置状态码
 * 
 * 状态码：
 *   FLYUX_OK: 读取成功
 *   FLYUX_EOF: 遇到文件结束（Ctrl+D / Ctrl+Z）
 *   FLYUX_IO_ERROR: 读取错误
 * 
 * 示例：
 *   name := input("请输入姓名: ")
 *   if (name == null) {
 *       println("输入已取消")
 *   }
 */
Value* value_input(Value *prompt) {
    // 清除之前的错误状态
    set_runtime_status(FLYUX_OK, NULL);
    
    // 显示提示符（如果提供）
    if (prompt && prompt->type == VALUE_STRING && prompt->data.string) {
        printf("%s", prompt->data.string);
        fflush(stdout);  // 确保提示符立即显示
    }
    
    // 读取输入
    char buffer[4096];  // 支持最多 4KB 的输入
    
    if (fgets(buffer, sizeof(buffer), stdin) == NULL) {
        // 读取失败：可能是 EOF 或错误
        if (feof(stdin)) {
            set_runtime_status(FLYUX_EOF, "(input) End of input (EOF)");
        } else {
            set_runtime_status(FLYUX_IO_ERROR, "(input) Input read error");
        }
        return box_null_typed(VALUE_STRING);
    }
    
    // 移除末尾的换行符
    size_t len = strlen(buffer);
    if (len > 0 && buffer[len - 1] == '\n') {
        buffer[len - 1] = '\0';
        len--;
    }
    // 处理 Windows 的 \r\n
    if (len > 0 && buffer[len - 1] == '\r') {
        buffer[len - 1] = '\0';
        len--;
    }
    
    // 复制字符串并创建 Value
    char *result_str = (char*)malloc(len + 1);
    if (!result_str) {
        set_runtime_status(FLYUX_ERROR, "(input) Memory allocation failed");
        return box_null_typed(VALUE_STRING);
    }
    
    memcpy(result_str, buffer, len + 1);
    
    Value *result = (Value*)malloc(sizeof(Value));
    result->type = VALUE_STRING;
    result->declared_type = VALUE_STRING;
    result->data.string = result_str;
    result->string_length = len;
    
    set_runtime_status(FLYUX_OK, NULL);
    return result;
}

