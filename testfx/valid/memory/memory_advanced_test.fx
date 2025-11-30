// 测试嵌套数据结构的内存管理

main := () {
    println("=== 嵌套数据结构内存测试 ===")
    
    // 测试1: 嵌套数组
    println("--- 测试1: 嵌套数组 ---")
    nested := [[1, 2], [3, 4], [5, 6]]
    println("nested = " , nested)
    
    // 重新赋值嵌套数组
    nested = [[7, 8, 9], [10, 11]]
    println("nested (重新赋值) = " , nested)
    
    // 测试2: 对象
    println("--- 测试2: 对象 ---")
    myObj := {name: "test", value: 42}
    println("myObj.name = " , myObj.name)
    println("myObj.value = " , myObj.value)
    
    // 重新赋值对象
    myObj = {name: "new", value: 100, extra: "data"}
    println("myObj (重新赋值).name = " , myObj.name)
    
    // 测试3: 多次函数调用创建临时值
    println("--- 测试3: 函数返回值 ---")
    makeArray := (n) {
        R> [n, n * 2, n * 3]
    }
    
    result := makeArray(5)
    println("result = " , result)
    
    result = makeArray(10)
    println("result (重新赋值) = " , result)
    
    // 测试4: 数组操作
    println("--- 测试4: 数组操作 ---")
    arr := [1, 2, 3]
    
    // 多次 push
    L> (i := 0; i < 3; i++) {
        arr = arr.>push(i + 10)
        println("after push " , i , ": " , arr)
    }
    
    println("=== 测试完成 ===")
}
