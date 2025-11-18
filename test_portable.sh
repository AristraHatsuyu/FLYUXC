#!/bin/bash
# 测试编译器的可移植性

echo "=== FLYUXC 可移植性测试 ==="
echo ""

# 检查 clang 是否存在
if ! command -v clang &> /dev/null; then
    echo "❌ 错误: 系统未安装 clang"
    echo "   macOS: clang 已预装"
    echo "   Linux: sudo apt install clang"
    echo "   或者: sudo yum install clang"
    exit 1
fi

echo "✓ clang 已安装: $(clang --version | head -1)"
echo ""

# 创建测试目录
TEST_DIR="/tmp/flyuxc_portable_test_$$"
mkdir -p "$TEST_DIR"
echo "✓ 创建测试目录: $TEST_DIR"

# 复制编译器
cp build/flyuxc "$TEST_DIR/"
echo "✓ 复制编译器 ($(ls -lh build/flyuxc | awk '{print $5}'))"

# 创建测试程序
cat > "$TEST_DIR/test.fx" << 'EOF'
main() {
    a := 10;
    b := 20;
    c := a + b;
    print(c);
}
EOF
echo "✓ 创建测试程序"
echo ""

# 编译
echo "=== 开始编译 ==="
cd "$TEST_DIR"
./flyuxc test.fx 2>&1 | grep -E "(Using embedded|Executable generated|Status:)"

# 运行
echo ""
echo "=== 运行结果 ==="
if [ -f ./test ]; then
    RESULT=$(./test)
    if [ "$RESULT" = "30" ]; then
        echo "✅ 输出正确: $RESULT"
        SUCCESS=1
    else
        echo "❌ 输出错误: $RESULT (期望: 30)"
        SUCCESS=0
    fi
else
    echo "❌ 可执行文件未生成"
    SUCCESS=0
fi

# 清理
cd - > /dev/null
rm -rf "$TEST_DIR"
echo ""
echo "✓ 清理测试目录"
echo ""

if [ $SUCCESS -eq 1 ]; then
    echo "🎉 可移植性测试通过!"
    echo ""
    echo "总结:"
    echo "  - flyuxc 编译器只需要系统的 clang"
    echo "  - 无需其他依赖文件"
    echo "  - 可以复制到任何位置使用"
    exit 0
else
    echo "❌ 可移植性测试失败"
    exit 1
fi
