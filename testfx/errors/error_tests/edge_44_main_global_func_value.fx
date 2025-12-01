// 测试：有main时全局变量赋函数值（应该失败）
getValue := () {
    R> 42
}
result := getValue()

main := () {
    print(result)
}
