// 综合测试 ! 后缀的错误处理机制

println("=== ! 后缀错误处理综合测试 ===\n")

// 测试1: 类型转换 - toNum
println("--- 测试1: toNum ---")
result1 := toNum("abc")  // 无 ! 返回带类型 null
println("toNum(\"abc\") =", result1)
println("类型:", typeOf(result1))
println("程序继续\n")

T> {
    result2 := toNum("xyz")!  // 有 ! 抛出异常
    println("不应该执行到这里")
} (err) {
    println("捕获到 toNum! 错误:", err)
}
println()

// 测试2: 文件读取 - readFile
println("--- 测试2: readFile ---")
content1 := readFile("/nonexistent/file.txt")  // 无 !
println("readFile(不存在的文件) =", content1)
println("类型:", typeOf(content1))
println("程序继续\n")

T> {
    content2 := readFile("/nonexistent/file.txt")!  // 有 !
    println("不应该执行到这里")
} (err) {
    println("捕获到 readFile! 错误:", err)
}
println()

// 测试3: JSON 解析 - parseJSON
println("--- 测试3: parseJSON ---")
json1 := parseJSON("invalid")  // 无 !
println("parseJSON(\"invalid\") =", json1)
println("类型:", typeOf(json1))
println("程序继续\n")

T> {
    json2 := parseJSON("invalid")!  // 有 !
    println("不应该执行到这里")
} (err) {
    println("捕获到 parseJSON! 错误:", err)
}
println()

// 测试4: 正常情况，无论有无 !
println("--- 测试4: 正常情况 ---")
num1 := toNum("123")
println("toNum(\"123\") =", num1)

num2 := toNum("456")!
println("toNum(\"456\")! =", num2)

json3 := parseJSON('{"a":1}')!
println("parseJSON(有效)! =", json3)
println()

println("=== 所有测试完成 ===")
