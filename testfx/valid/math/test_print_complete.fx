// ============================================
// FLYUX Print 功能完整测试套件
// ============================================

// 1. 基础类型输出
println("=== 1. 基础类型 ===")
number := 42
text := "Hello FLYUX"
boolean := true
null_val:[num] = null
println(number)
println(text)
println(boolean)
println(null_val)

// 2. 未定义变量和 undef
println("=== 2. Undef 测试 ===")
println(undefined_var)
x:[str] = "test"
x = undef
println(x)
println(undef)

// 3. 数学运算结果
println("=== 3. 数学运算 ===")
println(10 + 20)
println(100 - 35)
println(6 * 7)
println(100 / 4)
println(2 ** 8)

// 4. 字符串拼接
println("=== 4. 字符串操作 ===")
greeting := "Hello"
name := "World"
println(greeting + " " + name + "!")
println("数字转字符串: " + 123)

// 5. 简单数组
println("=== 5. 简单数组 ===")
numbers := [1, 2, 3, 4, 5]
println(numbers)
empty_arr := []
println(empty_arr)
mixed := [42, "text", true, null]
println(mixed)

// 6. 嵌套数组（2层）
println("=== 6. 嵌套数组（2层）===")
matrix2d := [[1, 2], [3, 4], [5, 6]]
println(matrix2d)

// 7. 深度嵌套数组（3层）
println("=== 7. 深度嵌套数组（3层）===")
matrix3d := [[[1, 2], [3, 4]], [[5, 6], [7, 8]]]
println(matrix3d)

// 8. 超深度嵌套数组（4层）- 测试彩虹括号循环
println("=== 8. 超深度嵌套数组（4层）===")
deep_array := [[[[1, 2], [3, 4]], [[5, 6], [7, 8]]], [[[9, 10], [11, 12]], [[13, 14], [15, 16]]]]
println(deep_array)

// 9. 简单对象
println("=== 9. 简单对象 ===")
person := { name: "Alice", age: 30, active: true }
println(person)
empty_obj := {}
println(empty_obj)

// 10. 对象包含数组
println("=== 10. 对象包含数组 ===")
student := {
    name: "Bob",
    grades: [85, 90, 95, 88],
    passed: true
}
println(student)

// 11. 数组包含对象
println("=== 11. 数组包含对象 ===")
users := [
    { id: 1, name: "Charlie", role: "Admin" },
    { id: 2, name: "Diana", role: "User" },
    { id: 3, name: "Eve", role: "Guest" }
]
println(users)

// 12. 对象嵌套对象
println("=== 12. 对象嵌套对象 ===")
config := {
    server: {
        host: "localhost",
        port: 8080
    },
    database: {
        type: "PostgreSQL",
        version: 14
    }
}
println(config)

// 13. 复杂混合嵌套（对象->数组->对象->数组）
println("=== 13. 复杂混合嵌套 ===")
company := {
    name: "TechCorp",
    departments: [
        {
            name: "Engineering",
            teams: [
                { name: "Frontend", members: 5 },
                { name: "Backend", members: 8 }
            ]
        },
        {
            name: "Sales",
            teams: [
                { name: "Enterprise", members: 3 },
                { name: "SMB", members: 4 }
            ]
        }
    ],
    founded: 2020
}
println(company)

// 14. 数组中包含各种类型
println("=== 14. 混合类型数组 ===")
mixed_array := [
    42,
    "string",
    true,
    null,
    undef,
    [1, 2, 3],
    { key: "value" }
]
println(mixed_array)

// 15. 对象中包含各种类型
println("=== 15. 混合类型对象 ===")
mixed_object := {
    number: 100,
    text: "hello",
    flag: false,
    nothing: null,
    missing: undef,
    list: [10, 20, 30],
    nested: { inner: "data" }
}
println(mixed_object)

// 16. 多行对象测试
println("=== 16. 多行对象 ===")
multiline_obj := {
    name: "MultiLine",
    properties: {
        width: 1920,
        height: 1080
    },
    features: ["feature1", "feature2", "feature3"]
}
println(multiline_obj)

