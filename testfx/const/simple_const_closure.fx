// 简化的闭包测试
main := () {
    println("=== 测试常量捕获 ===")
    
    MULTIPLIER :(num)= 10
    counter := 0
    
    inc := () {
        counter = counter + 1
        println("counter =", counter)
        R> counter
    }
    
    multiply := () {
        result := counter * MULTIPLIER
        println("result =", result)
        R> result
    }
    
    inc()
    inc()
    multiply()
    
    println("=== 完成 ===")
}
