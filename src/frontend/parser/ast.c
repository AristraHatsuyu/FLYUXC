#include "flyuxc/frontend/ast.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* ============================================================================
 * AST节点创建和销毁
 * ============================================================================ */

ASTNode *ast_node_create(ASTNodeKind kind, SourceLocation loc) {
    ASTNode *node = (ASTNode *)malloc(sizeof(ASTNode));
    if (!node) return NULL;
    
    node->kind = kind;
    node->loc = loc;
    node->data = NULL;
    
    return node;
}

void ast_node_free(ASTNode *node) {
    if (!node) return;
    
    // 递归释放具体节点数据
    switch (node->kind) {
        case AST_PROGRAM: {
            ASTProgram *prog = (ASTProgram *)node->data;
            for (size_t i = 0; i < prog->stmt_count; i++) {
                ast_node_free(prog->statements[i]);
            }
            free(prog->statements);
            free(prog);
            break;
        }
        
        case AST_VAR_DECL: {
            ASTVarDecl *decl = (ASTVarDecl *)node->data;
            free(decl->name);
            ast_node_free(decl->type_annotation);
            ast_node_free(decl->init_expr);
            free(decl);
            break;
        }
        
        case AST_FUNC_DECL: {
            ASTFuncDecl *func = (ASTFuncDecl *)node->data;
            free(func->name);
            for (size_t i = 0; i < func->param_count; i++) {
                free(func->params[i]);
            }
            free(func->params);
            ast_node_free(func->return_type);
            ast_node_free(func->body);
            free(func);
            break;
        }
        
        case AST_BLOCK: {
            ASTBlock *block = (ASTBlock *)node->data;
            for (size_t i = 0; i < block->stmt_count; i++) {
                ast_node_free(block->statements[i]);
            }
            free(block->statements);
            free(block);
            break;
        }
        
        case AST_BINARY_EXPR: {
            ASTBinaryExpr *expr = (ASTBinaryExpr *)node->data;
            ast_node_free(expr->left);
            ast_node_free(expr->right);
            free(expr);
            break;
        }
        
        case AST_TERNARY_EXPR: {
            ASTTernaryExpr *expr = (ASTTernaryExpr *)node->data;
            ast_node_free(expr->condition);
            ast_node_free(expr->true_value);
            ast_node_free(expr->false_value);
            free(expr);
            break;
        }
        
        case AST_CALL_EXPR: {
            ASTCallExpr *call = (ASTCallExpr *)node->data;
            ast_node_free(call->callee);
            for (size_t i = 0; i < call->arg_count; i++) {
                ast_node_free(call->args[i]);
            }
            free(call->args);
            free(call);
            break;
        }
        
        case AST_IDENTIFIER: {
            ASTIdentifier *id = (ASTIdentifier *)node->data;
            free(id->name);
            free(id);
            break;
        }
        
        case AST_NUM_LITERAL: {
            ASTNumLiteral *num = (ASTNumLiteral *)node->data;
            free(num->raw);
            free(num);
            break;
        }
        
        case AST_STRING_LITERAL: {
            ASTStringLiteral *str = (ASTStringLiteral *)node->data;
            free(str->value);
            free(str);
            break;
        }
        
        case AST_RETURN_STMT: {
            ASTReturnStmt *ret = (ASTReturnStmt *)node->data;
            ast_node_free(ret->value);
            free(ret);
            break;
        }
        
        case AST_TRY_STMT: {
            ASTTryStmt *try_stmt = (ASTTryStmt *)node->data;
            ast_node_free(try_stmt->try_block);
            if (try_stmt->catch_param) free(try_stmt->catch_param);
            ast_node_free(try_stmt->catch_block);
            ast_node_free(try_stmt->finally_block);
            free(try_stmt);
            break;
        }
        
        case AST_IF_STMT: {
            ASTIfStmt *ifstmt = (ASTIfStmt *)node->data;
            for (size_t i = 0; i < ifstmt->cond_count; i++) {
                ast_node_free(ifstmt->conditions[i]);
                ast_node_free(ifstmt->then_blocks[i]);
            }
            free(ifstmt->conditions);
            free(ifstmt->then_blocks);
            ast_node_free(ifstmt->else_block);
            free(ifstmt);
            break;
        }
        
        case AST_ASSIGN_STMT: {
            ASTAssignStmt *assign = (ASTAssignStmt *)node->data;
            ast_node_free(assign->target);
            ast_node_free(assign->value);
            free(assign);
            break;
        }
        
        case AST_ARRAY_LITERAL: {
            ASTArrayLiteral *arr = (ASTArrayLiteral *)node->data;
            for (size_t i = 0; i < arr->elem_count; i++) {
                ast_node_free(arr->elements[i]);
            }
            if (arr->elements) free(arr->elements);
            if (arr->is_spread) free(arr->is_spread);
            free(arr);
            break;
        }
        
        case AST_OBJECT_LITERAL: {
            ASTObjectLiteral *obj = (ASTObjectLiteral *)node->data;
            for (size_t i = 0; i < obj->prop_count; i++) {
                if (obj->properties[i].key) free(obj->properties[i].key);
                ast_node_free(obj->properties[i].value);
            }
            if (obj->properties) free(obj->properties);
            free(obj);
            break;
        }
        
        default:
            if (node->data) free(node->data);
            break;
    }
    
    free(node);
}

