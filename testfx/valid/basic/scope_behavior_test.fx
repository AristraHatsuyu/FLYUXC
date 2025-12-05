// 作用域行为测试 - 记录当前编译器的作用域行为
// 当前行为：块内 := 不创建遮蔽变量，而是修改外部变量
// 这个测试文件记录的是当前实际行为，不是理想行为

main := () {
    println("=== 作用域行为测试 ===")
    
    // 测试1: if块内的 := 
    // 当前行为：修改外部变量，不是遮蔽
    println("\n--- 测试1: if块内 := ---")
    x := 100
    println("if前 x = ", x)  // 100
    if (true) {
        x := 200  // 当前行为：修改外部x
        println("if内 x = ", x)  // 200
    }
    println("if后 x = ", x)  // 当前行为: 200（如果有遮蔽应该是100）
    
    // 测试2: L> for循环内的 :=
    // 当前行为：循环变量和块内变量都泄漏
    println("\n--- 测试2: for循环 := ---")
    y := 100
    println("循环前 y = ", y)  // 100
    L> (i := 0; i < 2; i = i + 1) {
        y := i * 10  // 当前行为：修改外部y
        println("循环内 i=", i, " y=", y)
    }
    println("循环后 y = ", y)  // 当前行为: 10（如果有遮蔽应该是100）
    println("循环后 i = ", i)  // 当前行为: 2（泄漏，理想应报错未定义）
    
    // 测试3: L> foreach循环变量
    // 当前行为：遍历变量泄漏
    println("\n--- 测试3: foreach循环变量 ---")
    item := "外部"
    arr := [1, 2, 3]
    println("遍历前 item = ", item)  // 外部
    L> (arr : item) {  // 当前行为：重用外部item
        println("遍历中 item = ", item)
    }
    println("遍历后 item = ", item)  // 当前行为: 3（如果有遮蔽应该是"外部"）
    
    // 测试4: T>块内的 :=
    // 当前行为：修改外部变量
    println("\n--- 测试4: T>块 := ---")
    z := 100
    println("T>前 z = ", z)  // 100
    T> {
        z := 200  // 当前行为：修改外部z
        println("T>内 z = ", z)  // 200
    }
    println("T>后 z = ", z)  // 当前行为: 200（如果有遮蔽应该是100）
    
    // 测试5: 同级重复 := 
    // 当前行为：不报错，变成赋值
    println("\n--- 测试5: 同级重复 := ---")
    dup := 100
    println("首次 dup = ", dup)  // 100
    dup := 200  // 当前行为：不报错，变成赋值（理想应报错）
    println("再次 dup = ", dup)  // 200
    
    // 测试6: 嵌套块作用域
    println("\n--- 测试6: 嵌套块 ---")
    outer := 100
    if (true) {
        inner := 200
        outer := 300  // 当前行为：修改外部outer
        println("内层 outer = ", outer)  // 300
        println("内层 inner = ", inner)  // 200
    }
    println("外层 outer = ", outer)  // 当前行为: 300
    // println("外层 inner = ", inner)  // 这应该报错未定义（取消注释测试）
    
    println("\n=== 测试完成 ===")
}
