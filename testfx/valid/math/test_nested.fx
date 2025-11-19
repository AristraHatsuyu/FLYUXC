// 复杂的数组、对象嵌套测试（不使用 obj 保留词）

// 1. 简单对象
person := {name: "Alice", age: 30}
println(person)

// 2. 对象中包含数组
student := {name: "Bob", scores: [85, 90, 95]}
println(student)

// 3. 数组中包含对象
users := [{name: "Charlie", age: 25}, {name: "Diana", age: 28}]
println(users)

// 4. 深度嵌套：对象 -> 数组 -> 对象
company := {name: "TechCorp", departments: [{name: "Engineering", count: 50}, {name: "Sales", count: 30}]}
println(company)

// 5. 深度嵌套：数组 -> 对象 -> 数组 -> 对象  
complex := [{category: "Electronics", items: [{name: "Phone", price: 999}, {name: "Laptop", price: 1999}]}, {category: "Books", items: [{name: "Fiction", price: 20}, {name: "Science", price: 30}]}]
println(complex)

// 6. 测试 undef 和 null 在对象中
mixed := {name: "Test", value: null, missing: undef}
println(mixed)

// 7. 空对象和空数组
empty_data := {arr: [], item: {}}
println(empty_data)
