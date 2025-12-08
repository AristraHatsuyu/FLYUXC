// 测试常量重新赋值错误
main := () {
    PI :(num)= 3.14159
    println("PI = ", PI)
    
    // 这应该报错：常量不能重新赋值
    PI = 2.71828
    println("PI = ", PI)
}
