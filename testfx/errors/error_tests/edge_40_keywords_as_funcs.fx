// 关键字作为函数调用（应该可以）
for := (x) { R> x * 2 }
while := (x) { R> x + 1 }
let := (a, b) { R> a + b }
const := 100
var := "hello"
return := (x) { R> x }

result1 := for(10)
result2 := while(5)
result3 := let(1, 2)
result4 := return(42)

println(result1)
println(result2)
println(result3)
println(const)
println(var)
println(result4)
