// 多个闭包共享顶层变量修改
counter := 0
STEP :(num)= 1

inc := () {
    counter = counter + STEP
}

double_inc := () {
    inc()
    inc()
}

inc()
println("After inc():", counter)  // 1

double_inc()
println("After double_inc():", counter)  // 3
