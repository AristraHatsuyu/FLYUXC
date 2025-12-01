#include "flyuxc/frontend/normalize.h"
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdio.h>

/**
 * 格式规范化模块
 * - 空白紧凑化
 * - 基于运算优先级的安全括号化简
 * - 保护函数定义参数列表括号与调用括号
 * - 允许在任何位置（包括代码块内）化简分组括号
 */

/* ---------------- 基础工具 ---------------- */

static int is_space_c(int c){ return c==' '||c=='\t'||c=='\n'||c=='\r'||c=='\v'||c=='\f'; }
static int is_ident_char(int c){ return (c=='_'||isalnum((unsigned char)c)||(unsigned char)c>=0x80); }

static int prev_nonspace_idx(const char* s, int i){
    for(int k=i;k>=0;k--){ if(!is_space_c((unsigned char)s[k])) return k; }
    return -1;
}
static int next_nonspace_idx(const char* s, int i){
    int n=(int)strlen(s);
    for(int k=i;k<n;k++){ if(!is_space_c((unsigned char)s[k])) return k; }
    return n;
}

static char* dup_range(const char* s, int l, int r_incl){
    if (l>r_incl){ char* z=(char*)malloc(1); if(z) z[0]='\0'; return z; }
    int n=r_incl-l+1; char* out=(char*)malloc(n+1); if(!out) return NULL;
    memcpy(out, s+l, n); out[n]='\0'; return out;
}

/* ---------------- 括号/字符串步进 ---------------- */

static int find_matching_paren(const char* s, int i){
    int n=(int)strlen(s), in_str=0, esc=0, pd=0, bd=0, cd=0;
    char str_quote = 0;  /* 记录字符串开始的引号类型 */
    for (int k=i; k<n; k++){
        char c=s[k];
        if (esc){ esc=0; continue; }
        if (c=='\\'){ esc=1; continue; }
        if (!in_str && (c=='"'||c==39)){  /* 39 is '\'' */
            in_str=1;
            str_quote=c;
            continue;
        }
        if (in_str && c==str_quote){
            in_str=0;
            str_quote=0;
            continue;
        }
        if (in_str) continue;

        // 追踪所有类型的括号深度
        if (c=='(') pd++;
        else if (c==')'){ pd--; if (pd==0 && bd==0 && cd==0) return k; }
        else if (c=='[') bd++;
        else if (c==']') bd--;
        else if (c=='{') cd++;
        else if (c=='}') cd--;
    }
    return -1;
}

/* '(' 是否为调用括号（紧随标识符/后缀） */
static int is_call_paren(const char* s, int open_pos){
    int j=prev_nonspace_idx(s, open_pos-1);
    if (j<0) return 0;
    if (is_ident_char((unsigned char)s[j])) return 1;
    if (s[j]==')'||s[j]==']'||s[j]=='}') return 1;
    if (s[j]=='.') return 1;
    if (j>=1 && s[j-1]=='.' && s[j]=='>') return 1; // .>
    return 0;
}

/* '(' 是否为函数定义的参数列表：
 * 1. 左侧是 '='（或 ':=' 的 '='），右侧紧跟 '{'  - 传统函数定义
 * 2. 右侧紧跟 '{'，左侧是 ',' 或 '(' - 匿名函数作为参数
 * 3. 右侧紧跟 '{'，没有左侧字符（开头） - 匿名函数在表达式开始位置
 */
static int is_function_param_list(const char* s, int open_pos, int close_pos){
    int next = next_nonspace_idx(s, close_pos+1);
    if (s[next] != '{') return 0;  // 右侧必须是 {
    
    int prev = prev_nonspace_idx(s, open_pos-1);
    
    // 没有左侧字符（匿名函数在开头）
    if (prev < 0) return 1;
    
    // 传统函数定义：左侧是 '='
    if (s[prev] == '=') return 1;
    
    // 匿名函数作为参数：左侧是 ',' 或 '('
    if (s[prev] == ',' || s[prev] == '(') return 1;
    
    return 0;
}

