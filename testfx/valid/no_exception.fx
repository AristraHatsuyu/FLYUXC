// 无异常测试

main := () {
    println("=== 无异常测试 ===\n")
    
    // 字符串
    text := "Hello"
    println("文本:", text)
    println("长度:", len(text))
    println("大写:", upper(text))
    println("")
    
    // 数组
    arr := [1, 2, 3]
    println("数组:", arr)
    
    arr2 := push(arr, 4)
    println("push:", arr2)
    
    arr3 := shift(arr2)
    println("shift:", arr3)
    
    arr4 := pop(arr2)
    println("pop:", arr4)
    println("")
    
    // 对象
    pt := {x: 10, y: 20}
    println("对象:", pt)
    println("x =", pt.x)
    println("y =", pt.y)
    println("")
    
    // 字符串操作
    parts := split("a,b,c", ",")
    println("分割:", parts)
    
    msg := replace("hello", "ll", "yy")
    println("替换:", msg)
    println("")
    
    // 类型转换
    n := toNum("456")
    println("toNum:", n)
    
    i := toInt("789")
    println("toInt:", i)
    println("")
    
    println("✨ 完成 ✨")
    
    R> 0
}
