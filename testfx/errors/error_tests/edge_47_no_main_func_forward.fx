// 测试：无main时函数前向引用（应该成功）
result := helper(10)
print(result)

helper := (x) {
    R> x * 2
}