/* ---------------- 优先级与操作符 ---------------- */
/* 数值越大优先级越高，仅用于比较强弱 */
/* C标准: ** > * / % > + - > < <= > >= > == != > & > ^ > | > && > || */
/* 注意: 虽然C标准表中数字越大优先级越低，但在此enum中数值越大优先级越高 */
enum {
    PREC_COMMA   = 5,
    PREC_ASSIGN  = 10,  // =, :=
    PREC_OR      = 20,  // ||      (最低)
    PREC_AND     = 30,  // &&
    PREC_BW_OR   = 40,  // |       (高于 &&)
    PREC_BW_XOR  = 42,  // ^       (高于 |)
    PREC_BW_AND  = 44,  // &       (高于 ^)
    PREC_EQ      = 48,  // == !=   (高于 &)
    PREC_CMP     = 50,  // < > <= >=
    PREC_ADD     = 60,  // + -
    PREC_MUL     = 70,  // * / %
    PREC_POW     = 80,  // **
    PREC_UNARY   = 90,  // ! 一元 + -
    PREC_POSTFIX = 100  // .  .>  []  ()  (最高)
};

typedef struct { int start,end,prec,is_unary,exists; } OpInfo;

static OpInfo read_op_left(const char* s, int pos_before){
    OpInfo op={-1,-1,0,0,0}; if(pos_before<0) return op;
    int i=pos_before;

    if (i>=1){
        if(s[i-1]=='.'&&s[i]=='>'){ op.start=i-1; op.end=i; op.prec=PREC_POSTFIX; op.exists=1; return op; }
        if(s[i-1]=='&'&&s[i]=='&'){ op.start=i-1; op.end=i; op.prec=PREC_AND; op.exists=1; return op; }
        if(s[i-1]=='|'&&s[i]=='|'){ op.start=i-1; op.end=i; op.prec=PREC_OR;  op.exists=1; return op; }
        if(s[i-1]=='='&&s[i]=='='){ op.start=i-1; op.end=i; op.prec=PREC_EQ; op.exists=1; return op; }
        if(s[i-1]=='!'&&s[i]=='='){ op.start=i-1; op.end=i; op.prec=PREC_EQ; op.exists=1; return op; }
        if(s[i-1]=='<'&&s[i]=='='){ op.start=i-1; op.end=i; op.prec=PREC_CMP; op.exists=1; return op; }
        if(s[i-1]=='>'&&s[i]=='='){ op.start=i-1; op.end=i; op.prec=PREC_CMP; op.exists=1; return op; }
        if(s[i-1]=='*'&&s[i]=='*'){ op.start=i-1; op.end=i; op.prec=PREC_POW; op.exists=1; return op; }
        if(s[i-1]==':'&&s[i]=='='){ op.start=i-1; op.end=i; op.prec=PREC_ASSIGN; op.exists=1; return op; }
        if(s[i-1]=='+'&&s[i]=='+'){ op.start=i-1; op.end=i; op.prec=PREC_POSTFIX; op.exists=1; return op; }
        if(s[i-1]=='-'&&s[i]=='-'){ op.start=i-1; op.end=i; op.prec=PREC_POSTFIX; op.exists=1; return op; }
    }

    switch (s[i]){
        case '.': op.start=i; op.end=i; op.prec=PREC_POSTFIX; op.exists=1; return op;
        case '+': case '-': {
            int j=prev_nonspace_idx(s, i-1);
            int prev_is_operand = (j>=0&&(is_ident_char((unsigned char)s[j])||s[j]==')'||s[j]==']'||s[j]=='}'));
            if (prev_is_operand){ op.start=i; op.end=i; op.prec=PREC_ADD; op.exists=1; return op; }
            else { op.start=i; op.end=i; op.prec=PREC_UNARY; op.is_unary=1; op.exists=1; return op; }
        }
        case '*': case '/': case '%': op.start=i; op.end=i; op.prec=PREC_MUL; op.exists=1; return op;
        case '<': case '>': op.start=i; op.end=i; op.prec=PREC_CMP; op.exists=1; return op;
        case '=':           op.start=i; op.end=i; op.prec=PREC_ASSIGN; op.exists=1; return op;
        case '!':           op.start=i; op.end=i; op.prec=PREC_UNARY; op.is_unary=1; op.exists=1; return op;
        case '&':           op.start=i; op.end=i; op.prec=PREC_BW_AND; op.exists=1; return op;
        case '^':           op.start=i; op.end=i; op.prec=PREC_BW_XOR; op.exists=1; return op;
        case '|':           op.start=i; op.end=i; op.prec=PREC_BW_OR;  op.exists=1; return op;
        default: break;
    }
    return op;
}