/* ============================================================================
 * 具体节点创建函数
 * ============================================================================ */

ASTNode *ast_program_create(ASTNode **statements, size_t count, SourceLocation loc) {
    ASTNode *node = ast_node_create(AST_PROGRAM, loc);
    if (!node) return NULL;
    
    ASTProgram *prog = (ASTProgram *)malloc(sizeof(ASTProgram));
    prog->statements = statements;
    prog->stmt_count = count;
    node->data = prog;
    
    return node;
}

ASTNode *ast_var_decl_create(char *name, ASTNode *type_ann, bool is_const, 
                              ASTNode *init, SourceLocation loc) {
    ASTNode *node = ast_node_create(AST_VAR_DECL, loc);
    if (!node) return NULL;
    
    ASTVarDecl *decl = (ASTVarDecl *)malloc(sizeof(ASTVarDecl));
    decl->name = name;
    decl->type_annotation = type_ann;
    decl->is_const = is_const;
    decl->init_expr = init;
    node->data = decl;
    
    return node;
}

ASTNode *ast_func_decl_create(char *name, char **params, size_t param_count,
                               ASTNode *return_type, ASTNode *body, SourceLocation loc) {
    ASTNode *node = ast_node_create(AST_FUNC_DECL, loc);
    if (!node) return NULL;
    
    ASTFuncDecl *func = (ASTFuncDecl *)malloc(sizeof(ASTFuncDecl));
    func->name = name;
    func->params = params;
    func->param_count = param_count;
    func->return_type = return_type;
    func->body = body;
    func->uses_self = false;  /* 初始化为 false，后续会通过分析设置 */
    node->data = func;
    
    return node;
}

ASTNode *ast_block_create(ASTNode **statements, size_t count, SourceLocation loc) {
    ASTNode *node = ast_node_create(AST_BLOCK, loc);
    if (!node) return NULL;
    
    ASTBlock *block = (ASTBlock *)malloc(sizeof(ASTBlock));
    block->statements = statements;
    block->stmt_count = count;
    node->data = block;
    
    return node;
}

ASTNode *ast_binary_expr_create(TokenKind op, ASTNode *left, ASTNode *right, 
                                 SourceLocation loc) {
    ASTNode *node = ast_node_create(AST_BINARY_EXPR, loc);
    if (!node) return NULL;
    
    ASTBinaryExpr *expr = (ASTBinaryExpr *)malloc(sizeof(ASTBinaryExpr));
    expr->op = op;
    expr->left = left;
    expr->right = right;
    node->data = expr;
    
    return node;
}

ASTNode *ast_call_expr_create(ASTNode *callee, ASTNode **args, size_t arg_count,
                               int throw_on_error, SourceLocation loc) {
    ASTNode *node = ast_node_create(AST_CALL_EXPR, loc);
    if (!node) return NULL;
    
    ASTCallExpr *call = (ASTCallExpr *)malloc(sizeof(ASTCallExpr));
    call->callee = callee;
    call->args = args;
    call->arg_count = arg_count;
    call->throw_on_error = throw_on_error;
    node->data = call;
    
    return node;
}

