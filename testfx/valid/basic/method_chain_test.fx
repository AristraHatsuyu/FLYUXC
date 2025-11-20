// 测试方法链 .> 语法和 len 函数

// 1. 字符串方法链
"Hello World!".>print

// 2. 数组 len
[1, 2, 3, 4, 5].>len.>println

// 3. 字符串 len  
"FLYUX".>len.>println

// 4. 对象字面量方法链
{a: 1, b: 2}.>len.>println

// 5. 变量方法链
arr := [10, 20, 30]
arr.>len.>println

text := "Test String"
text.>len.>println