static OpInfo read_op_right(const char* s, int pos_after){
    OpInfo op={-1,-1,0,0,0}; int n=(int)strlen(s); if(pos_after>=n) return op;
    int i=pos_after;

    if (i+1<n){
        if(s[i]=='.'&&s[i+1]=='>'){ op.start=i; op.end=i+1; op.prec=PREC_POSTFIX; op.exists=1; return op; }
        if(s[i]=='&'&&s[i+1]=='&'){ op.start=i; op.end=i+1; op.prec=PREC_AND; op.exists=1; return op; }
        if(s[i]=='|'&&s[i+1]=='|'){ op.start=i; op.end=i+1; op.prec=PREC_OR;  op.exists=1; return op; }
        if(s[i]=='='&&s[i+1]=='='){ op.start=i; op.end=i+1; op.prec=PREC_EQ; op.exists=1; return op; }
        if(s[i]=='!'&&s[i+1]=='='){ op.start=i; op.end=i+1; op.prec=PREC_EQ; op.exists=1; return op; }
        if(s[i]=='<'&&s[i+1]=='='){ op.start=i; op.end=i+1; op.prec=PREC_CMP; op.exists=1; return op; }
        if(s[i]=='>'&&s[i+1]=='='){ op.start=i; op.end=i+1; op.prec=PREC_CMP; op.exists=1; return op; }
        if(s[i]=='*'&&s[i+1]=='*'){ op.start=i; op.end=i+1; op.prec=PREC_POW; op.exists=1; return op; }
        if(s[i]==':'&&s[i+1]=='='){ op.start=i; op.end=i+1; op.prec=PREC_ASSIGN; op.exists=1; return op; }
        if(s[i]=='+'&&s[i+1]=='+'){ op.start=i; op.end=i+1; op.prec=PREC_POSTFIX; op.exists=1; return op; }
        if(s[i]=='-'&&s[i+1]=='-'){ op.start=i; op.end=i+1; op.prec=PREC_POSTFIX; op.exists=1; return op; }
    }

    switch (s[i]){
        case '.': op.start=i; op.end=i; op.prec=PREC_POSTFIX; op.exists=1; return op;
        case ',': op.start=i; op.end=i; op.prec=PREC_COMMA;  op.exists=1; return op;
        case '+': case '-': op.start=i; op.end=i; op.prec=PREC_ADD; op.exists=1; return op;
        case '*': case '/': case '%': op.start=i; op.end=i; op.prec=PREC_MUL; op.exists=1; return op;
        case '<': case '>': op.start=i; op.end=i; op.prec=PREC_CMP; op.exists=1; return op;
        case '=':           op.start=i; op.end=i; op.prec=PREC_ASSIGN; op.exists=1; return op;
        case '&':           op.start=i; op.end=i; op.prec=PREC_BW_AND; op.exists=1; return op;
        case '^':           op.start=i; op.end=i; op.prec=PREC_BW_XOR; op.exists=1; return op;
        case '|':           op.start=i; op.end=i; op.prec=PREC_BW_OR;  op.exists=1; return op;
        default: break;
    }
    return op;
}

