/* ============================================================================
 * codegen_closure.c - 闭包变量捕获分析
 * ============================================================================
 * 分析匿名函数（lambda）并收集其引用的外部变量。
 * 这些变量需要作为额外参数传递给匿名函数，以实现闭包语义。
 * ============================================================================ */

#include "codegen_internal.h"
#include <stdlib.h>
#include <string.h>

/* ============================================================================
 * CapturedVars 结构管理
 * ============================================================================ */

CapturedVars *captured_vars_create(void) {
    CapturedVars *cv = (CapturedVars *)malloc(sizeof(CapturedVars));
    cv->names = NULL;
    cv->count = 0;
    cv->capacity = 0;
    return cv;
}

void captured_vars_add(CapturedVars *cv, const char *name) {
    // 检查是否已存在
    if (captured_vars_contains(cv, name)) {
        return;
    }
    
    // 扩展容量
    if (cv->count >= cv->capacity) {
        size_t new_capacity = cv->capacity == 0 ? 8 : cv->capacity * 2;
        cv->names = (char **)realloc(cv->names, new_capacity * sizeof(char *));
        cv->capacity = new_capacity;
    }
    
    cv->names[cv->count++] = strdup(name);
}

int captured_vars_contains(CapturedVars *cv, const char *name) {
    for (size_t i = 0; i < cv->count; i++) {
        if (strcmp(cv->names[i], name) == 0) {
            return 1;
        }
    }
    return 0;
}

void captured_vars_free(CapturedVars *cv) {
    if (cv) {
        for (size_t i = 0; i < cv->count; i++) {
            free(cv->names[i]);
        }
        free(cv->names);
        free(cv);
    }
}

/* ============================================================================
 * 辅助函数：检查标识符是否是函数参数
 * ============================================================================ */

static int is_param(const char *name, char **params, size_t param_count) {
    for (size_t i = 0; i < param_count; i++) {
        if (strcmp(name, params[i]) == 0) {
            return 1;
        }
    }
    return 0;
}

/* ============================================================================
 * 递归收集外部变量引用
 * ============================================================================ */

/* 局部变量集合 - 用于跟踪函数体内定义的变量 */
typedef struct LocalVarSet {
    char **names;
    size_t count;
    size_t capacity;
} LocalVarSet;

static LocalVarSet *local_var_set_create(void) {
    LocalVarSet *lvs = (LocalVarSet *)malloc(sizeof(LocalVarSet));
    lvs->names = NULL;
    lvs->count = 0;
    lvs->capacity = 0;
    return lvs;
}

static void local_var_set_add(LocalVarSet *lvs, const char *name) {
    // 检查是否已存在
    for (size_t i = 0; i < lvs->count; i++) {
        if (strcmp(lvs->names[i], name) == 0) {
            return;
        }
    }
    
    // 扩展容量
    if (lvs->count >= lvs->capacity) {
        size_t new_capacity = lvs->capacity == 0 ? 8 : lvs->capacity * 2;
        lvs->names = (char **)realloc(lvs->names, new_capacity * sizeof(char *));
        lvs->capacity = new_capacity;
    }
    
    lvs->names[lvs->count++] = strdup(name);
}

static int local_var_set_contains(LocalVarSet *lvs, const char *name) {
    for (size_t i = 0; i < lvs->count; i++) {
        if (strcmp(lvs->names[i], name) == 0) {
            return 1;
        }
    }
    return 0;
}

static void local_var_set_free(LocalVarSet *lvs) {
    if (lvs) {
        for (size_t i = 0; i < lvs->count; i++) {
            free(lvs->names[i]);
        }
        free(lvs->names);
        free(lvs);
    }
}

/* 内置函数名列表 - 这些不需要捕获 */
static const char *builtin_funcs[] = {
    "print", "println", "len", "type", "str", "num", "bool",
    "push", "pop", "shift", "unshift", "slice", "concat",
    "indexOf", "lastIndexOf", "includes", "join", "split", "reverse", "sort",
    "keys", "values", "entries", "has", "delete", "merge", "clone", "deepClone",
    "map", "filter", "reduce", "forEach", "find", "findIndex", "every", "some",
    "substr", "charAt", "startsWith", "endsWith", "replace", "trim", "upper", "lower",
    "floor", "ceil", "round", "abs", "sqrt", "pow", "random", "min", "max",
    "range", "fill", "flat", "unique",
    "now", "time", "sleep", "date",
    "match", "test", "matchAll",
    "input", "readFile", "writeFile", "appendFile", "exists", "mkdir",
    "isArray", "isObject", "isString", "isNumber", "isBool", "isNull", "isError",
    "isNum", "isStr", "isBl", "isArr", "isObj", "isUndef", "isFunc",
    "toNum", "toStr", "toBl", "toInt", "toFloat",
    "error", "throw",
    "typeof", "assert", "printf",
    NULL
};

