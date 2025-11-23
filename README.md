# FLYUX Compiler (FLYUXC AOT)

<div align="center">

![Version](https://img.shields.io/badge/version-0.1-blue.svg)
![C Standard](https://img.shields.io/badge/C-C11-green.svg)
![CMake](https://img.shields.io/badge/CMake-3.10+-orange.svg)
![License](https://img.shields.io/badge/license-MIT-green.svg)

**A Modern Programming Language Compiler with Unicode/Emoji Variable Names**

[Quick Start](#-quick-start) • [Documentation](#-documentation) • [Features](#-features) • [Architecture](#-architecture) • [Contributing](#-contributing)

[中文](README_CN.md) |  **English**

</div>

---

## 🌟 Highlights

### 🎨 Unicode/Emoji Variable Names

FLYUX fully supports Unicode and Emoji as identifiers!

```flyux
// Programming with Emojis!
🚀 := "rocket";
🎯 := 100;

🤪🫵:<num>=(🐙, 🍄) {
    R> 🐙 + 🍄 * 🐙;
};

main := () {
    result := 🤪🫵(5, 3);  // 20
    print(result, 🚀, 🎯);
};
```

### ⚡ High-Performance Compiler

- **Compilation Speed**: ~600ms (first run), ~500ms (subsequent)
- **Binary Size**: 76MB (statically linked LLVM)
- **Zero External Dependencies**: Only system libraries, fully portable

### 🔍 Precise Error Location

```
ERROR: Type mismatch at line 15, column 10
       x:[num]= "string"
                ^^^^^^^^
       Expected: num
       Got:      str
```

**Byte-level** precise error location, supporting multi-byte UTF-8 characters.

### 🧠 Intelligent Type Inference

```flyux
x := 42;              // Automatically inferred as num
name := "Alice";      // Automatically inferred as str
active := true;       // Automatically inferred as bl
```

## 🚀 Quick Start

### Installation

```bash
# 1. Clone the repository
git clone https://github.com/AristraHatsuyu/flyuxc.git
cd flyuxc

# 2. Build
cmake -B build
cmake --build build

# 3. Run tests
./build/flyuxc testfx/valid/basic/demo.fx
```

### Your First Program

Create `hello.fx`:

```flyux
"Hello, FLYUX! 🎉".>print
```

Compile:

```bash
./build/flyuxc hello.fx
```

See **[docs/guides/QUICKSTART.md](docs/guides/QUICKSTART.md)** for more examples.

## 📚 Documentation

### 📖 Main Documents

| Document | Description |
|----------|-------------|
| **[docs/INDEX.md](docs/INDEX.md)** | 📑 **Complete Documentation Index** - Navigation hub for all docs |
| **[docs/guides/QUICKSTART.md](docs/guides/QUICKSTART.md)** | Quick start guide, get started in 5 minutes |
| **[docs/reference/FLYUX_SYNTAX.md](docs/reference/FLYUX_SYNTAX.md)** | Complete syntax specification |
| **[docs/architecture/ARCHITECTURE.md](docs/architecture/ARCHITECTURE.md)** | Compiler architecture design details |

> 💡 More docs (lexical analysis, parser, type system, etc.) at **[docs/INDEX.md](docs/INDEX.md)**

## ✨ Features

### Core Features ✅

- [x] **Complete Compilation Pipeline**
  - [x] Lexical Analysis (Lexer) - Unicode/Emoji support
  - [x] Syntax Analysis (Parser) - Complete AST construction
  - [x] Code Generation (CodeGen) - LLVM IR generation
  - [x] Binary Generation - Native executable files

- [x] **Language Features**
  - [x] Variable declaration and assignment (`x := 10`, `x = 20`)
  - [x] Function definition and calls (`func:<num>=(a,b) { R> a+b }`)
  - [x] Array operations (`[1,2,3]`, `arr[0]`, `arr.>len`)
  - [x] Object operations (`{key: value}`, `obj.key`)
  - [x] Control flow (`if`, `L>` loop, `T>` try-catch)
  - [x] Operators (arithmetic, comparison, logical, bitwise)
  - [x] Method chaining (`.>` operator)

- [x] **Built-in Functions**
  - [x] `print()` / `println()` - Output functions
  - [x] `len()` - Unified length function (string/array/object)
  - [x] Type checking functions

- [x] **Optimization Features**
  - [x] LLVM Static Linking - Zero Homebrew dependencies
  - [x] Embedded Runtime - Single-file distribution
  - [x] Fast Startup - Only native target initialization
  - [x] Fully Portable - Runs on any macOS system

### Technical Highlights 🌟

- **Static Linking**: Single executable with embedded LLVM
- **Zero Dependencies**: Only system libraries (libz, libc++, libSystem, CoreFoundation)
- **Cross-Platform**: macOS (ARM64) native support
- **Modern**: Full Unicode/Emoji identifier support

## 🏗️ Architecture

FLYUXC adopts a classic multi-stage compiler architecture based on LLVM:

```
┌──────────┐    ┌──────────┐    ┌──────────┐    ┌──────────┐
│ Normalize│ →  │  VarMap  │ →  │  Lexer   │ →  │  Parser  │
└──────────┘    └──────────┘    └──────────┘    └──────────┘
                                                       ↓
┌──────────┐    ┌──────────┐    ┌──────────┐
│  Binary  │ ←  │   LLVM   │ ←  │ CodeGen  │
│          │    │  Backend │    │          │
└──────────┘    └──────────┘    └──────────┘
```

### Tech Stack

- **Frontend**: Pure C11 implementation of lexer and parser
- **Backend**: LLVM 20.1.6 (statically linked)
- **Runtime**: Embedded pre-compiled runtime objects
- **Build**: CMake 3.13+

### Static Linking Architecture

```
flyuxc (76MB)
├── LLVM Static Libraries (~70MB)
│   ├── Core, IRReader, Passes
│   └── AArch64 Native Target
├── Compiler Code (~5MB)
└── Runtime Objects (~1MB, embedded)

Dynamic Dependencies (System only):
├── /usr/lib/libz.1.dylib
├── /usr/lib/libc++.1.dylib
├── /usr/lib/libSystem.B.dylib
└── CoreFoundation.framework
```

## 🔧 Development

### System Requirements

- **OS**: macOS (ARM64/x86_64) / Linux / Windows (WSL)
- **Runtime**: No external dependencies (macOS pre-built binary)
- **Build Requirements** (developers only):
  - CMake 3.13+
  - C11 compiler (Clang/GCC)
  - C++17 compiler
  - LLVM 20.x (Homebrew)

### Building

```bash
# Configure
cmake -B build

# Compile
cmake --build build

# Clean
rm -rf build
```

### VS Code Tasks

Convenient VS Code tasks are configured:

- **Cmd+Shift+B**: Build project
- **Tasks: Run Task** → "run": Run tests

### Testing

```bash
# Run test cases
./build/flyuxc testfx/valid/basic/demo.fx
```

## 📖 Syntax Examples

### Variables and Types

```flyux
// Implicit types
name := "Alice";
age := 25;
active := true;

// Explicit types
score:[num]= 95;
username:[str]= "Bob";
```

### Functions

```flyux
// Type-annotated function
add:<num>=(x, y) {
    R> x + y;  // R> = return
};

// Call
result := add(10, 20);

// Method chaining
[1, 2, 3].>len.>println;  // Output: 3
"Hello".>println;          // Output: Hello
```

### Arrays and Objects

```flyux
// Array
fruits := ["apple", "banana", "orange"];

// Object
person := {
    name: "Charlie",
    age: 30,
    hobbies: ["reading", "coding"]
};

// Access
print(person.name);
print(fruits[0]);
```

### Control Flow

```flyux
// If statement
if (x > 10) {
    print("Greater than 10");
} {
    print("Less than or equal to 10");
};

// Loop
L> (i := 0; i < 10; i++) {  // L> = loop
    print(i);
};

// Error handling
T> {
    content := readFile("data.txt")!
    print(content)
} (err) {
    println("Error:", err.message)
}
```

See **[docs/reference/FLYUX_SYNTAX.md](docs/reference/FLYUX_SYNTAX.md)** for complete syntax.

## Contributing

Contributions, bug reports, and suggestions are welcome!

### Contribution Workflow

1. Fork the project
2. Create a feature branch (`git checkout -b feature/amazing`)
3. Commit your changes (`git commit -m 'Add amazing feature'`)
4. Push to the branch (`git push origin feature/amazing`)
5. Create a Pull Request

### Development Guidelines

- Follow C11/C++17 standards
- Code style: 4-space indentation, clear naming
- Documentation: Update relevant `.md` files
- Testing: Add test cases to `testfx/valid/`

See **[CONTRIBUTING.md](CONTRIBUTING.md)** for detailed guidelines.

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

## Acknowledgments

FLYUXC's design is inspired by:

- **Rust Compiler** (rustc) - Modular architecture
- **LLVM** - Compiler infrastructure
- **Go** - Clean syntax design
- **TypeScript** - Type inference system

## Contact

- **Documentation**: `*.md` files in project root
- **Bug Reports**: [GitHub Issues](https://github.com/AristraHatsuyu/FLYUXC/issues)
- **Discussions**: [GitHub Discussions](https://github.com/AristraHatsuyu/FLYUXC/discussions)
