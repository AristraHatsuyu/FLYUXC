# FLYUX Compiler Architecture Design Document

## 1. Overview

```
Source Code (.fx)
       ↓
   Preprocessing (normalize.c)
   - Remove comments
   - Insert semicolons
   - Map identifiers to ASCII (_XXXXX)
   - Output: normalized code, mapping JSON, intermediate representation
       ↓
   Lexical Analysis (lexer.c) [TO BE IMPLEMENTED]
   - Tokenize intermediate representation
   - Token stream with position info
       ↓
   Syntax Analysis (parser.c) [TO BE IMPLEMENTED]
   - Parse token stream
   - Build Abstract Syntax Tree (AST)
       ↓
   Semantic Analysis (semantic.c) [TO BE IMPLEMENTED]
   - Type checking
   - Scope resolution
   - Symbol table management
   - Validate semantics
       ↓
   Code Generation (codegen.c) [TO BE IMPLEMENTED]
   - Convert AST → LLVM IR (.ll)
       ↓
   Output (.ll file)
   [Can be compiled further by LLVM tools: llc, llvm-link, etc.]
```

---

## 2. Lexical Analysis (Lexer)

### 2.1 Token Types

#### Keywords & Language Constructs
- `F>` - Function definition
- `R>` - Return statement
- `L>` - Loop statement
- `if` - Conditional (note: no `else if`, use pattern matching with sequential conditions)
- `:=` - Assignment
- `.>` - Method chaining operator

#### Identifiers
- `IDENTIFIER` - User-defined names (mapped to `_XXXXX` from intermediate representation)
- `NUMBER` - Integer or floating-point literals
- `STRING` - String literals (enclosed in `"..."`)
- `CHAR` - Character literals (enclosed in `'...'`)

#### Operators
- Arithmetic: `+`, `-`, `*`, `/`, `%`
- Comparison: `<`, `>`, `<=`, `>=`, `==`, `!=`
- Logical: `&&`, `||`, `!`
- Bitwise: `&`, `|`, `^`
- Assignment: `:=`
- Method chain: `.>`
- Access: `.`

#### Delimiters
- `(`, `)` - Function call / expression grouping
- `{`, `}` - Code block / scope
- `[`, `]` - Array/list literal or indexing
- `,` - Separator
- `;` - Statement terminator

#### Special
- `EOF` - End of file

### 2.2 Lexer API

```c
typedef struct {
    int type;              // Token type (IDENTIFIER, NUMBER, STRING, etc.)
    char *text;            // Token text (for identifiers, strings, numbers)
    size_t line, column;   // Position in normalized source
    size_t start, end;     // Byte offsets in normalized source
} Token;

typedef struct {
    Token *tokens;
    size_t count;
    size_t capacity;
    size_t current_pos;
} TokenStream;

TokenStream* lexer_tokenize(const char *normalized_code);
void lexer_free(TokenStream *stream);
Token lexer_peek(TokenStream *stream);      // Look at current token
Token lexer_next(TokenStream *stream);      // Consume and return token
bool lexer_match(TokenStream *stream, int token_type);  // Check & consume
```

### 2.3 Lexer Design Notes

- Input: intermediate representation (all identifiers already mapped to `_XXXXX`)
- UTF-8 aware (though most identifiers are now ASCII after mapping)
- Tracks position for error reporting
- Multi-character operators: `.>` must be recognized as single token, not `.` + `>`
- Numbers: support both integers and floats (e.g., `123`, `3.14`)
- Strings/chars: handle escape sequences (`\"`, `\\`, etc.)

---

## 3. Syntax Analysis (Parser)

### 3.1 Grammar (Simplified BNF)