// 17. 多行数组测试
println("=== 17. 多行数组 ===")
multiline_arr := [
    [1, 2, 3],
    [4, 5, 6],
    [7, 8, 9]
]
println(multiline_arr)

// 18. 极限嵌套测试（5层）
println("=== 18. 极限嵌套（5层）===")
ultra_deep := {
    level1: {
        level2: {
            level3: {
                level4: {
                    level5: "最深层",
                    data: [1, 2, 3]
                }
            }
        }
    }
}
println(ultra_deep)

// 19. 真实场景模拟：用户数据
println("=== 19. 真实场景：用户数据 ===")
user_profile := {
    id: 12345,
    username: "john_doe",
    email: "john@example.com",
    profile: {
        firstName: "John",
        lastName: "Doe",
        age: 28,
        avatar: "https://example.com/avatar.jpg"
    },
    preferences: {
        theme: "dark",
        language: "en",
        notifications: true
    },
    posts: [
        {
            id: 1,
            title: "Hello World",
            tags: ["intro", "first"],
            likes: 42
        },
        {
            id: 2,
            title: "My Journey",
            tags: ["personal", "story"],
            likes: 156
        }
    ],
    friends: [
        { id: 101, name: "Alice" },
        { id: 102, name: "Bob" },
        { id: 103, name: "Charlie" }
    ]
}
println(user_profile)

// 20. 真实场景模拟：API 响应
println("=== 20. 真实场景：API 响应 ===")
api_response := {
    status: 200,
    success: true,
    data: {
        items: [
            {
                id: "prod_001",
                name: "Laptop",
                price: 1299,
                specs: {
                    cpu: "Intel i7",
                    ram: 16,
                    storage: 512
                },
                reviews: [
                    { user: "User1", rating: 5, comment: "Excellent!" },
                    { user: "User2", rating: 4, comment: "Good value" }
                ]
            },
            {
                id: "prod_002",
                name: "Mouse",
                price: 29,
                specs: {
                    dpi: 1600,
                    wireless: true
                },
                reviews: []
            }
        ],
        total: 2,
        page: 1
    },
    timestamp: 1700000000
}
println(api_response)

// 21. 测试空值在复杂结构中
println("=== 21. 空值测试 ===")
with_nulls := {
    defined: "value",
    set_to_null: null,
    set_to_undef: undef,
    nested_empty: {
        arr: [],
        item: {}
    }
}
println(with_nulls)

// 22. 布尔值组合
println("=== 22. 布尔值组合 ===")
bool_test := {
    allTrue: [true, true, true],
    allFalse: [false, false, false],
    mixed: [true, false, true, false]
}
println(bool_test)

// 23. 数字类型测试
println("=== 23. 数字类型 ===")
numbers_test := {
    integer: 42,
    decimal: 3.14159,
    negative: -273,
    zero: 0,
    large: 999999
}
println(numbers_test)

// 24. 特殊字符串测试
println("=== 24. 特殊字符串 ===")
special_strings := {
    empty: "",
    spaces: "   ",
    mixed: "Hello 世界 🌍",
    symbols: "!@#$%^&*()"
}
println(special_strings)

// 25. 最终压力测试：超大复杂结构
println("=== 25. 压力测试：超大结构 ===")
massive_structure := {
    meta: { version: "1.0", type: "test" },
    data: [
        {
            category: "A",
            items: [
                { id: 1, values: [1, 2, 3], tags: ["a", "b"] },
                { id: 2, values: [4, 5, 6], tags: ["c", "d"] }
            ]
        },
        {
            category: "B",
            items: [
                { id: 3, values: [7, 8, 9], tags: ["e", "f"] },
                { id: 4, values: [10, 11, 12], tags: ["g", "h"] }
            ]
        }
    ],
    stats: {
        total: 4,
        byCategory: { A: 2, B: 2 },
        active: true
    }
}
println(massive_structure)

println("=== 测试完成 ===")
