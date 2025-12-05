// 测试块外访问循环变量应该报错
main := () {
    L> (i := 0; i < 3; i = i + 1) {
        println("i = ", i)
    }
    println("循环后 i = ", i)  // 这应该报错：Undefined variable 'i'
}