ASTNode *ast_identifier_create(char *name, SourceLocation loc) {
    ASTNode *node = ast_node_create(AST_IDENTIFIER, loc);
    if (!node) return NULL;
    
    ASTIdentifier *id = (ASTIdentifier *)malloc(sizeof(ASTIdentifier));
    id->name = strdup(name);
    node->data = id;
    
    return node;
}

ASTNode *ast_num_literal_create(double value, char *raw, SourceLocation loc) {
    ASTNode *node = ast_node_create(AST_NUM_LITERAL, loc);
    if (!node) return NULL;
    
    ASTNumLiteral *num = (ASTNumLiteral *)malloc(sizeof(ASTNumLiteral));
    num->value = value;
    num->raw = raw ? strdup(raw) : NULL;
    node->data = num;
    
    return node;
}

ASTNode *ast_string_literal_create(char *value, size_t length, SourceLocation loc) {
    ASTNode *node = ast_node_create(AST_STRING_LITERAL, loc);
    if (!node) return NULL;
    
    ASTStringLiteral *str = (ASTStringLiteral *)malloc(sizeof(ASTStringLiteral));
    /* 使用memcpy而不是strdup，支持包含\0的字符串 */
    str->value = (char*)malloc(length + 1);
    if (str->value) {
        memcpy(str->value, value, length);
        str->value[length] = '\0';  /* 添加终止符以便其他地方使用 */
    }
    str->length = length;  /* 保存实际长度，支持\0字符串 */
    node->data = str;
    
    return node;
}

ASTNode *ast_bool_literal_create(bool value, SourceLocation loc) {
    ASTNode *node = ast_node_create(AST_BOOL_LITERAL, loc);
    if (!node) return NULL;
    
    ASTBoolLiteral *bl = (ASTBoolLiteral *)malloc(sizeof(ASTBoolLiteral));
    bl->value = value;
    node->data = bl;
    
    return node;
}

ASTNode *ast_return_stmt_create(ASTNode *value, SourceLocation loc) {
    ASTNode *node = ast_node_create(AST_RETURN_STMT, loc);
    if (!node) return NULL;
    
    ASTReturnStmt *ret = (ASTReturnStmt *)malloc(sizeof(ASTReturnStmt));
    ret->value = value;
    node->data = ret;
    
    return node;
}

ASTNode *ast_try_stmt_create(ASTNode *try_block, char *catch_param,
                              ASTNode *catch_block, ASTNode *finally_block, SourceLocation loc) {
    ASTNode *node = ast_node_create(AST_TRY_STMT, loc);
    if (!node) return NULL;
    
    ASTTryStmt *try_stmt = (ASTTryStmt *)malloc(sizeof(ASTTryStmt));
    try_stmt->try_block = try_block;
    try_stmt->catch_param = catch_param;
    try_stmt->catch_block = catch_block;
    try_stmt->finally_block = finally_block;
    node->data = try_stmt;
    
    return node;
}

ASTNode *ast_if_stmt_create(ASTNode **conditions, ASTNode **then_blocks,
                             size_t cond_count, ASTNode *else_block, SourceLocation loc) {
    ASTNode *node = ast_node_create(AST_IF_STMT, loc);
    if (!node) return NULL;
    
    ASTIfStmt *ifstmt = (ASTIfStmt *)malloc(sizeof(ASTIfStmt));
    ifstmt->conditions = conditions;
    ifstmt->then_blocks = then_blocks;
    ifstmt->cond_count = cond_count;
    ifstmt->else_block = else_block;
    node->data = ifstmt;
    
    return node;
}

ASTNode *ast_assign_stmt_create(ASTNode *target, ASTNode *value, SourceLocation loc) {
    ASTNode *node = ast_node_create(AST_ASSIGN_STMT, loc);
    if (!node) return NULL;
    
    ASTAssignStmt *assign = (ASTAssignStmt *)malloc(sizeof(ASTAssignStmt));
    assign->target = target;
    assign->value = value;
    node->data = assign;
    
    return node;
}

