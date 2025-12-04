// 完整测试：匿名函数、对象/数组内函数、闭包

println("=== Test 1: Anonymous Function in Object ===")
calculator := {
    add: (a, b) {
        R> a + b
    },
    sub: (a, b) {
        R> a - b
    },
    multiply: (a, b) {
        R> a * b
    }
}
println("calculator.add(10, 5): " + calculator.add(10, 5))
println("calculator.sub(10, 5): " + calculator.sub(10, 5))
println("calculator.multiply(10, 5): " + calculator.multiply(10, 5))

println("\n=== Test 2: Anonymous Function in Array ===")
funcs := [
    (x) { R> x * 2 },
    (x) { R> x * 3 },
    (x) { R> x + 100 }
]
println("funcs[0](5): " + funcs[0](5))
println("funcs[1](5): " + funcs[1](5))
println("funcs[2](5): " + funcs[2](5))

println("\n=== Test 3: Closure Returning Function ===")
makeMultiplier := (factor) {
    R> (x) {
        R> x * factor
    }
}
times2 := makeMultiplier(2)
times5 := makeMultiplier(5)
println("times2(7): " + times2(7))
println("times5(7): " + times5(7))

println("\n=== Test 4: Closure in Object ===")
factory := {
    createAdder: (base) {
        R> (x) {
            R> base + x
        }
    }
}
add10 := factory.createAdder(10)
println("add10(5): " + add10(5))
println("factory.createAdder(100)(25): " + factory.createAdder(100)(25))

println("\n=== Test 5: typeOf Check (should be 'func') ===")
myFunc := (x) { R> x }
println("typeOf(myFunc): " + typeOf(myFunc))
println("typeOf(calculator.add): " + typeOf(calculator.add))
println("typeOf(funcs[0]): " + typeOf(funcs[0]))
println("typeOf(makeMultiplier): " + typeOf(makeMultiplier))
println("typeOf(times2): " + typeOf(times2))

println("\n=== Test 6: Nested Closures ===")
createCounter := () {
    count := 0
    R> {
        increment: () {
            count = count + 1
            R> count
        },
        getCount: () {
            R> count
        }
    }
}
counter := createCounter()
println("counter.increment(): " + counter.increment())
println("counter.increment(): " + counter.increment())
println("counter.getCount(): " + counter.getCount())

println("\n=== All tests completed ===")