static int is_builtin(const char *name) {
    for (int i = 0; builtin_funcs[i] != NULL; i++) {
        if (strcmp(name, builtin_funcs[i]) == 0) {
            return 1;
        }
    }
    return 0;
}

/* 前向声明 */
static void collect_vars_in_node(ASTNode *node, CapturedVars *captured,
                                  LocalVarSet *locals, char **params,
                                  size_t param_count, CodeGen *gen);

/* 全局变量：用于跟踪当前分析的函数是否使用了 self */
static bool current_func_uses_self = false;

/* 递归收集变量引用 */
static void collect_vars_in_node(ASTNode *node, CapturedVars *captured,
                                  LocalVarSet *locals, char **params,
                                  size_t param_count, CodeGen *gen) {
    if (!node) return;
    
    switch (node->kind) {
        case AST_SELF_EXPR: {
            /* 检测到使用了 self 关键字 */
            current_func_uses_self = true;
            break;
        }
        
        case AST_IDENTIFIER: {
            ASTIdentifier *id = (ASTIdentifier *)node->data;
            const char *name = id->name;
            
            // 跳过以下情况：
            // 1. 是函数参数
            // 2. 是函数体内定义的局部变量
            // 3. 是内置函数
            // 4. 是顶层定义的函数名（不需要捕获，直接调用）
            // 
            // 注意：不再检查 is_symbol_defined，因为在嵌套函数分析时，
            // 外层函数的局部变量可能还没有注册到符号表。
            // 所有其他标识符都假定需要从外部捕获。
            // 如果变量实际不存在，会在代码生成阶段报"未定义变量"错误。
            
            if (!is_param(name, params, param_count) &&
                !local_var_set_contains(locals, name) &&
                !is_builtin(name) &&
                !is_function_name(gen, name)) {
                // 这是一个来自父作用域的变量，需要捕获
                captured_vars_add(captured, name);
            }
            break;
        }
        
        case AST_VAR_DECL: {
            ASTVarDecl *var = (ASTVarDecl *)node->data;
            // 先处理初始化表达式（在变量定义之前）
            if (var->init_expr) {
                collect_vars_in_node(var->init_expr, captured, locals, params, param_count, gen);
            }
            // 然后将变量加入局部变量集
            local_var_set_add(locals, var->name);
            break;
        }
        
        case AST_BINARY_EXPR: {
            ASTBinaryExpr *bin = (ASTBinaryExpr *)node->data;
            collect_vars_in_node(bin->left, captured, locals, params, param_count, gen);
            collect_vars_in_node(bin->right, captured, locals, params, param_count, gen);
            break;
        }
        
        case AST_UNARY_EXPR: {
            ASTUnaryExpr *unary = (ASTUnaryExpr *)node->data;
            collect_vars_in_node(unary->operand, captured, locals, params, param_count, gen);
            break;
        }
        
        case AST_CALL_EXPR: {
            ASTCallExpr *call = (ASTCallExpr *)node->data;
            collect_vars_in_node(call->callee, captured, locals, params, param_count, gen);
            for (size_t i = 0; i < call->arg_count; i++) {
                collect_vars_in_node(call->args[i], captured, locals, params, param_count, gen);
            }
            break;
        }
        
        case AST_MEMBER_EXPR: {
            ASTMemberExpr *member = (ASTMemberExpr *)node->data;
            collect_vars_in_node(member->object, captured, locals, params, param_count, gen);
            // 注意：属性名不是变量引用，不需要处理
            break;
        }
        
        case AST_INDEX_EXPR: {
            ASTIndexExpr *index = (ASTIndexExpr *)node->data;
            collect_vars_in_node(index->object, captured, locals, params, param_count, gen);
            collect_vars_in_node(index->index, captured, locals, params, param_count, gen);
            break;
        }
        
        case AST_ARRAY_LITERAL: {
            ASTArrayLiteral *arr = (ASTArrayLiteral *)node->data;
            for (size_t i = 0; i < arr->elem_count; i++) {
                collect_vars_in_node(arr->elements[i], captured, locals, params, param_count, gen);
            }
            break;
        }
        
        case AST_OBJECT_LITERAL: {
            ASTObjectLiteral *obj = (ASTObjectLiteral *)node->data;
            for (size_t i = 0; i < obj->prop_count; i++) {
                collect_vars_in_node(obj->properties[i].value, captured, locals, params, param_count, gen);
            }
            break;
        }
        
        case AST_BLOCK: {
            ASTBlock *block = (ASTBlock *)node->data;
            for (size_t i = 0; i < block->stmt_count; i++) {
                collect_vars_in_node(block->statements[i], captured, locals, params, param_count, gen);
            }
            break;
        }
        
        case AST_RETURN_STMT: {
            ASTReturnStmt *ret = (ASTReturnStmt *)node->data;
            collect_vars_in_node(ret->value, captured, locals, params, param_count, gen);
            break;
        }
        
        case AST_IF_STMT: {
            ASTIfStmt *if_stmt = (ASTIfStmt *)node->data;
            for (size_t i = 0; i < if_stmt->cond_count; i++) {
                collect_vars_in_node(if_stmt->conditions[i], captured, locals, params, param_count, gen);
                collect_vars_in_node(if_stmt->then_blocks[i], captured, locals, params, param_count, gen);
            }
            if (if_stmt->else_block) {
                collect_vars_in_node(if_stmt->else_block, captured, locals, params, param_count, gen);
            }
            break;
        }
        
        case AST_LOOP_STMT: {
            ASTLoopStmt *loop = (ASTLoopStmt *)node->data;
            switch (loop->loop_type) {
                case LOOP_REPEAT:
                    collect_vars_in_node(loop->loop_data.repeat_count, captured, locals, params, param_count, gen);
                    break;
                case LOOP_FOR:
                    collect_vars_in_node(loop->loop_data.for_loop.init, captured, locals, params, param_count, gen);
                    collect_vars_in_node(loop->loop_data.for_loop.condition, captured, locals, params, param_count, gen);
                    collect_vars_in_node(loop->loop_data.for_loop.update, captured, locals, params, param_count, gen);
                    break;
                case LOOP_FOREACH:
                    collect_vars_in_node(loop->loop_data.foreach_loop.iterable, captured, locals, params, param_count, gen);
                    // foreach 的迭代变量是新定义的局部变量
                    if (loop->loop_data.foreach_loop.item_var) {
                        local_var_set_add(locals, loop->loop_data.foreach_loop.item_var);
                    }
                    break;
            }
            collect_vars_in_node(loop->body, captured, locals, params, param_count, gen);
            break;
        }
        
        case AST_ASSIGN_STMT: {
            ASTAssignStmt *assign = (ASTAssignStmt *)node->data;
            collect_vars_in_node(assign->target, captured, locals, params, param_count, gen);
            collect_vars_in_node(assign->value, captured, locals, params, param_count, gen);
            break;
        }
        
        case AST_TRY_STMT: {
            ASTTryStmt *tc = (ASTTryStmt *)node->data;
            collect_vars_in_node(tc->try_block, captured, locals, params, param_count, gen);
            // catch 参数是新定义的局部变量
            if (tc->catch_param) {
                local_var_set_add(locals, tc->catch_param);
            }
            collect_vars_in_node(tc->catch_block, captured, locals, params, param_count, gen);
            if (tc->finally_block) {
                collect_vars_in_node(tc->finally_block, captured, locals, params, param_count, gen);
            }
            break;
        }
        
        case AST_FUNC_DECL: {
            // 嵌套函数定义：函数名是新的局部变量
            ASTFuncDecl *func = (ASTFuncDecl *)node->data;
            local_var_set_add(locals, func->name);
            
            // 重要：必须递归分析嵌套函数的函数体！
            // 如果嵌套函数内部引用了外层作用域的变量，当前函数也需要捕获这些变量。
            // 例如：
            //   outer := (a) {
            //       inner := (b) {
            //           R> (c) { R> a + b + c }  // 匿名函数捕获 a, b
            //       }
            //       R> inner
            //   }
            // 这里 inner 必须捕获 a，即使 inner 自己没有直接使用 a，
            // 因为 inner 内部的匿名函数需要 a，而匿名函数只能从 inner 的作用域获取。
            //
            // 创建临时的 locals 集合，包含嵌套函数的参数作为局部变量
            LocalVarSet *nested_locals = local_var_set_create();
            // 复制当前已知的局部变量
            for (size_t i = 0; i < locals->count; i++) {
                local_var_set_add(nested_locals, locals->names[i]);
            }
            // 添加嵌套函数的参数作为局部变量
            for (size_t i = 0; i < func->param_count; i++) {
                local_var_set_add(nested_locals, func->params[i]);
            }
            // 递归分析嵌套函数体，捕获的变量会添加到当前函数的 captured 列表
            collect_vars_in_node(func->body, captured, nested_locals, params, param_count, gen);
            local_var_set_free(nested_locals);
            break;
        }
        
        case AST_CHAIN_EXPR: {
            ASTChainExpr *chain = (ASTChainExpr *)node->data;
            collect_vars_in_node(chain->object, captured, locals, params, param_count, gen);
            for (size_t i = 0; i < chain->chain_count; i++) {
                for (size_t j = 0; j < chain->chain[i].arg_count; j++) {
                    collect_vars_in_node(chain->chain[i].args[j], captured, locals, params, param_count, gen);
                }
            }
            break;
        }
        
        case AST_EXPR_STMT: {
            ASTExprStmt *expr_stmt = (ASTExprStmt *)node->data;
            collect_vars_in_node(expr_stmt->expr, captured, locals, params, param_count, gen);
            break;
        }
        
        // 字面量不需要处理
        case AST_NUM_LITERAL:
        case AST_STRING_LITERAL:
        case AST_BOOL_LITERAL:
        case AST_NULL_LITERAL:
        case AST_UNDEF_LITERAL:
        case AST_BREAK_STMT:
        case AST_NEXT_STMT:
            break;
            
        default:
            // 其他节点类型暂不处理
            break;
    }
}

