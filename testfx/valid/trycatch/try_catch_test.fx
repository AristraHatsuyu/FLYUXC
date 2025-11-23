// Try-catch测试

main := () {
    println("=== Try-Catch 测试 ===\n")
    
    // Test 1: 捕获错误
    println("测试1: 应该捕获TypeError")
    T> {
        bad := toNum("abc")
        println("❌ 不应该执行这行")
    } (e) {
        println("✅ 捕获到错误:", e.message)
        println("   错误类型:", e.type)
        println("   错误代码:", e.code)
    }
    println("")
    
    // Test 2: 正常执行
    println("测试2: 不应该进入catch")
    T> {
        good := toNum("456")
        println("✅ 正常执行, 结果:", good)
    } (e) {
        println("❌ 不应该捕获")
    }
    println("")
    
    // Test 3: 嵌套try-catch
    println("测试3: 嵌套异常")
    T> {
        println("外层try")
        T> {
            println("内层try")
            err := toNum("xyz")
            println("❌ 内层不应该执行")
        } (inner) {
            println("✅ 内层catch:", inner.message)
        }
        println("✅ 外层继续执行")
    } (outer) {
        println("❌ 外层不应该catch")
    }
    println("")
    
    println("✨ 所有测试完成 ✨")
}