/* 计算字符串 s 的“顶层（二元）运算符”的最低优先级 */
static int min_top_level_binary_prec(const char* s){
    int n=(int)strlen(s), in_str=0, esc=0;
    char str_quote = 0;
    int pd=0, bd=0, cd=0;
    int last_operand=0, minp=1000000;

    for(int i=0;i<n;i++){
        char c=s[i];

        if(esc){esc=0;continue;}
        if(c=='\\'){esc=1;continue;}

        if(!in_str && (c=='"'||c==39)){
            in_str=1;
            str_quote=c;
            continue;
        }
        if(in_str && c==str_quote){
            in_str=0;
            str_quote=0;
            continue;
        }
        if(in_str) continue;

        if(c=='('){ pd++; last_operand=0; continue; }
        if(c==')'){ pd--; last_operand=1; continue; }
        if(c=='['){ bd++; last_operand=0; continue; }
        if(c==']'){ bd--; last_operand=1; continue; }
        if(c=='{'){ cd++; last_operand=0; continue; }
        if(c=='}'){ cd--; last_operand=1; continue; }

        if (pd||bd||cd) continue;

        if (is_ident_char((unsigned char)c)){ last_operand=1; continue; }

        int p=-1;
        if(i+1<n){
            if(s[i]=='.'&&s[i+1]=='>') p=PREC_POSTFIX;
            else if(s[i]=='&'&&s[i+1]=='&') p=PREC_AND;
            else if(s[i]=='|'&&s[i+1]=='|') p=PREC_OR;
            else if(s[i]=='='&&s[i+1]=='=') p=PREC_CMP;
            else if(s[i]=='!'&&s[i+1]=='=') p=PREC_CMP;
            else if(s[i]=='<'&&s[i+1]=='=') p=PREC_CMP;
            else if(s[i]=='>'&&s[i+1]=='=') p=PREC_CMP;
            else if(s[i]=='*'&&s[i+1]=='*') p=PREC_POW;
            else if(s[i]==':'&&s[i+1]=='=') p=PREC_ASSIGN;
            else if(s[i]=='+'&&s[i+1]=='+') p=PREC_POSTFIX;
            else if(s[i]=='-'&&s[i+1]=='-') p=PREC_POSTFIX;
            if (p>=0){ if(p!=PREC_UNARY){ if(p<minp) minp=p; } i++; last_operand=0; continue; }
        }

        switch (c){
            case '+': case '-': p = last_operand ? PREC_ADD : PREC_UNARY; break;
            case '*': case '/': case '%': p=PREC_MUL; break;
            case '<': case '>': p=PREC_CMP; break;
            case '=':           p=PREC_ASSIGN; break;
            case '&':           p=PREC_BW_AND; break;
            case '^':           p=PREC_BW_XOR; break;
            case '|':           p=PREC_BW_OR;  break;
            case '!':           p=PREC_UNARY;  break;
            case '.':           p=PREC_POSTFIX;break;
            case ',':           p=PREC_COMMA;  break;
            default:            p=-1;          break;
        }
        if (p>=0){ if(p!=PREC_UNARY){ if(p<minp) minp=p; } last_operand=0; } else { last_operand=0; }
    }
    return (minp==1000000) ? -1 : minp;
}

static int fully_enclosed_by_paren(const char* s){
    int n=(int)strlen(s);
    if (n<2||s[0]!='('||s[n-1]!=')') return 0;
    int in_str=0,esc=0,pd=0,bd=0,cd=0;
    char str_quote = 0;
    for (int i=0;i<n;i++){
        char c=s[i];
        if(esc){esc=0;continue;}
        if(c=='\\'){esc=1;continue;}
        if(!in_str && (c=='"'||c==39)){
            in_str=1;
            str_quote=c;
            continue;
        }
        if(in_str && c==str_quote){
            in_str=0;
            str_quote=0;
            continue;
        }
        if(in_str) continue;

        if(c=='('){ pd++; }
        else if(c==')'){ pd--; if (pd==0 && i!=n-1) return 0; }
        else if(c=='['){ bd++; }
        else if(c==']'){ bd--; }
        else if(c=='{'){ cd++; }
        else if(c=='}'){ cd--; }
    }
    return (pd==0);
}

static int is_primary_expr(const char* s){
    int n=(int)strlen(s), l=0, r=n-1;
    while(l<=r && is_space_c((unsigned char)s[l])) l++;
    while(r>=l && is_space_c((unsigned char)s[r])) r--;
    if(l>r) return 0;

    if ((s[l]=='"'&&s[r]=='"') || (s[l]=='\''&&s[r]=='\'')) return 1;
    if ((s[l]=='['&&s[r]==']') || (s[l]=='{'&&s[r]=='}')) return 1;

    for (int i=l;i<=r;i++){ if (!is_ident_char((unsigned char)s[i])) return 0; }
    return 1;
}

