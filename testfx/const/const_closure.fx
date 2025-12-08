// 测试闭包捕获常量
main := () {
    BASE :(num)= 10
    
    makeAdder := (x) {
        // 闭包捕获外层常量
        R> x + BASE
    }
    
    adder := makeAdder(5)
    result := adder
    println("result = ", result)  // 应该输出 15
}