/* ============================================================================
 * 公开接口
 * ============================================================================ */

CapturedVars *analyze_captured_vars(CodeGen *gen, ASTNode *func_body,
                                    char **params, size_t param_count) {
    CapturedVars *captured = captured_vars_create();
    LocalVarSet *locals = local_var_set_create();
    
    /* 重置 self 使用标记 */
    current_func_uses_self = false;
    
    // 递归收集变量引用
    collect_vars_in_node(func_body, captured, locals, params, param_count, gen);
    
    // 调试输出
    if (getenv("DEBUG_CLOSURE")) {
        fprintf(stderr, "[DEBUG CLOSURE] analyze_captured_vars: found %zu captured vars\n", captured->count);
        for (size_t i = 0; i < captured->count; i++) {
            fprintf(stderr, "[DEBUG CLOSURE]   - %s\n", captured->names[i]);
        }
    }
    
    // 清理
    local_var_set_free(locals);
    
    return captured;
}

/* 检查最近分析的函数是否使用了 self */
bool closure_analysis_uses_self(void) {
    return current_func_uses_self;
}

/* 复制捕获变量列表 */
CapturedVars *captured_vars_copy(CapturedVars *cv) {
    if (!cv) return NULL;
    
    CapturedVars *copy = captured_vars_create();
    for (size_t i = 0; i < cv->count; i++) {
        captured_vars_add(copy, cv->names[i]);
    }
    return copy;
}

/* 注册闭包映射 - 记录变量存储了哪个闭包函数 */
void register_closure_mapping(CodeGen *gen, const char *var_name, 
                              const char *func_name, CapturedVars *captured) {
    ClosureMapping *mapping = (ClosureMapping *)malloc(sizeof(ClosureMapping));
    mapping->var_name = strdup(var_name);
    mapping->func_name = strdup(func_name);
    mapping->captured = captured_vars_copy(captured);
    mapping->next = gen->closure_mappings;
    gen->closure_mappings = mapping;
}

/* 查找闭包映射 - 检查变量是否存储了闭包 */
ClosureMapping *find_closure_mapping(CodeGen *gen, const char *var_name) {
    ClosureMapping *m = gen->closure_mappings;
    while (m) {
        if (strcmp(m->var_name, var_name) == 0) {
            return m;
        }
        m = m->next;
    }
    return NULL;
}