```
program        → declaration* EOF

declaration    → function_decl | statement

function_decl  → F> IDENTIFIER ( param_list? ) block

param_list     → IDENTIFIER ( , IDENTIFIER )*

block          → { statement* }

statement      → expression_stmt
               | assignment_stmt
               | if_stmt
               | loop_stmt
               | return_stmt
               | block

expression_stmt → expression ;

assignment_stmt → IDENTIFIER ( [ expression ] )? := expression ;

if_stmt        → if ( expression ) block
               | ( expression ) block         // continuation of if (no 'else if' keyword)

loop_stmt      → L> loop_form block

loop_form      → [ NUMBER ]                   // repeat N times
               | ( expr ; expr ; expr )       // C-style for loop
               | IDENTIFIER : IDENTIFIER      // for-each: for var : collection

return_stmt    → R> expression ;

expression     → logical_or_expr

logical_or_expr → logical_and_expr ( || logical_and_expr )*

logical_and_expr → equality_expr ( && equality_expr )*

equality_expr  → comparison_expr ( (== | !=) comparison_expr )*

comparison_expr → additive_expr ( (<|>|<=|>=) additive_expr )*   // supports chaining like: a < b <= c

additive_expr  → multiplicative_expr ( (+ | -) multiplicative_expr )*

multiplicative_expr → method_chain_expr ( (* | / | %) method_chain_expr )*

method_chain_expr → unary_expr ( .> IDENTIFIER ( ( arg_list? ) ) )*

unary_expr     → (! | -) ? postfix_expr

postfix_expr   → primary_expr ( [ expression ] | . IDENTIFIER )*

primary_expr   → IDENTIFIER
               | NUMBER
               | STRING
               | CHAR
               | ( expression )
               | array_literal
               | object_literal
               | function_call

function_call  → IDENTIFIER ( arg_list? )

arg_list       → expression ( , expression )*

array_literal  → [ ( expression ( , expression )* ) ? ]

object_literal → { ( key : value ( , key : value )* ) ? }

key            → IDENTIFIER

value          → expression
```

### 3.2 Parser Design Notes

- **Recursive descent parser**: Each grammar rule becomes a function
- **Operator precedence**: Handled by grammar structure (method chaining has high precedence)
- **Error recovery**: Report first syntax error and stop (for simplicity in v1)
- **AST nodes**: Each production creates a corresponding AST node type
- **Associativity**: Left-associative for binary operators

---

## 4. Abstract Syntax Tree (AST)

### 4.1 AST Node Types

```c
// Base node type
typedef struct ASTNode {
    int type;  // NODE_PROGRAM, NODE_FUNCTION, NODE_BLOCK, etc.
    // specific fields depend on node type
} ASTNode;

// Program: root node
typedef struct {
    ASTNode base;
    ASTNode **declarations;  // Array of function/statement nodes
    size_t decl_count;
} ASTProgram;

// Function declaration
typedef struct {
    ASTNode base;
    char *name;
    char **params;  // Parameter names
    size_t param_count;
    ASTNode *body;  // Block statement
} ASTFunction;

// Block statement (sequence of statements)
typedef struct {
    ASTNode base;
    ASTNode **statements;
    size_t stmt_count;
} ASTBlock;

// Assignment: var := expr or var[idx] := expr
typedef struct {
    ASTNode base;
    char *target;      // variable name
    ASTNode *index;    // NULL if not indexed, else expression
    ASTNode *value;    // RHS expression
} ASTAssignment;

// If statement: if (cond) block
typedef struct {
    ASTNode base;
    ASTNode *condition;  // Expression
    ASTNode *then_block; // Block
} ASTIf;

// Loop: L>
typedef struct {
    ASTNode base;
    int loop_type;  // LOOP_REPEAT, LOOP_FOR_RANGE, LOOP_FOR_EACH
    // For LOOP_REPEAT: count (expression)
    // For LOOP_FOR_RANGE: init, cond, update (expressions)
    // For LOOP_FOR_EACH: var (identifier), collection (expression)
    union {
        struct {
            ASTNode *count;
        } repeat;
        struct {
            ASTNode *init;
            ASTNode *condition;
            ASTNode *update;
        } for_range;
        struct {
            char *var;
            ASTNode *collection;
        } for_each;
    } spec;
    ASTNode *body;  // Block
} ASTLoop;

// Return statement: R> expr
typedef struct {
    ASTNode base;
    ASTNode *value;  // Expression to return (NULL if none)
} ASTReturn;

// Binary operation: a op b
typedef struct {
    ASTNode base;
    int operator;  // OP_ADD, OP_SUB, OP_LT, etc.
    ASTNode *left;
    ASTNode *right;
} ASTBinOp;

// Unary operation: op a
typedef struct {
    ASTNode base;
    int operator;  // OP_NEG, OP_NOT
    ASTNode *operand;
} ASTUnaryOp;

// Method chain: obj.>method1(args).>method2(args)
typedef struct {
    ASTNode base;
    ASTNode *object;  // Left-hand side
    char *method;     // Method name
    ASTNode **args;   // Arguments
    size_t arg_count;
} ASTMethodCall;

// Member access: obj.field
typedef struct {
    ASTNode base;
    ASTNode *object;
    char *member;
} ASTMemberAccess;

// Array indexing: arr[index]
typedef struct {
    ASTNode base;
    ASTNode *array;
    ASTNode *index;
} ASTArrayAccess;

// Literal values
typedef struct {
    ASTNode base;
    union {
        long int_val;
        double float_val;
        char *str_val;
    } value;
} ASTLiteral;

// Identifier reference
typedef struct {
    ASTNode base;
    char *name;
} ASTIdentifier;

// Function call: func(args)
typedef struct {
    ASTNode base;
    char *name;
    ASTNode **args;
    size_t arg_count;
} ASTFunctionCall;

// Array literal: [expr, expr, ...]
typedef struct {
    ASTNode base;
    ASTNode **elements;
    size_t element_count;
} ASTArrayLiteral;

// Object literal: {key: val, key: val, ...}
typedef struct {
    ASTNode base;
    char **keys;
    ASTNode **values;
    size_t pair_count;
} ASTObjectLiteral;
```

