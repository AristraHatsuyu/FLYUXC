// 基础测试

main := () {
    println("=== 基础测试 ===\n")
    
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
    println("")
    
    // 类型转换
    T> {
        val := toNum("123")
        println("toNum:", val)
    } (err) {
        println("错误:", err.message)
    }
    println("")
    
    // 对象
    pt := {x: 10, y: 20}
    println("对象:", pt)
    println("x =", pt.x)
    println("")
    
    println("完成")
}
