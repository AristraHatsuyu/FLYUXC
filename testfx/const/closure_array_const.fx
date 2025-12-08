// 闭包数组与顶层常量
OFFSET :(num)= 100

funcs := []
i := 0
L> (3) {
    // 注意：需要使用局部变量才能正确捕获
    val := i
    func := () {
        R> val + OFFSET
    }
    funcs[i] = func
    i = i + 1
}

result0 := funcs[0]()
result1 := funcs[1]()
result2 := funcs[2]()

println("Results:", result0, result1, result2)
println("Expected: 100 101 102")
