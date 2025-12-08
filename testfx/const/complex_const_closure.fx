// 测试复杂的闭包和作用域场景
main := () {
    println("=== 测试复杂常量/变量混合场景 ===")
    
    // 外层常量和变量
    MULTIPLIER :(num)= 10
    counter := 0
    
    // 创建闭包
    makeCounter := () {
        // 内层变量
        step := 1
        LIMIT :(num)= 100
        
        // 返回多个函数的对象
        R> {
            inc: () {
                counter = counter + step
                println("counter =", counter, "step =", step, "LIMIT =", LIMIT)
                R> counter
            },
            multiply: () {
                result := counter * MULTIPLIER
                println("result =", result, "MULTIPLIER =", MULTIPLIER)
                R> result
            },
            changeStep: (newStep) {
                step = newStep  // 修改变量
                println("step changed to", step)
            }
        }
    }
    
    counterObj := makeCounter()
    
    counterObj.inc()
    counterObj.inc()
    counterObj.multiply()
    counterObj.changeStep(5)
    counterObj.inc()
    counterObj.multiply()
    
    println("=== 测试嵌套作用域遮蔽 ===")
    
    VALUE :(num)= 1
    x := 100
    
    L> (i := 0; i < 2; i = i + 1) {
        VALUE :(num)= 2  // 遮蔽外层常量
        x := 200         // 遮蔽外层变量
        
        inner := () {
            VALUE :(num)= 3  // 再次遮蔽
            x := 300
            println("最内层: VALUE =", VALUE, "x =", x)
        }
        
        inner()
        println("中间层: VALUE =", VALUE, "x =", x)
    }
    
    println("外层: VALUE =", VALUE, "x =", x)
}
