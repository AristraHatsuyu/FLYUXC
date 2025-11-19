// 测试类型转换函数

println("╔════════════════════════════════════════╗")
println("║     类型转换函数测试                   ║")
println("╚════════════════════════════════════════╝")
println()

// === toNum 测试 ===
println("=== toNum() 测试 ===")

// 字符串转数字
num1 := toNum("123")
println("toNum(\"123\") = ", num1, ", 状态: ", isOk())

num2 := toNum("3.14")
println("toNum(\"3.14\") = ", num2, ", 状态: ", isOk())

// 错误的字符串
num3 := toNum("abc")
println("toNum(\"abc\") = ", num3, ", 状态: ", isOk())
if (!isOk()) {
    println("  错误: ", lastError())
}
clearError()

// 布尔转数字
num4 := toNum(true)
println("toNum(true) = ", num4)

num5 := toNum(false)
println("toNum(false) = ", num5)

println()

// === toStr 测试 ===
println("=== toStr() 测试 ===")

str1 := toStr(123)
println("toStr(123) = \"", str1, "\"")

str2 := toStr(3.14)
println("toStr(3.14) = \"", str2, "\"")

str3 := toStr(true)
println("toStr(true) = \"", str3, "\"")

str4 := toStr(null)
println("toStr(null) = \"", str4, "\"")

println()

// === toBl 测试 ===
println("=== toBl() 测试 ===")

bl1 := toBl(1)
println("toBl(1) = ", bl1)

bl2 := toBl(0)
println("toBl(0) = ", bl2)

bl3 := toBl("hello")
println("toBl(\"hello\") = ", bl3)

bl4 := toBl("")
println("toBl(\"\") = ", bl4)

bl5 := toBl(null)
println("toBl(null) = ", bl5)

println()

// === 实际应用示例 ===
println("=== 实际应用示例 ===")

// 读取输入并转换为数字
age_str := input("请输入年龄: ")
if (isOk()) {
    age := toNum(age_str)
    if (isOk()) {
        println("你输入的年龄是: ", age)
        println("明年你将是: ", age + 1, " 岁")
    } {
        println("年龄格式错误: ", lastError())
    }
}

println()
println("✓ 测试完成!")
