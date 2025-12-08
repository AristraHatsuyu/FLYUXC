// 简单常量捕获测试
STEP :(num)= 1

inc := () {
    counter = counter + STEP
    println("counter =", counter)
}

counter := 0
inc()
inc()
println("Final counter =", counter)