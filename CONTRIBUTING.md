# 贡献指南

> 感谢你对 FLYUX 编译器项目的关注！

---

## 🤝 如何贡献

FLYUXC 欢迎各种形式的贡献，包括但不限于：

- 🐛 报告 Bug
- 💡 提出新功能建议
- 📝 改进文档
- 🔧 提交代码修复
- ✨ 实现新特性
- 🧪 编写测试用例

---

## 📋 开始之前

### 环境准备

1. **Fork 项目**
   ```bash
   # 在 GitHub 上点击 Fork 按钮
   ```

2. **克隆你的 Fork**
   ```bash
   git clone https://github.com/YOUR_USERNAME/FLYUXC.git
   cd FLYUXC
   ```

3. **添加上游仓库**
   ```bash
   git remote add upstream https://github.com/AristraHatsuyu/FLYUXC.git
   ```

4. **安装依赖**
   ```bash
   # macOS
   brew install llvm cmake

   # Linux (Ubuntu/Debian)
   sudo apt-get install llvm-20 cmake build-essential
   ```

5. **构建项目**
   ```bash
   cmake -B build
   cmake --build build
   ```

---

## 🐛 报告 Bug

### 在提交 Issue 前

1. **搜索现有 Issues**: 确认问题是否已被报告
2. **使用最新版本**: 确保使用最新的 `main` 分支
3. **提供复现步骤**: 详细描述如何触发问题

### Bug 报告模板

```markdown
**描述**
简洁清晰地描述 Bug

**复现步骤**
1. 创建文件 'test.fx'
2. 编写代码 '...'
3. 运行命令 './build/flyuxc test.fx'
4. 观察错误

**预期行为**
描述预期应该发生什么

**实际行为**
描述实际发生了什么

**环境信息**
- OS: [e.g., macOS 14.0, Ubuntu 22.04]
- 编译器版本: [e.g., 0.1.0]
- LLVM 版本: [e.g., 20.1.6]

**附加信息**
- 错误消息
- 源代码文件
- 编译输出
```

---

## 💡 功能建议

### 提交建议前

1. **检查路线图**: 查看 [docs/STATUS.md](docs/STATUS.md)
2. **搜索现有建议**: 避免重复提议
3. **思考可行性**: 考虑实现复杂度和兼容性

### 功能建议模板

```markdown
**功能描述**
清晰描述你想要的功能

**使用场景**
为什么需要这个功能？解决什么问题？

**示例代码**
```flyux
// 展示期望的语法
x := newFeature(arg);
```

**替代方案**
是否有其他实现方式？

**优先级**
- [ ] 必需 (Critical)
- [ ] 重要 (High)
- [ ] 一般 (Medium)
- [ ] 可选 (Low)
```

---

## 🔧 代码贡献

### 工作流程

1. **创建分支**
   ```bash
   git checkout -b feature/your-feature-name
   # 或
   git checkout -b fix/bug-description
   ```

2. **编写代码**
   - 遵循代码规范（见下文）
   - 添加必要的注释
   - 更新相关文档

3. **编写测试**
   ```bash
   # 在 testfx/valid/ 下创建测试文件
   vim testfx/valid/your_test.fx
   
   # 运行测试
   ./build/flyuxc testfx/valid/your_test.fx
   ```

4. **提交更改**
   ```bash
   git add .
   git commit -m "feat: add your feature description"
   ```

5. **推送分支**
   ```bash
   git push origin feature/your-feature-name
   ```

6. **创建 Pull Request**
   - 访问 GitHub 仓库
   - 点击 "New Pull Request"
   - 填写 PR 模板
   - 等待 Review

### 提交信息规范

