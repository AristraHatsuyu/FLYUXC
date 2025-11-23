/* 简单测试 forEach 不带括号 */
main := () {
    nums := [1, 2, 3]
    println("With parentheses:")
    L>(nums:n){ println(n) }
    
    println("Without parentheses:")
    L>nums:m{ println(m) }
}
