// 测试不使用对象字面量的闭包
main := () {
    println("=== 测试直接闭包 ===")
    
    MULTIPLIER :(num)= 10
    counter := 0
    step := 1
    
    inc := () {
        counter = counter + step
        println("counter =", counter)
        R> counter
    }
    
    multiply := () {
        result := counter * MULTIPLIER
        println("result =", result)
        R> result
    }
    
    inc()
    println("inc called")
    
    multiply()
    println("multiply called")
    
    println("=== 完成 ===")
}
