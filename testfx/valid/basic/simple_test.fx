// 简化测试
println("=== FLYUX 功能测试 ===\n")

// 字符串操作
text := "Hello FLYUX"
println("原文:", text)
println("长度:", len(text))
println("大写:", upper(text))
println("小写:", lower(text))

// 数组操作
arr := [1, 2, 3]
println("\n数组:", arr)
arr2 := push(arr, 4)
println("push(4):", arr2)

// 类型转换
T> {
    value := toNum("123")
    println("\ntoNum('123'):", value)
} (err) {
    println("错误:", err.message)
}

// 对象
point := {x: 10, y: 20}
println("\n对象:", point)
println("x =", point.x)
println("y =", point.y)

println("\n=== 测试完成 ===")
