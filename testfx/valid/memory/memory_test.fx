// 测试引用计数内存管理

main := () {
    println("=== 内存管理测试 ===")
    
    // 测试字符串变量
    println("--- 测试1: 字符串变量 ---")
    str1 := "Hello"
    println("str1 = " , str1)
    
    // 重新赋值，应该释放旧值 "Hello"
    str1 = "World"
    println("str1 (重新赋值) = " , str1)
    
    // 再次重新赋值
    str1 = "Final"
    println("str1 (再次赋值) = " , str1)
    
    // 测试2: 数字变量
    println("--- 测试2: 数字变量 ---")
    num1 := 42
    println("num1 = " , num1)
    
    num1 = 100
    println("num1 (重新赋值) = " , num1)
    
    // 测试3: 数组
    println("--- 测试3: 数组 ---")
    arr1 := [1, 2, 3]
    println("arr1 = " , arr1)
    
    // 重新赋值，应该释放旧数组
    arr1 = [4, 5, 6, 7]
    println("arr1 (重新赋值) = " , arr1)
    
    // 测试4: 循环中的变量重新赋值
    println("--- 测试4: 循环中的赋值 ---")
    result := ""
    L> (i := 1; i < 4; i++) {
        result = result + "item" + i + " "
        println("iteration " , i , ": result = " , result)
    }
    println("final result = " , result)
    
    println("=== 测试完成 ===")
}
