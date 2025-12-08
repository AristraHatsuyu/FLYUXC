// 嵌套闭包捕获顶层常量
MULTIPLIER :(num)= 10

createAdder := (base) {
    add := (x) {
        R> (base + x) * MULTIPLIER  // 捕获参数和全局常量
    }
    R> add
}

adder := createAdder(5)
result := adder(3)
println("Result:", result, "Expected: 80")