const char *ast_kind_name(ASTNodeKind kind) {
    switch (kind) {
        case AST_PROGRAM: return "PROGRAM";
        case AST_VAR_DECL: return "VAR_DECL";
        case AST_FUNC_DECL: return "FUNC_DECL";
        case AST_BLOCK: return "BLOCK";
        case AST_BINARY_EXPR: return "BINARY_EXPR";
        case AST_TERNARY_EXPR: return "TERNARY_EXPR";
        case AST_CALL_EXPR: return "CALL_EXPR";
        case AST_IDENTIFIER: return "IDENTIFIER";
        case AST_SELF_EXPR: return "SELF_EXPR";
        case AST_NUM_LITERAL: return "NUM_LITERAL";
        case AST_STRING_LITERAL: return "STRING_LITERAL";
        case AST_RETURN_STMT: return "RETURN_STMT";
        case AST_TRY_STMT: return "TRY_STMT";
        case AST_IF_STMT: return "IF_STMT";
        case AST_ASSIGN_STMT: return "ASSIGN_STMT";
        default: return "UNKNOWN";
    }
}

void ast_print(ASTNode *node, int indent) {
    if (!node) return;
    
    for (int i = 0; i < indent; i++) printf("  ");
    printf("%s\n", ast_kind_name(node->kind));
    
    // 递归打印子节点（简化版）
    switch (node->kind) {
        case AST_PROGRAM: {
            ASTProgram *prog = (ASTProgram *)node->data;
            for (size_t i = 0; i < prog->stmt_count; i++) {
                ast_print(prog->statements[i], indent + 1);
            }
            break;
        }
        case AST_BINARY_EXPR: {
            ASTBinaryExpr *expr = (ASTBinaryExpr *)node->data;
            ast_print(expr->left, indent + 1);
            ast_print(expr->right, indent + 1);
            break;
        }
        case AST_TERNARY_EXPR: {
            ASTTernaryExpr *expr = (ASTTernaryExpr *)node->data;
            ast_print(expr->condition, indent + 1);
            ast_print(expr->true_value, indent + 1);
            ast_print(expr->false_value, indent + 1);
            break;
        }
        case AST_IDENTIFIER: {
            ASTIdentifier *id = (ASTIdentifier *)node->data;
            for (int i = 0; i < indent + 1; i++) printf("  ");
            printf("name: %s\n", id->name);
            break;
        }
        default:
            break;
    }
}

/* ============================================================================
 * 新增的AST节点创建函数
 * ============================================================================ */

// 一元表达式
ASTNode *ast_unary_expr_create(TokenKind op, ASTNode *operand, SourceLocation loc) {
    ASTNode *node = ast_node_create(AST_UNARY_EXPR, loc);
    ASTUnaryExpr *expr = (ASTUnaryExpr *)malloc(sizeof(ASTUnaryExpr));
    expr->op = op;
    expr->operand = operand;
    expr->is_postfix = false;  // 默认为前缀
    node->data = expr;
    return node;
}

// 三元表达式
ASTNode *ast_ternary_expr_create(ASTNode *condition, ASTNode *true_value, 
                                  ASTNode *false_value, SourceLocation loc) {
    ASTNode *node = ast_node_create(AST_TERNARY_EXPR, loc);
    ASTTernaryExpr *expr = (ASTTernaryExpr *)malloc(sizeof(ASTTernaryExpr));
    expr->condition = condition;
    expr->true_value = true_value;
    expr->false_value = false_value;
    node->data = expr;
    return node;
}

// 成员访问表达式
ASTNode *ast_member_expr_create(ASTNode *object, char *property, bool is_computed, SourceLocation loc) {
    ASTNode *node = ast_node_create(AST_MEMBER_EXPR, loc);
    ASTMemberExpr *expr = (ASTMemberExpr *)malloc(sizeof(ASTMemberExpr));
    expr->object = object;
    expr->property = property;
    expr->is_computed = is_computed;
    expr->is_unbound = false;  // 默认为绑定访问
    expr->is_optional = false; // 默认为非可选访问
    node->data = expr;
    return node;
}

