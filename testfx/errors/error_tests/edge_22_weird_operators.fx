// 奇怪的运算符
x := 10
y := 20

// 严格相等
if (x === y) { println("equal") }

// 严格不等
if (x !== y) { println("not equal") }

// 三元运算符（FLYUX不支持）
result := x > y ? x : y

// 空值合并
value := null ?? "default"

// 可选链
obj?.property?.nested
