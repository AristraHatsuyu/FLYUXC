# `!` 后缀操作符实现进度

## 功能需求

用户希望通过 `!` 后缀控制内置函数的错误处理行为：

- **无 `!`**：`parseJSON("invalid")` → 返回带类型的 null（`obj` 类型，值为 null），不抛出异常
- **有 `!`**：`parseJSON("invalid")!` → 抛出异常，可被 `T>{}(err){}` 捕获，无 try-catch 则终止程序

## 实现状态

### ✅ 已完成（Phase 1）

1. **AST 支持**：
   - `ASTCallExpr` 添加 `throw_on_error` 字段
   - `ast_call_expr_create()` 接受 `throw_on_error` 参数

2. **Parser 支持**：
   - 在 `parse_primary()` 中检测 `!` 后缀（`TK_BANG`）
   - 在 `parse_postfix()` 的链式调用中检测 `!` 后缀
   - 正确设置 `throw_on_error = 1` 当检测到 `!`

3. **Runtime 支持（部分）**：
   - `parseJSON()` 在解析失败时返回 `box_null_typed(VALUE_OBJECT)`
   - 返回的 null 有正确的 `declared_type`，`typeOf()` 显示为 `obj`
   - 错误检测改进：检查 JSON 起始字符是否有效

### ⏳ 待实现（Phase 2）

1. **Codegen 支持**：
   - 检查 `call->throw_on_error` 标志
   - 生成不同的错误处理代码：
     ```c
     if (throw_on_error == 0) {
         // 调用后检查错误状态
         // 如果有错误，清除错误并保留带类型的 null
     } else {
         // 不做特殊处理，让错误传播到 try-catch
     }
     ```

2. **Runtime 完整支持**：
   - 其他可能失败的函数也返回带类型的 null：
     - `toNum()` → `num` 类型的 null
     - `readFile()` → `str` 类型的 null
     - `readBytes()` → `obj:Buffer` 类型的 null
   - 确保所有函数在失败时设置 `runtime_status`

3. **Try-Catch 集成**：
   - 确保带 `!` 的函数调用能被 `T>{}(err){}` 捕获
   - 错误对象包含正确的错误信息

## 测试结果

### 当前行为（无 `!` 时）

```flyux
invalid := parseJSON("invalid json")
println("类型:", typeOf(invalid))  // 输出: obj ✅
println("值:", invalid)             // 输出: null ✅
```

### 期望行为（有 `!` 时）

```flyux
T> {
    invalid := parseJSON("invalid json")!
    println("不会执行到这里")
} (err) {
    println("捕获错误:", err)  // 应该执行 ⏳ 待实现
}
```

## 技术细节

### 1. `box_null_typed()` 函数

```c
Value* box_null_typed(int decl_type) {
    Value *v = (Value*)malloc(sizeof(Value));
    v->type = VALUE_NULL;
    v->declared_type = decl_type;  // 关键：保留声明类型
    v->ext_type = EXT_TYPE_NONE;
    return v;
}
```

### 2. Parser 检测 `!`

```c
// 在 parse_primary() 中
if (!match(p, TK_R_PAREN)) {
    error_at(p, current_token(p), "Expected ')' after arguments");
}

// 检测 ! 后缀
int throw_on_error = 0;
if (match(p, TK_BANG)) {
    throw_on_error = 1;
}

return ast_call_expr_create(id, args, arg_count, throw_on_error, token_to_loc(t));
```

### 3. parseJSON() 错误处理

```c
Value* value_parse_json(Value* json_str) {
    // ... 参数检查 ...
    
    const char* ptr = skip_whitespace(str);
    
    // 检查起始字符是否有效
    if (*ptr == '\0' || (*ptr != '{' && *ptr != '[' && ...)) {
        set_runtime_status(FLYUX_TYPE_ERROR, "parseJSON: 无效的 JSON 格式");
        return box_null_typed(VALUE_OBJECT);  // 返回 obj 类型的 null
    }
    
    Value* result = parse_json_value(&ptr);
    set_runtime_status(FLYUX_OK, NULL);
    return result;
}
```

## 下一步计划

### 优先级 1：Codegen 支持

在 `codegen_expr.c` 的 `AST_CALL_EXPR` 处理中：

```c
case AST_CALL_EXPR: {
    ASTCallExpr *call = (ASTCallExpr *)node->data;
    
    // ... 生成函数调用代码 ...
    
    // 检查 throw_on_error 标志
    if (call->throw_on_error == 0) {
        // 无 ! 后缀：检查错误，清除错误状态
        char *ok_result = new_temp(gen);
        fprintf(gen->code_buf, "  %s = call %%struct.Value* @value_is_ok()\n", ok_result);
        
        char *truthy = new_temp(gen);
        fprintf(gen->code_buf, "  %s = call i32 @value_is_truthy(%%struct.Value* %s)\n", 
                truthy, ok_result);
        
        char *is_ok = new_temp(gen);
        fprintf(gen->code_buf, "  %s = icmp ne i32 %s, 0\n", is_ok, truthy);
        
        // 如果有错误，清除错误状态（结果已经是带类型的 null）
        char *no_error_label = new_label(gen);
        char *clear_error_label = new_label(gen);
        
        fprintf(gen->code_buf, "  br i1 %s, label %%%s, label %%%s\n", 
                is_ok, no_error_label, clear_error_label);
        
        fprintf(gen->code_buf, "%s:\n", clear_error_label);
        fprintf(gen->code_buf, "  call %%struct.Value* @value_clear_error()\n");
        fprintf(gen->code_buf, "  br label %%%s\n", no_error_label);
        
        fprintf(gen->code_buf, "%s:\n", no_error_label);
    }
    // 有 ! 后缀：不做处理，让 try-catch 捕获
}
```

### 优先级 2：扩展到其他函数

修改其他可能失败的内置函数：
- `toNum()` → 返回 `box_null_typed(VALUE_NUMBER)`
- `readFile()` → 返回 `box_null_typed(VALUE_STRING)`  
- `readBytes()` → 返回 `box_null_typed(VALUE_OBJECT)` with `ext_type = EXT_TYPE_BUFFER`

### 优先级 3：测试

创建完整的测试套件：
1. 无 `!` 时返回带类型 null
2. 有 `!` 时 try-catch 能捕获
3. 有 `!` 无 try-catch 时程序终止

## 文件修改清单

### 已修改

- ✅ `include/flyuxc/frontend/ast.h` - 添加 `throw_on_error` 字段
- ✅ `src/frontend/parser/ast.c` - 修改 `ast_call_expr_create()` 实现
- ✅ `src/frontend/parser/parser.c` - 检测 `!` 后缀（2处）
- ✅ `src/backend/runtime/value_runtime.c` - `parseJSON()` 返回带类型 null

### 待修改

- ⏳ `src/backend/codegen/codegen_expr.c` - 根据 `throw_on_error` 生成代码
- ⏳ `src/backend/runtime/value_runtime.c` - 其他函数返回带类型 null

## 兼容性

所有现有代码不受影响：
- 旧代码中的函数调用默认 `throw_on_error = 0`
- 行为变化：错误函数现在返回带类型的 null 而不是普通 null
- 这是向后兼容的改进（类型信息更精确）