遵循 [Conventional Commits](https://www.conventionalcommits.org/):

```
<type>(<scope>): <subject>

<body>

<footer>
```

**Type 类型**:
- `feat`: 新功能
- `fix`: Bug 修复
- `docs`: 文档更新
- `style`: 代码格式（不影响功能）
- `refactor`: 重构
- `test`: 测试相关
- `chore`: 构建/工具相关

**示例**:
```bash
feat(parser): add support for ternary operator

Implement ternary operator (condition ? true_val : false_val)
with proper precedence and type checking.

Closes #123
```

---

## 📝 代码规范

### C 代码风格

```c
// 1. 4 空格缩进
void function_name(int param) {
    if (condition) {
        // code
    }
}

// 2. 函数命名: snake_case
int calculate_sum(int a, int b);

// 3. 结构体: PascalCase
typedef struct Token {
    TokenType type;
    char* value;
} Token;

// 4. 常量: UPPER_CASE
#define MAX_BUFFER_SIZE 1024

// 5. 指针: 靠近类型
int* ptr;
char* str;

// 6. 注释: 清晰描述意图
// 解析表达式并返回 AST 节点
ASTNode* parse_expression(Parser* parser);
```

### C++ 代码风格

```cpp
// 1. 类命名: PascalCase
class LLVMCodeGen {
public:
    // 2. 方法命名: camelCase
    void generateIR(ASTNode* node);
    
private:
    // 3. 成员变量: m_ 前缀
    llvm::Module* m_module;
};

// 4. 命名空间
namespace flyux {
namespace codegen {
    // code
}
}
```

### 文件组织

```
src/
├── frontend/
│   ├── lexer/
│   │   ├── lexer.h          # 头文件
│   │   ├── lexer.c          # 实现
│   │   └── tests/           # 测试（如需要）
│   └── parser/
│       ├── parser.h
│       └── parser.c
└── backend/
    ├── codegen/
    │   ├── codegen.h
    │   └── codegen.c
    └── runtime/
        └── value_runtime.c
```

---

## 🧪 测试

### 添加测试用例

1. **有效测试** - 应该成功编译
   ```bash
   # 创建在 testfx/valid/<category>/
   testfx/valid/basic/your_test.fx
   ```

2. **无效测试** - 应该产生错误
   ```bash
   # 创建在 testfx/invalid/<category>/
   testfx/invalid/syntax/your_error_test.fx
   ```

3. **运行测试**
   ```bash
   # 单个测试
   ./build/flyuxc testfx/valid/basic/your_test.fx
   
   # 批量测试（如有测试脚本）
   ./run_tests.sh
   ```

### 测试覆盖

贡献的新功能应包含：
- ✅ 基本功能测试
- ✅ 边界情况测试
- ✅ 错误处理测试

---

## 📚 文档

### 更新文档

当你的代码涉及以下变更时，请更新相应文档：

| 变更类型 | 需要更新的文档 |
|---------|--------------|
| 新增语法 | `docs/FLYUX_SYNTAX.md` |
| API 变更 | `docs/ARCHITECTURE.md`, 相关模块文档 |
| 架构修改 | `docs/ARCHITECTURE.md` |
| 功能完成 | `docs/STATUS.md`, `docs/PROGRESS.md` |
| Bug 修复 | `CHANGELOG.md` |

### 文档风格

- 使用 Markdown 格式
- 包含代码示例
- 清晰的标题层级
- 适当的表格和列表

---

## 🔍 代码审查

### 审查标准

你的 PR 将根据以下标准审查：

- ✅ **功能正确性**: 代码是否按预期工作
- ✅ **代码质量**: 是否遵循规范，可读性好
- ✅ **测试覆盖**: 是否有充分的测试
- ✅ **文档完整**: 是否更新了相关文档
- ✅ **无破坏性**: 是否影响现有功能
- ✅ **性能考虑**: 是否有性能影响

### 响应反馈

- 及时回复审查意见
- 讨论不同的实现方案
- 根据建议修改代码
- 保持友好和专业

---

## 🌟 成为维护者

活跃贡献者可能被邀请成为项目维护者，获得：

- 直接提交权限
- 参与项目决策
- 审查他人 PR
- 在 README 中列名

---

## 📞 联系方式

- **GitHub Issues**: [提交问题](https://github.com/AristraHatsuyu/FLYUXC/issues)
- **GitHub Discussions**: [讨论区](https://github.com/AristraHatsuyu/FLYUXC/discussions)
- **Email**: [联系维护者]

---

## 📜 行为准则

参与项目即表示同意遵守我们的行为准则：

- 🤝 尊重他人
- 💬 建设性沟通
- 🎯 专注于问题本身
- ❤️ 欢迎新手
- 🚫 禁止骚扰和歧视

---

## 🙏 致谢

感谢每一位贡献者让 FLYUXC 变得更好！

你的贡献将被记录在：
- [CHANGELOG.md](CHANGELOG.md)
- GitHub Contributors 页面
- 项目文档中

---

<div align="center">

**开始贡献**: [Fork 项目](https://github.com/AristraHatsuyu/FLYUXC/fork) | [查看 Issues](https://github.com/AristraHatsuyu/FLYUXC/issues)

[⬆ 回到顶部](#贡献指南)

</div>