### 4.2 AST Node IDs

```c
#define NODE_PROGRAM           1
#define NODE_FUNCTION          2
#define NODE_BLOCK             3
#define NODE_ASSIGNMENT        4
#define NODE_IF                5
#define NODE_LOOP              6
#define NODE_RETURN            7
#define NODE_BINOP             8
#define NODE_UNARYOP           9
#define NODE_METHOD_CALL       10
#define NODE_MEMBER_ACCESS     11
#define NODE_ARRAY_ACCESS      12
#define NODE_LITERAL           13
#define NODE_IDENTIFIER        14
#define NODE_FUNCTION_CALL     15
#define NODE_ARRAY_LITERAL     16
#define NODE_OBJECT_LITERAL    17
```

---

## 5. Semantic Analysis

### 5.1 Tasks

1. **Symbol Table Management**
   - Global scope: function definitions, builtin functions
   - Function scope: parameters, local variables
   - Block scope: variables defined in loops/conditionals
   - Track type and location of each symbol

2. **Type Checking**
   - FLYUX is dynamically typed at runtime, but we can infer/check basic types
   - Operations: ensure operands are compatible
   - Function calls: check argument count (arity)
   - Array/object access: ensure index/key operations make sense

3. **Name Resolution**
   - Resolve all identifier references to their declarations
   - Report undefined variable errors
   - Handle shadowing (inner scope overrides outer)

4. **Control Flow Analysis**
   - Ensure return statements are reachable where needed
   - Warn about unreachable code (optional)
   - Validate loop bodies

### 5.2 Semantic Analyzer API

```c
typedef struct {
    char *name;
    int type;        // VAR_TYPE_INT, VAR_TYPE_STR, VAR_TYPE_ARRAY, VAR_TYPE_ANY
    int scope_level;
    // ... other metadata
} Symbol;

typedef struct {
    Symbol *symbols;
    size_t count;
    size_t capacity;
    int current_scope;  // Track nesting level
} SymbolTable;

typedef struct {
    SymbolTable *symtab;
    // Error/warning lists
    char **errors;
    size_t error_count;
} SemanticAnalyzer;

SemanticAnalyzer* semantic_create();
void semantic_analyze(SemanticAnalyzer *sa, ASTNode *ast);
bool semantic_has_errors(SemanticAnalyzer *sa);
void semantic_print_errors(SemanticAnalyzer *sa);
void semantic_free(SemanticAnalyzer *sa);
```

### 5.3 Semantic Analyzer Design Notes

- **Type system**: FLYUX is dynamically typed at runtime
  - For compilation, we treat most values as generic "any" type
  - Special handling for arrays and objects (structure known)
  - Function return types: inferred from R> statements
- **Built-in functions**: `print()`, array methods (`.>length`), etc.
- **Early error detection**: Stop on semantic errors before code generation

---

## 6. Code Generation (to LLVM IR)

### 6.1 Overview

- **Target**: LLVM IR (.ll format, text-based)
- **Strategy**: Walk AST and emit LLVM instructions
- **Memory model**: 
  - Stack: local variables
  - Heap: arrays, objects (allocated with `malloc`)
  - GC: Manual or reference counting (simple for v1)
