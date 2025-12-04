// 高级测试：复杂闭包、拷贝行为、JSON序列化、链式调用

println("=== Test 1: Object with Closure Using External Function ===")
// 闭包函数里引用外部函数
helper := (x) {
    R> x * 2
}
createProcessor := (multiplier) {
    R> (x) {
        // 使用外部函数 helper
        R> helper(x) + multiplier
    }
}
processor := createProcessor(100)
println("processor(5): " + processor(5))  // helper(5)=10, +100=110

println("\n=== Test 2: Object with Method Using External Function ===")
mathUtils := {
    doubleIt: helper,  // 引用外部函数
    process: (x) {
        // 在对象内部引用外部函数
        R> helper(x) + 50
    }
}
println("mathUtils.doubleIt(7): " + mathUtils.doubleIt(7))
println("mathUtils.process(7): " + mathUtils.process(7))

println("\n=== Test 3: Shallow Copy and Function References ===")
original := {
    name: "original",
    greet: (msg) { R> "Hello, " + msg }
}
// 浅拷贝
copied := {...original}
println("original.greet('World'): " + original.greet("World"))
println("copied.greet('World'): " + copied.greet("World"))

// 修改拷贝的名字，原对象不应受影响
copied.name = "copied"
println("After copy modification:")
println("original.name: " + original.name)
println("copied.name: " + copied.name)

// 函数引用应该相同（浅拷贝）
// 这里可以通过调用来验证
println("Both should call same function:")
println("original.greet('test'): " + original.greet("test"))
println("copied.greet('test'): " + copied.greet("test"))

println("\n=== Test 4: JSON String with Function ===")
objectWithFunc := {
    value: 42,
    getValue: () { R> 42 },
    nested: {
        innerFunc: (x) { R> x }
    }
}
println("toJSON(objectWithFunc):")
jsonStr := toJSON(objectWithFunc)
println(jsonStr)

println("\n=== Test 5: Chain Calls - Direct Double Invocation ===")
// createCounter().increment() 双重直接调用
makeCounter := () {
    val := 0
    R> {
        inc: () {
            val = val + 1
            R> val
        },
        dec: () {
            val = val - 1
            R> val
        }
    }
}

// 直接链式调用
println("makeCounter().inc(): " + makeCounter().inc())
println("makeCounter().inc(): " + makeCounter().inc())  // 新计数器，应该还是1

// 保存引用后调用
cnt := makeCounter()
println("cnt.inc(): " + cnt.inc())
println("cnt.inc(): " + cnt.inc())

println("\n=== Test 6: Array of Closures with Shared State ===")
makeAdders := (base) {
    R> [
        (x) { R> base + x },
        (x) { R> base + x * 2 },
        (x) { R> base + x * 3 }
    ]
}
adders := makeAdders(10)
println("adders[0](5): " + adders[0](5))  // 10+5=15
println("adders[1](5): " + adders[1](5))  // 10+10=20
println("adders[2](5): " + adders[2](5))  // 10+15=25

println("\n=== Test 7: Higher-Order Functions ===")
// 函数作为参数和返回值
applyTwice := (f, x) {
    R> f(f(x))
}
addOne := (n) { R> n + 1 }
println("applyTwice(addOne, 5): " + applyTwice(addOne, 5))  // ((5+1)+1)=7

// map-like operation
transform := (arr, f) {
    R> [f(arr[0]), f(arr[1]), f(arr[2])]
}
nums := [1, 2, 3]
doubled := transform(nums, (x) { R> x * 2 })
println("doubled[0]: " + doubled[0])
println("doubled[1]: " + doubled[1])
println("doubled[2]: " + doubled[2])

println("\n=== All advanced tests completed ===")
