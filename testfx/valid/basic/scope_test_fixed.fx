// 作用域行为测试 - 测试正确的块级作用域
// 预期行为：
// 1. 内层 := 创建遮蔽变量，不修改外部变量
// 2. 循环变量不泄漏到循环外
// 3. 同级重复 := 应该报编译错误

main := () {
    println("=== 块级作用域测试 ===")
    
    // 测试1: if块内的 := 遮蔽外部变量
    println("\n--- 测试1: if块内变量遮蔽 ---")
    x := 100
    println("if前 x = ", x)  // 100
    if (true) {
        x := 200  // 应该遮蔽外部x，创建新变量
        println("if内 x = ", x)  // 200
    }
    println("if后 x = ", x)  // 应该还是 100（遮蔽不影响外部）
    
    // 测试2: for循环的作用域
    println("\n--- 测试2: for循环作用域 ---")
    y := 100
    println("循环前 y = ", y)  // 100
    L> (i := 0; i < 3; i = i + 1) {
        y := i * 10  // 遮蔽外部y
        println("循环内 i=", i, " y=", y)
    }
    println("循环后 y = ", y)  // 应该还是 100
    // println("循环后 i = ", i)  // 会报错: Undefined variable 'i'
    
    // 测试3: foreach循环的作用域
    println("\n--- 测试3: foreach循环作用域 ---")
    item := "外部值"
    arr := [1, 2, 3]
    println("遍历前 item = ", item)  // 外部值
    L> (arr : elem) {  // elem是循环局部变量
        println("遍历中 elem = ", elem)
    }
    println("遍历后 item = ", item)  // 应该还是 "外部值"
    // println("elem = ", elem)  // 会报错: Undefined variable 'elem'
    
    // 测试4: T>块的作用域
    println("\n--- 测试4: T>块作用域 ---")
    z := 100
    println("T>前 z = ", z)  // 100
    T> {
        z := 200  // 遮蔽外部z
        println("T>内 z = ", z)  // 200
    }
    println("T>后 z = ", z)  // 应该还是 100
    
    // 测试5: 嵌套遮蔽
    println("\n--- 测试5: 嵌套遮蔽 ---")
    val := 1
    println("第1层 val = ", val)  // 1
    if (true) {
        val := 2
        println("第2层 val = ", val)  // 2
        if (true) {
            val := 3
            println("第3层 val = ", val)  // 3
        }
        println("回到第2层 val = ", val)  // 2
    }
    println("回到第1层 val = ", val)  // 1
    
    // 测试6: 块内变量不泄漏
    println("\n--- 测试6: 块内变量不泄漏 ---")
    if (true) {
        inner_var := 999
        println("块内 inner_var = ", inner_var)  // 999
    }
    // println("块外 inner_var = ", inner_var)  // 会报错
    
    println("\n=== 所有测试完成 ===")
}
