// 测试常量作用域和遮蔽
main := () {
    MAX :(num)= 100
    println("外层 MAX = ", MAX)
    
    if (true) {
        MAX :(num)= 50  // 内层作用域的新常量（遮蔽外层）
        println("内层 MAX = ", MAX)
    }
    
    println("返回外层 MAX = ", MAX)
}
