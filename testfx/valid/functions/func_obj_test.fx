// Test functions stored in objects and arrays

println("=== Test 1: Function as Object Property ===")
// Define a simple function
greet := (name) {
    R> "Hello, " + name + "!"
}

// Store function in object
person := { name: "Alice", sayHello: greet }
println("person.sayHello('World'):")
result1 := person.sayHello("World")
println(result1)

println("\n=== Test 2: Function reference in Object ===")
addFunc := (a, b) {
    R> a + b
}
multiplyFunc := (a, b) {
    R> a * b
}
calculator := {
    add: addFunc,
    multiply: multiplyFunc
}
println("calculator.add(3, 5):")
println(calculator.add(3, 5))
println("calculator.multiply(4, 6):")
println(calculator.multiply(4, 6))

println("\n=== Test 3: Function in Array ===")
func0 := () { R> "Function 0" }
func1 := () { R> "Function 1" }
func2 := (x) { R> x * 2 }
funcs := [func0, func1, func2]
println("funcs[0]():")
println(funcs[0]())
println("funcs[1]():")
println(funcs[1]())
println("funcs[2](10):")
println(funcs[2](10))

println("\n=== Test 4: typeof Check ===")
println("typeOf(greet):")
println(typeOf(greet))
println("typeOf(person.sayHello):")
println(typeOf(person.sayHello))
println("typeOf(calculator.add):")
println(typeOf(calculator.add))
println("typeOf(funcs[0]):")
println(typeOf(funcs[0]))

println("\n=== Test 5: Closure Returning Function ===")
makeAdder := (x) {
    R> (y) {
        R> x + y
    }
}
add5 := makeAdder(5)
println("add5(3):")
println(add5(3))
println("makeAdder(10)(7):")
println(makeAdder(10)(7))

println("\n=== Test 6: Function Stored in Object Returning Closure ===")
createMultiplier := (factor) {
    R> (x) {
        R> x * factor
    }
}
factory := {
    createMultiplier: createMultiplier
}
times3 := factory.createMultiplier(3)
println("times3(7):")
println(times3(7))

println("\n=== All tests completed ===")
