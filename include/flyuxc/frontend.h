/* include/flyuxc/frontend.h - 前端模块统一头文件 */
#ifndef FLYUXC_FRONTEND_H
#define FLYUXC_FRONTEND_H

/* 词法分析模块 */
#include "frontend/lexer.h"
#include "frontend/normalize.h"
#include "frontend/varmap.h"

/* 语法分析模块 */
#include "frontend/parser.h"
#include "frontend/ast.h"

#endif /* FLYUXC_FRONTEND_H */
