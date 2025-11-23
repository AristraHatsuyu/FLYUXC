// 调试 box_null_typed

println("=== 调试 box_null_typed ===\n")

result := parseJSON("invalid")
println("result =", result)
println("type =", typeOf(result))

println("\n=== 完成 ===")
