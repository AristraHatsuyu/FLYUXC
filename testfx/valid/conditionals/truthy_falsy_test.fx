// 测试 if 的 truthy/falsy 行为
// 期望：false, 0, null, undef 为 falsy；其他值为 truthy
// FLYUX 语法：if (cond) { true_branch } { false_branch }

main := () {
    println("=== if truthy/falsy 测试 ===")
    
    // 测试 false
    println("\n--- 测试 false ---")
    if (false) {
        println("false 是 truthy")  // 不应该执行
    } {
        println("false 是 falsy ✓")
    }
    
    // 测试 true
    println("\n--- 测试 true ---")
    if (true) {
        println("true 是 truthy ✓")
    } {
        println("true 是 falsy")
    }
    
    // 测试数字 0
    println("\n--- 测试 0 ---")
    if (0) {
        println("0 是 truthy")
    } {
        println("0 是 falsy ✓")
    }
    
    // 测试非零数字
    println("\n--- 测试 1 ---")
    if (1) {
        println("1 是 truthy ✓")
    } {
        println("1 是 falsy")
    }
    
    // 测试 null
    println("\n--- 测试 null ---")
    if (null) {
        println("null 是 truthy")
    } {
        println("null 是 falsy ✓")
    }
    
    // 测试 undef
    println("\n--- 测试 undef ---")
    if (undef) {
        println("undef 是 truthy")
    } {
        println("undef 是 falsy ✓")
    }
    
    // 测试空字符串
    println("\n--- 测试空字符串 '' ---")
    if ("") {
        println("空字符串 是 truthy")
    } {
        println("空字符串 是 falsy ✓")
    }
    
    // 测试非空字符串
    println("\n--- 测试非空字符串 ---")
    if ("hello") {
        println("非空字符串 是 truthy ✓")
    } {
        println("非空字符串 是 falsy")
    }
    
    // 测试空数组
    println("\n--- 测试空数组 [] ---")
    if ([]) {
        println("空数组 是 truthy ✓")  // 通常数组（即使空）也是 truthy
    } {
        println("空数组 是 falsy")
    }
    
    // 测试可选链 - 存在的属性
    println("\n--- 测试可选链 - 存在 ---")
    myObj := { name: "test", value: 42 }
    if (myObj?.name) {
        println("myObj?.name 存在 ✓")
    } {
        println("myObj?.name 不存在")
    }
    
    // 测试可选链 - 不存在的属性
    println("\n--- 测试可选链 - 不存在 ---")
    if (myObj?.missing) {
        println("myObj?.missing 存在")
    } {
        println("myObj?.missing 不存在 (undef) ✓")
    }
    
    // 测试可选链 - null 对象
    println("\n--- 测试可选链 - null 对象 ---")
    nullObj:[obj] = null
    if (nullObj?.prop) {
        println("nullObj?.prop 存在")
    } {
        println("nullObj?.prop 不存在 (null 短路) ✓")
    }
    
    println("\n=== 测试完成 ===")
}
