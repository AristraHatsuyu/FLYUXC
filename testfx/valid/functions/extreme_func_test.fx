// 极限测试：打印函数、循环调用、复杂场景

println("=== Test 1: Print Function Directly ===")
myFunc := (x) { R> x * 2 }
println("Printing function directly:")
println(myFunc)

println("\n=== Test 2: Print Object with Function ===")
myObj := {
    name: "test",
    action: (x) { R> x + 1 }
}
println("Printing object with function:")
println(myObj)

println("\n=== Test 3: Loop Over Function Array with L> ===")
funcs := [
    (x) { R> x + 1 },
    (x) { R> x * 2 },
    (x) { R> x ** 2 }
]
println("Calling each function with 5:")
L> (i := 0; i < 3; i = i + 1) {
    result := funcs[i](5)
    println("funcs[" + i + "](5) = " + result)
}

println("\n=== Test 4: For-each style loop ===")
multipliers := [
    (x) { R> x * 1 },
    (x) { R> x * 2 },
    (x) { R> x * 3 }
]
L> (idx := 0; idx < 3; idx = idx + 1) {
    fn := multipliers[idx]
    println("multipliers[" + idx + "](10) = " + fn(10))
}

println("\n=== Test 5: Nested Functions in Object ===")
calculator := {
    basic: {
        add: (a, b) { R> a + b },
        sub: (a, b) { R> a - b }
    },
    advanced: {
        pow: (a, b) { R> a ** b },
        mod: (a, b) { R> a % b }
    }
}
println("calculator.basic.add(3, 4) = " + calculator.basic.add(3, 4))
println("calculator.advanced.pow(2, 10) = " + calculator.advanced.pow(2, 10))

println("\n=== Test 6: Function Returning Object with Functions ===")
createCalculator := (base) {
    R> {
        getBase: () { R> base },
        addToBase: (x) { R> base + x },
        multiplyBase: (x) { R> base * x }
    }
}
calc := createCalculator(100)
println("calc.getBase() = " + calc.getBase())
println("calc.addToBase(50) = " + calc.addToBase(50))
println("calc.multiplyBase(3) = " + calc.multiplyBase(3))

println("\n=== Test 7: Array of Objects with Functions ===")
items := [
    { name: "A", process: (x) { R> x + "A" } },
    { name: "B", process: (x) { R> x + "B" } },
    { name: "C", process: (x) { R> x + "C" } }
]
L> (j := 0; j < 3; j = j + 1) {
    println(items[j].name + ": " + items[j].process("item_"))
}

println("\n=== Test 8: Chained Method Calls ===")
// 注意：在 FLYUX 中，对象方法内部无法直接引用对象自身（没有 this）
// 但可以通过返回值来实现简单的链式调用
buildValue := ""
buildAppend := (s) {
    buildValue = buildValue + s
    R> s  // 返回添加的字符串
}
buildAppend("Hello")
buildAppend(" ")
buildAppend("World")
println("buildValue = " + buildValue)

println("\n=== Test 9: Function Composition ===")
// compose := (f, g) {
//     R> (x) {
//         R> f(g(x))
//     }
// }
// double := (x) { R> x * 2 }
// addTen := (x) { R> x + 10 }
// doubleAndAddTen := compose(addTen, double)
// println("compose(addTen, double)(5) = " + doubleAndAddTen(5))  // (5*2)+10 = 20
println("(Skipped - complex closure)")

println("\n=== Test 10: Currying ===")
// curry := (f) {
//     R> (a) {
//         R> (b) {
//             R> f(a, b)
//         }
//     }
// }
// add := (a, b) { R> a + b }
// curriedAdd := curry(add)
// add5 := curriedAdd(5)
// println("curriedAdd(5)(3) = " + add5(3))
println("(Skipped - complex closure)")

println("\n=== All extreme tests completed ===")