- **Calling convention**: Follow LLVM standard
- **Runtime support**: Define minimal runtime library for builtin functions

### 6.2 LLVM IR Basics Used

```llvm
; Function definition
define i32 @function_name(i32 %arg1, i8* %arg2) {
entry:
  %local_var = alloca i32
  store i32 42, i32* %local_var
  %val = load i32, i32* %local_var
  
  ; Arithmetic
  %add_result = add i32 %val, 10
  
  ; Comparisons
  %cmp = icmp slt i32 %val, 100
  
  ; Branching
  br i1 %cmp, label %then_block, label %else_block
  
then_block:
  ret i32 1
  
else_block:
  ret i32 0
}
```

### 6.3 Code Generation Strategy

#### 6.3.1 Global Structure
- **Module**: Declare LLVM module with target triple (e.g., `target triple = "x86_64-apple-macosx10.14.0"`)
- **Declare builtin functions**: `printf`, `malloc`, `free`, custom runtime functions
- **Generate function bodies**: For each AST function node

#### 6.3.2 Function Code Generation
```
For each function:
  1. Create LLVM function with name and parameter types
  2. Create entry basic block
  3. Allocate stack space for local variables
  4. Emit code for function body
  5. Add return statement (implicit return 0 if none)
```

#### 6.3.3 Expression Code Generation
```
- Binary operators: emit `add`, `sub`, `mul`, `div`, `icmp`, etc.
- Method calls (.>): Call LLVM function with appropriate signature
- Array/object access: Calculate offset, load/store
- Literals: Emit constant values
- Identifiers: Load from allocated local/parameter
```

#### 6.3.4 Statement Code Generation
```
- Assignment: Evaluate RHS, store to LHS variable
- If: Generate conditional branch with label for then/after blocks
- Loop: Generate loop header, condition check, back-edge
- Return: Emit `ret` instruction
- Block: Generate code for all statements in sequence
```

#### 6.3.5 Type Handling
- **Integers**: `i32` or `i64`
- **Floats**: `double` (f64)
- **Strings**: `i8*` (char pointer)
- **Arrays**: `i8*` (pointer to allocated buffer) or struct with length
- **Objects**: `i8*` (pointer to struct-like data)

### 6.4 Code Generation API

```c
typedef struct {
    FILE *output;  // .ll file handle
    int next_var_id;  // For generating unique variable names
    int next_label_id;  // For generating unique labels
} CodeGenerator;

CodeGenerator* codegen_create(const char *output_file);
void codegen_generate(CodeGenerator *gen, ASTNode *ast);
void codegen_free(CodeGenerator *gen);
```

### 6.5 Runtime Library

**Builtin functions to implement in C and link with LLVM output:**
- `print()` → calls `printf` internally
- `length` property for arrays
- Array methods: `.>length`, etc.
- String operations (if needed)
- Memory management wrappers

---

## 7. Type System

### 7.1 Runtime Type Representation

FLYUX is dynamically typed. At runtime, we use a tagged union:

```c
typedef enum {
    TYPE_NONE,
    TYPE_INT,
    TYPE_FLOAT,
    TYPE_STRING,
    TYPE_ARRAY,
    TYPE_OBJECT,
    TYPE_FUNCTION
} ValueType;

typedef struct {
    ValueType type;
    union {
        long int_val;
        double float_val;
        char *str_val;
        struct { void *data; size_t length; } array;
        struct { void *data; } object;
        void (*func_ptr)(void);
    } value;
} Value;
```

But for code generation to LLVM, we simplify:
- Most values are treated as generic pointers (`i8*`) or integers (`i32`)
- Type conversions handled by runtime

---

## 8. Error Handling & Reporting

### 8.1 Error Levels

1. **Lexer errors**: Invalid tokens, unterminated strings
2. **Parser errors**: Syntax errors (grammar violations)
3. **Semantic errors**: Undefined variables, type mismatches, scope issues
4. **Runtime errors**: Array out of bounds, division by zero (detected at runtime)

### 8.2 Error Messages

Include:
- Error type and message
- Location in source (mapped back to original file with identifier mapping)
- Surrounding context (few lines before/after)

---

## 9. Implementation Order (Recommended)

