// 测试错误对象的详细字段

println("=== 测试错误对象字段访问 ===")
T> {
    x := toNum("abc")
    println("这行不应该输出")
} (err) {
    println("捕获到错误对象:")
    println("  - message:", err.message)
    println("  - code:", err.code)
    println("  - type:", err.type)
}

println("\n=== 测试多条语句，第二条出错 ===")
T> {
    a := 10
    println("第一条语句执行成功, a =", a)
    b := toNum("invalid")
    println("这行不应该输出, b =", b)
    c := 20
    println("这行也不应该输出, c =", c)
} (err) {
    println("在第二条语句时停止")
    println("错误类型:", err.type)
    println("错误代码:", err.code)
}

println("\n=== 测试全部执行成功（无错误） ===")
T> {
    x := 1
    println("语句1: x =", x)
    y := 2
    println("语句2: y =", y)
    z := 3
    println("语句3: z =", z)
} (err) {
    println("不应该进入catch块")
}

println("\n=== 所有测试完成 ===")