/* ---------------- 空白紧凑化 ---------------- */

/* 判断字符是否是标识符字符（字母、数字、下划线、Unicode字符） */
static int is_ident_char_for_ws(unsigned char ch) {
    return ch == '_' || isalnum(ch) || ch >= 0x80;
}

static char* normalize_whitespace(const char* stmt){
    if (!stmt) return NULL;
    size_t len=strlen(stmt);
    char* result=(char*)malloc(len+1); if(!result) return NULL;
    int out=0;
    int in_string=0;   // 跟踪是否在字符串内
    char str_quote=0;  // 跟踪开启字符串的引号类型
    int escape=0;      // 跟踪转义状态
    
    for (size_t i=0;i<len;i++){
        unsigned char ch=(unsigned char)stmt[i];
        
        // 处理转义
        if (escape) {
            result[out++]=(char)ch;
            escape=0;
            continue;
        }
        if (ch=='\\') {
            result[out++]=(char)ch;
            escape=1;
            continue;
        }
        
        // 处理字符串边界 - 区分单双引号
        if (!in_string && (ch=='"' || ch==39)) {  /* 39 is '\'' */
            in_string=1;
            str_quote=(char)ch;
            result[out++]=(char)ch;
            continue;
        }
        if (in_string && ch==str_quote) {
            in_string=0;
            str_quote=0;
            result[out++]=(char)ch;
            continue;
        }
        
        // 在字符串内：保留所有字符（包括空格）
        if (in_string) {
            result[out++]=(char)ch;
            continue;
        }
        
        // 在字符串外：智能处理空格
        // 规则：
        // - 标识符和标识符之间的空格 → 保留（如 "if x" 不应变成 "ifx"）
        // - 标识符和符号之间的空格 → 删除（如 "x (" 变成 "x("）
        // - 符号和符号之间的空格 → 删除（如 "+ +" 变成 "++"）
        if (is_space_c(ch)) {
            // 查看前一个非空格字符和后一个非空格字符
            unsigned char prev_char = 0;
            unsigned char next_char = 0;
            
            // 获取前一个字符
            if (out > 0) {
                prev_char = (unsigned char)result[out - 1];
            }
            
            // 获取后一个非空格字符
            size_t j = i + 1;
            while (j < len && is_space_c((unsigned char)stmt[j])) {
                j++;
            }
            if (j < len) {
                next_char = (unsigned char)stmt[j];
            }
            
            // 如果前后都是标识符字符，保留空格
            if (is_ident_char_for_ws(prev_char) && is_ident_char_for_ws(next_char)) {
                result[out++] = ' ';  // 只保留一个空格
            }
            // 否则删除空格
            continue;
        }
        
        result[out++]=(char)ch;
    }
    result[out]='\0';
    return result;
}

/* ---------------- 括号化简核心 ---------------- */

