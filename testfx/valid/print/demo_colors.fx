// FLYUX 彩色终端输出演示
// 类似 Node.js 的 console.log() 效果

println("=== FLYUX 彩色输出测试 ===")
println("")

// 1. 数字 - 黄色
println("数字（黄色）:")
println(42)
println(3.14159)
println(-100)
println("")

// 2. 字符串 - 无颜色（原样）
println("字符串（无颜色）:")
println("Hello, FLYUX!")
println("支持中文和特殊字符: 你好世界 🚀")
println("")

// 3. 布尔值 - 黄色
println("布尔值（黄色）:")
println(true)
println(false)
println("")

// 4. null 值 - 灰色加粗
println("null 值（灰色加粗）:")
n :[num]= null
println(n)
println("")

// 5. 数组 - 括号灰色，内容带颜色
println("数组（括号灰色，元素带颜色）:")
println([1, 2, 3, 4, 5])
println(["apple", "banana", "cherry"])
println([true, false, true])
println("")

// 6. 混合数组
println("混合类型数组:")
println([1, "text", true, [10, 20]])
println("")

// 7. 嵌套数组
println("嵌套数组:")
println([[1, 2], [3, 4], [5, 6]])
println("")

println("=== 测试完成 ===")