// 索引访问表达式
ASTNode *ast_index_expr_create(ASTNode *object, ASTNode *index, SourceLocation loc) {
    ASTNode *node = ast_node_create(AST_INDEX_EXPR, loc);
    ASTIndexExpr *expr = (ASTIndexExpr *)malloc(sizeof(ASTIndexExpr));
    expr->object = object;
    expr->index = index;
    expr->is_unbound = false;  // 默认为普通绑定访问
    expr->is_optional = false; // 默认为非可选访问
    node->data = expr;
    return node;
}

// 数组字面量（无展开）
ASTNode *ast_array_literal_create(ASTNode **elements, size_t count, SourceLocation loc) {
    ASTNode *node = ast_node_create(AST_ARRAY_LITERAL, loc);
    ASTArrayLiteral *arr = (ASTArrayLiteral *)malloc(sizeof(ASTArrayLiteral));
    arr->elements = elements;
    arr->elem_count = count;
    // 初始化 is_spread 为全 false
    if (count > 0) {
        arr->is_spread = (bool *)calloc(count, sizeof(bool));
    } else {
        arr->is_spread = NULL;
    }
    node->data = arr;
    return node;
}

// 数组字面量（支持展开）
ASTNode *ast_array_literal_create_with_spread(ASTNode **elements, bool *is_spread, size_t count, SourceLocation loc) {
    ASTNode *node = ast_node_create(AST_ARRAY_LITERAL, loc);
    ASTArrayLiteral *arr = (ASTArrayLiteral *)malloc(sizeof(ASTArrayLiteral));
    arr->elements = elements;
    arr->is_spread = is_spread;
    arr->elem_count = count;
    node->data = arr;
    return node;
}

// 对象字面量
ASTNode *ast_object_literal_create(ASTObjectProperty *properties, size_t count, SourceLocation loc) {
    ASTNode *node = ast_node_create(AST_OBJECT_LITERAL, loc);
    ASTObjectLiteral *obj = (ASTObjectLiteral *)malloc(sizeof(ASTObjectLiteral));
    obj->properties = properties;
    obj->prop_count = count;
    node->data = obj;
    return node;
}

// 循环语句
ASTNode *ast_loop_stmt_create(LoopType type, ASTNode *body, SourceLocation loc) {
    ASTNode *node = ast_node_create(AST_LOOP_STMT, loc);
    ASTLoopStmt *loop = (ASTLoopStmt *)malloc(sizeof(ASTLoopStmt));
    loop->loop_type = type;
    loop->label = NULL;  /* 默认无标签 */
    loop->body = body;
    node->data = loop;
    return node;
}

// for循环专用
ASTNode *ast_for_loop_create(ASTNode *init, ASTNode *cond, ASTNode *update, ASTNode *body, SourceLocation loc) {
    ASTNode *node = ast_loop_stmt_create(LOOP_FOR, body, loc);
    ASTLoopStmt *loop = (ASTLoopStmt *)node->data;
    loop->loop_data.for_loop.init = init;
    loop->loop_data.for_loop.condition = cond;
    loop->loop_data.for_loop.update = update;
    return node;
}

// 重复循环专用: L> [n] { }
ASTNode *ast_repeat_loop_create(ASTNode *count_expr, ASTNode *body, SourceLocation loc) {
    ASTNode *node = ast_loop_stmt_create(LOOP_REPEAT, body, loc);
    ASTLoopStmt *loop = (ASTLoopStmt *)node->data;
    loop->loop_data.repeat_count = count_expr;
    return node;
}

// foreach循环专用: L> (array : item) { }
ASTNode *ast_foreach_loop_create(ASTNode *iterable, char *item_var, ASTNode *body, SourceLocation loc) {
    ASTNode *node = ast_loop_stmt_create(LOOP_FOREACH, body, loc);
    ASTLoopStmt *loop = (ASTLoopStmt *)node->data;
    loop->loop_data.foreach_loop.iterable = iterable;
    loop->loop_data.foreach_loop.item_var = item_var;
    return node;
}
