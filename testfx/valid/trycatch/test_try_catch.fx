// 测试T>异常处理系统

// 测试1: 基本的try-catch
println("=== 测试1: 基本try-catch ===")
T> {
    x := toNum("abc")  // 这会产生错误
    println("Try块：x =", x)
} (err) {
    println("Catch块：捕获到错误")
    println("错误信息:", err)
}

// 测试2: 没有错误的情况
println("\n=== 测试2: 没有错误 ===")
T> {
    y := toNum("123")
    println("Try块：y =", y)
} (err) {
    println("Catch块：不应该执行")
}

// 测试3: try-finally（没有catch）
println("\n=== 测试3: try-finally ===")
T> {
    z := toNum("456")
    println("Try块：z =", z)
} {
    println("Finally块：总是执行")
}

// 测试4: try-catch-finally
println("\n=== 测试4: try-catch-finally ===")
T> {
    a := toNum("invalid")
    println("Try块：a =", a)
} (e) {
    println("Catch块：错误 -", e)
} {
    println("Finally块：清理资源")
}

// 测试5: finally在无错误时也执行
println("\n=== 测试5: finally在无错误时执行 ===")
T> {
    b := toNum("789")
    println("Try块：b =", b)
} (e) {
    println("Catch块：不应该执行")
} {
    println("Finally块：清理完成")
}

// 测试6: 只有try块（最简单形式）
println("\n=== 测试6: 只有try块 ===")
T> {
    c := 100
    println("Try块：c =", c)
}

println("\n=== 所有测试完成 ===")
