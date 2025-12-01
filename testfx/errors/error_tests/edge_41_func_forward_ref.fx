// 测试：函数前向引用（应该成功）
main := () {
    result := helper(10)
    print(result)
}

helper := (x) {
    R> x * 2
}
