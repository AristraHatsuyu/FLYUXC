// 测试嵌套访问和赋值
println("=== 测试嵌套对象访问 ===")
person := {
    name: "Bob",
    address: {city: "Beijing", zip: "100000"}
}

println("person:", person)
println("person.address:", person.address)
println("person.address.city:", person.address.city)
println("person.address.zip:", person.address.zip)

// 测试嵌套赋值
println("\n=== 测试嵌套对象修改 ===")
person.address.city = "Shanghai"
person.address.zip = "200000"
println("修改后 person:", person)
println("修改后 person.address:", person.address)
println("修改后 person.address.city:", person.address.city)

// 测试对象数组属性访问
println("\n=== 测试对象数组属性 ===")
data := {items: [10, 20, 30], tags: ["a", "b", "c"]}
println("data:", data)
println("data.items:", data.items)
println("data.items[0]:", data.items[0])
println("data.items[1]:", data.items[1])
println("data.tags[2]:", data.tags[2])

// 测试索引修改
println("\n=== 测试数组索引修改 ===")
data.items[1] = 99
println("修改后 data:", data)
println("修改后 data.items[1]:", data.items[1])

println("\n=== 测试完成 ===")
