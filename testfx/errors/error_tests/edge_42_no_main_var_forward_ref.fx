// 测试：无main时变量前向引用（应该失败）
x := y + 10
y := 5
print(x)
