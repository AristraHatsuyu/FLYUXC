// 测试属性修改
println("=== 测试1: 修改已有属性 ===")
user := {name: "Alice", age: 25}
println("初始:", user)

user.name = "Bob"
println("修改name后:", user)

user.age = 30
println("修改age后:", user)

// 测试嵌套访问
println("\n=== 测试2: 嵌套对象访问 ===")
person := {
    address: {city: "Beijing", zip: "100000"}
}
println("person:", person)
println("person.address:", person.address)
// println("person.address.city:", person.address.city)  // 暂时注释

// 测试数组索引
println("\n=== 测试3: 数组索引访问 ===")
arr := ["a", "b", "c"]
println("arr:", arr)
println("arr[0]:", arr[0])
println("arr[1]:", arr[1])
println("arr[2]:", arr[2])

// 测试对象的数组属性
println("\n=== 测试4: 对象数组属性 ===")
data := {items: [10, 20, 30]}
println("data:", data)
println("data.items:", data.items)
// println("data.items[0]:", data.items[0])  // 暂时注释
