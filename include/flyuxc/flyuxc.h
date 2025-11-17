/* include/flyuxc/flyuxc.h - FLYUX编译器主头文件 */
#ifndef FLYUXC_H
#define FLYUXC_H

/* 编译器版本信息 */
#define FLYUXC_VERSION_MAJOR 0
#define FLYUXC_VERSION_MINOR 1
#define FLYUXC_VERSION_PATCH 0
#define FLYUXC_VERSION "0.1.0"

/* 编译器模块 */
#include "frontend.h"   /* 前端模块：词法、语法、语义分析 */
#include "utils.h"      /* 工具模块：内存、IO、CLI */

/* 编译器配置 */
#ifndef FLYUXC_MAX_ERRORS
#define FLYUXC_MAX_ERRORS 100
#endif

#ifndef FLYUXC_MAX_WARNINGS
#define FLYUXC_MAX_WARNINGS 200
#endif

#endif /* FLYUXC_H */
