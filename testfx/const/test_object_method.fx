// 测试对象方法调用
main := () {
    println("=== 测试对象方法 ===")
    
    MULTIPLIER :(num)= 10
    counter := 0
    
    makeCounter := () {
        step := 1
        
        R> {
            inc: () {
                counter = counter + step
                println("counter =", counter)
                R> counter
            },
            multiply: () {
                result := counter * MULTIPLIER
                println("result =", result)
                R> result
            }
        }
    }
    
    myObj := makeCounter()
    println("obj created")
    
    myObj.inc()
    println("inc called")
    
    myObj.multiply()
    println("multiply called")
}