static char* simplify_parens(const char* expr){
    if (!expr) return NULL;

    int n=(int)strlen(expr);
    char* out=(char*)malloc(n*2+32); if(!out) return NULL;

    int out_idx=0;
    int in_str=0;
    char str_quote=0;  // 跟踪开启字符串的引号类型
    int esc=0;
    int bd=0, cd=0; // 追踪 [] 与 {}，但不用于禁止块内化简

    for (int i=0;i<n;i++){
        char c=expr[i];

        if(esc){ out[out_idx++]=c; esc=0; continue; }
        if(c=='\\'){ out[out_idx++]=c; esc=1; continue; }

        // 区分单双引号
        if(!in_str && (c=='"'||c==39)){ in_str=1; str_quote=c; out[out_idx++]=c; continue; }
        if(in_str && c==str_quote){ in_str=0; str_quote=0; out[out_idx++]=c; continue; }
        if(in_str){ out[out_idx++]=c; continue; }

        if(c=='['){ bd++; out[out_idx++]=c; continue; }
        if(c==']'){ bd--; out[out_idx++]=c; continue; }
        if(c=='{'){ cd++; out[out_idx++]=c; continue; }
        if(c=='}'){ cd--; out[out_idx++]=c; continue; }

        if (c=='('){
            int close = find_matching_paren(expr, i);
            if (close<0){ out[out_idx++]=c; continue; }

            char* inner_raw = dup_range(expr, i+1, close-1);
            if(!inner_raw){ out[out_idx++]=c; continue; }
            char* inner = simplify_parens(inner_raw);  // 递归先简化内部
            free(inner_raw);
            if(!inner){ out[out_idx++]=c; continue; }

            // 把内层连环外壳剥到“至多一层”
            while (fully_enclosed_by_paren(inner)){
                int L=(int)strlen(inner);
                char* tmp=dup_range(inner, 1, L-2);
                free(inner); inner=tmp;
                if(!inner) break;
            }
            if(!inner){ out[out_idx++]='('; out[out_idx++]=')'; i=close; continue; }

            int prev_i = prev_nonspace_idx(expr, i-1);
            int next_i = next_nonspace_idx(expr, close+1);

            int keep_whole = 0;

            // 1) 函数定义参数列表：保留括号（包括空参数）
            if (is_function_param_list(expr, i, close)) {
                keep_whole = 1;
            }
            // 2) 调用括号：保留
            else if (is_call_paren(expr, i)) {
                keep_whole = 1;
            }
            // 3) 右侧紧跟 '||'：为可读性保留一层（但仍会把 inner 的外壳剥到单层）
            else {
                if (next_i < n && expr[next_i]=='|'){
                    OpInfo Rpeek = read_op_right(expr, next_i);
                    if (Rpeek.exists && Rpeek.prec==PREC_OR){
                        keep_whole = 1;
                    }
                }
            }

            int remove_last_layer = 0;
            if (!keep_whole){
                // 检查左侧是否是 L> 或 R>，如果是则保留括号（循环/返回语法）
                int protect_for_keyword = 0;
                if (prev_i >= 1 && expr[prev_i] == '>' && 
                    (expr[prev_i-1] == 'L' || expr[prev_i-1] == 'R')) {
                    protect_for_keyword = 1;
                }
                
                if (!protect_for_keyword) {
                    // primary 允许彻底去——满足 z:=(((x))) -> z:=x
                    if (is_primary_expr(inner)){
                        remove_last_layer = 1;
                    }else{
                        int Pin = min_top_level_binary_prec(inner);
                        if (Pin < 0){
                            // 无二元运算符，强绑定，也可去
                            remove_last_layer = 1;
                        }else{
                            OpInfo L = read_op_left(expr, prev_i);
                            OpInfo R = read_op_right(expr, next_i);
                            int left_ok  = (!L.exists) || (L.prec < Pin);
                            int right_ok = (!R.exists) || (R.prec < Pin);
                            if (left_ok && right_ok) remove_last_layer = 1;
                        }
                    }
                }
            }

            if (remove_last_layer){
                int L=(int)strlen(inner);
                memcpy(out+out_idx, inner, (size_t)L);
                out_idx += L;
            }else{
                out[out_idx++]='(';
                int L=(int)strlen(inner);
                memcpy(out+out_idx, inner, (size_t)L);
                out_idx += L;
                out[out_idx++]=')';
            }

            free(inner);
            i = close;
            continue;
        }

        out[out_idx++]=c;
    }

    out[out_idx]='\0';
    return out;
}

/* ---------------- 公开入口：规范化单个语句 ---------------- */

char* normalize_statement_content(const char* stmt){
    if (!stmt) return NULL;

    // 1) 空白紧凑化
    char* compact = normalize_whitespace(stmt);
    if (!compact) return NULL;

    // 2) 多轮括号化简直到稳定
    char* cur = compact;
    for (int iter=0; iter<16; iter++){
        char* nxt = simplify_parens(cur);
        if (!nxt) break;
        if (strcmp(nxt, cur) == 0){ free(nxt); break; }
        if (cur != compact) free(cur);
        cur = nxt;
    }

    if (cur != compact) free(compact);
    return cur;
}
