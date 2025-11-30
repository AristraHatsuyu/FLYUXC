// P2 测试：作用域退出时的内存清理
// 测试函数返回时局部变量的自动释放

// 测试1：简单函数返回
createString := () {
    s := "hello from function"
    R> s
}

// 测试2：多个局部变量
multipleLocals := () {
    a := "string a"
    b := "string b" 
    c := [1, 2, 3]
    R> a
}

// 测试3：嵌套调用
inner := () {
    x := "inner string"
    R> x
}

outer := () {
    y := inner()
    z := "outer string"
    R> y
}

// 测试4：循环中创建的变量
loopTest := () {
    result := "initial"
    L> (3) {
        temp := "loop iteration"
        result = temp
    }
    R> result
}

// 测试5：不返回值的函数
noReturn := () {
    a := "will be released"
    b := [1, 2, 3]
    c := {x: 1, y: 2}
    // 没有 R> 语句，所有变量应在函数结束时释放
}

// 测试6：参数作为局部变量
withParams := (param1, param2) {
    local := "local var"
    combined := param1
    R> combined
}

main := () {
    println("=== P2 作用域清理测试 ===")
    
    // 测试1
    println("测试1: 简单函数返回")
    r1 := createString()
    println(r1)
    
    // 测试2
    println("测试2: 多个局部变量")
    r2 := multipleLocals()
    println(r2)
    
    // 测试3
    println("测试3: 嵌套调用")
    r3 := outer()
    println(r3)
    
    // 测试4
    println("测试4: 循环中的变量")
    r4 := loopTest()
    println(r4)
    
    // 测试5
    println("测试5: 无返回值函数")
    noReturn()
    println("noReturn completed")
    
    // 测试6
    println("测试6: 带参数的函数")
    r6 := withParams("arg1", "arg2")
    println(r6)
    
    println("=== P2 测试完成 ===")
}
