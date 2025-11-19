// 复杂多行嵌套测试
company := {
    name: "TechCorp",
    employees: [
        {
            name: "Alice",
            role: "Engineer",
            skills: [ "Python", "C", "Go" ]
        },
        {
            name: "Bob",
            role: "Designer",
            skills: [ "Figma", "Sketch" ]
        }
    ],
    founded: 2020
}
println(company)

// 深度嵌套数组
matrix := [
    [ 1, 2, 3 ],
    [ 4, 5, 6 ],
    [ 7, 8, 9 ]
]
println(matrix)

// 混合测试
config := {
    debug: true,
    ports: [ 8080, 8081, 8082 ],
    database: {
        host: "localhost",
        port: 5432
    }
}
println(config)