### Phase 1: Lexer (Week 1)
1. Define token enum and Token structure
2. Implement `lexer_tokenize()` to scan intermediate representation
3. Handle multi-char operators (`.>`, `:=`, `&&`, `||`, etc.)
4. Test with `print.fx` and `demo.fx`
5. **Output**: Token stream for a given input

### Phase 2: Parser (Week 2-3)
1. Define AST node structures
2. Implement recursive descent parser
3. Parse expressions (with correct precedence)
4. Parse statements and blocks
5. Parse function definitions
6. Test with `print.fx` and `demo.fx`
7. **Output**: AST representation

### Phase 3: Semantic Analysis (Week 3)
1. Implement symbol table management
2. Build symbol table during AST walk
3. Validate references and scope
4. Basic type checking
5. Error collection and reporting
6. **Output**: Validated AST + error list

### Phase 4: Code Generation (Week 4-5)
1. Create LLVM module and setup
2. Generate code for literals and identifiers
3. Generate code for binary/unary operations
4. Generate code for function definitions
5. Generate code for statements (assignment, if, loop, return)
6. Generate code for function calls and method chaining
7. Implement runtime library (stubs for `print`, array ops, etc.)
8. Test and refine
9. **Output**: .ll file

### Phase 5: Integration & Polish (Week 5-6)
1. Wire all phases together: preprocessing → lexer → parser → semantic → codegen
2. Add command-line options for intermediate outputs (tokens, AST, semantic results, .ll)
3. Implement LLVM linking (if needed)
4. Comprehensive testing
5. Error message refinement
6. Documentation

---

## 10. Key Design Decisions

1. **Single-pass compilation?**
   - No: Separate lexer, parser, semantic analysis for cleaner design
   - Allows error recovery and better diagnostics

2. **LLVM IR format?**
   - Text (.ll) for readability and debugging
   - Can later pipe to `llc` for native code generation

3. **Dynamic typing?**
   - Yes, FLYUX is dynamically typed
   - Compile-time: treat as generic types
   - Runtime: values carry type information

4. **Memory management?**
   - Manual (malloc/free) for v1
   - Arrays/objects allocated on heap
   - Can add GC later if needed

5. **Optimization?**
   - Minimal for v1; rely on LLVM optimization passes
   - Can add simple passes later (constant folding, dead code elimination)

---

## 11. Testing Strategy

### Unit Tests
- Lexer: Token stream correctness
- Parser: AST structure validation
- Semantic: Symbol table, type checking
- Code Gen: Correct LLVM IR for each construct

### Integration Tests
- End-to-end: Source → .ll file
- Compile .ll with LLVM and run
- Verify output matches expected results

### Test Files
- `testfx/print.fx` - Simple function and print
- `testfx/demo.fx` - Complex code with loops, conditionals, arrays, objects
- Additional edge cases (to be created)

---

## 12. File Structure (Projected)

```
src/
  core/
    cli.c          ✓ (command-line parsing)
    io.c           ✓ (file I/O)
    normalize.c    ✓ (preprocessing)
    mapper.c       ✓ (identifier mapping)
    lexer.c        [TODO]
    parser.c       [TODO]
    ast.c          [TODO: AST creation/destruction]
    semantic.c     [TODO]
    codegen.c      [TODO]
    runtime.c      [TODO: Runtime library]
  frontend/
    (future: optimizations, transformations)
  middle/
    (future: IR passes)
  backend/
    (future: backend-specific code)

include/flyuxc/
    cli.h          ✓
    io.h           ✓
    normalize.h    ✓
    mapper.h       ✓
    lexer.h        [TODO]
    parser.h       [TODO]
    ast.h          [TODO]
    semantic.h     [TODO]
    codegen.h      [TODO]
    common.h       [TODO: shared defs, error handling]

testfx/
    print.fx       ✓
    demo.fx        ✓
    (more tests)   [TODO]
```

---

## Summary

This design provides a solid foundation for a FLYUX compiler:
- **Modular architecture**: Each phase is independent and testable
- **Clear data flow**: Source → Tokens → AST → IR
- **Error handling**: Comprehensive error reporting with source mapping
- **LLVM backend**: Leverage LLVM for optimization and code generation
- **Extensibility**: Easy to add optimizations, more language features, etc.

Next step: Implement lexer (Phase 1) based on this architecture.
