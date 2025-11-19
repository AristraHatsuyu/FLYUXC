println("=== 测试新的颜色策略 ===\n")

// 1. 直接字符串 - 不应该有颜色
println("1. 直接字符串测试：")
print("这是一个直接的字符串，不应该有颜色")
println("")

// 2. 数组中的字符串 - 应该有颜色
println("\n2. 数组中的字符串测试：")
arr := ["红褐色", "字符串", "在数组中"]
print(arr)
println("")

// 3. 对象中的字符串 - 应该有颜色
println("\n3. 对象中的字符串测试：")
person := { name: "Alice", city: "北京" }
print(person)
println("")

// 4. printf 的参数字符串 - 应该有颜色
println("\n4. printf 参数字符串测试：")
printf("Name: %s, Age: %d\n", "Bob", 25)

// 5. 混合测试
println("\n5. 混合测试：")
println("这是直接字符串（无色）")
data := {
    title: "测试对象",
    items: ["项目1", "项目2", "项目3"],
    count: 3
}
print(data)
println("")

// 6. 数字和布尔值应该保持有颜色
println("\n6. 数字和布尔值测试：")
print(42)
println("")
print(3.14159)
println("")
print(true)
println("")
print(false)
println("")

println("\n=== 测试完成 ===")
