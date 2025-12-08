// 测试简单的嵌套作用域
main := () {
    VALUE :(num)= 1
    println("外层 VALUE =", VALUE)
    
    L> (i := 0; i < 1; i = i + 1) {
        VALUE :(num)= 2
        println("循环中 VALUE =", VALUE)
        
        inner := () {
            VALUE :(num)= 3
            println("inner 中 VALUE =", VALUE)
        }
        
        inner()
    }
}
