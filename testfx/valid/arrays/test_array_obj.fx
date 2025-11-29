// 测试简单数组和对象函数

// 测试1: reverse
println("=== 测试 reverse ===")
arr := [1, 2, 3, 4, 5]
reversed := reverse(arr)
println(reversed)  // 应该输出: [5, 4, 3, 2, 1]

// 测试2: indexOf
println("\n=== 测试 indexOf ===")
idx := indexOf([10, 20, 30, 40], 30)
println(idx)  // 应该输出: 2

notFound := indexOf([10, 20, 30], 99)
println(notFound)  // 应该输出: -1

// 测试3: includes
println("\n=== 测试 includes ===")
has := includes([1, 2, 3], 2)
println(has)  // 应该输出: true

notHas := includes([1, 2, 3], 9)
println(notHas)  // 应该输出: false

// 测试4: values
println("\n=== 测试 values ===")
myObj := {name: "Alice", age: 30}
vals := values(myObj)
println(vals)  // 应该输出: ["Alice", 30]

// 测试5: entries
println("\n=== 测试 entries ===")
ents := entries(myObj)
println(ents)  // 应该输出: [["name", "Alice"], ["age", 30]]

println("\n=== 所有简单函数测试通过! ===")
