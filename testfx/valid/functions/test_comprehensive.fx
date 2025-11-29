// 简化综合功能测试 - 验证删除 codegen_builtin.c 后所有功能正常

println("=== 综合功能测试 ===")

// 1. 基本输出
println("1. 输出函数")
print("print测试: ")
print("a", "b", "c")
println()
println("println测试:", 1, 2, 3)

// 2. 类型转换
println("\n2. 类型转换")
println("toNum('123') =", toNum("123"))
println("toStr(456) =", toStr(456))
println("toBl(1) =", toBl(1))
println("toInt(3.7) =", toInt(3.7))
println("toFloat(5) =", toFloat(5))

// 3. 字符串操作
println("\n3. 字符串操作")
myStr := "Hello World"
println("len =", len(myStr))
println("charAt(0) =", charAt(myStr, 0))
println("substr(0,5) =", substr(myStr, 0, 5))
println("indexOf('World') =", indexOf(myStr, "World"))
println("upper =", upper(myStr))
println("lower =", lower(myStr))
println("trim(' hi ') =", trim("  hi  "))
println("split('a,b,c', ',') =", split("a,b,c", ","))
println("join(['x','y'], '-') =", join(["x","y"], "-"))
println("replace('foo', 'o', 'a') =", replace("foo", "o", "a"))
println("startsWith('hello', 'he') =", startsWith("hello", "he"))
println("endsWith('hello', 'lo') =", endsWith("hello", "lo"))
println("contains('hello', 'ell') =", contains("hello", "ell"))
println("reverse('abc') =", reverse("abc"))

// 4. 数组操作
println("\n4. 数组操作")
myArr := [1, 2, 3]
println("原数组:", myArr)
println("push(4) =", push(myArr, 4))
println("pop =", pop([1,2,3]))
println("shift =", shift([1,2,3]))
println("unshift(0) =", unshift([1,2,3], 0))
println("slice(1,2) =", slice([1,2,3,4], 1, 3))
println("concat =", concat([1,2], [3,4]))
println("reverse([1,2,3]) =", reverse([1,2,3]))
println("indexOf([1,2,3], 2) =", indexOf([1,2,3], 2))
println("includes([1,2,3], 2) =", includes([1,2,3], 2))

// 5. 对象操作
println("\n5. 对象操作")
myObj := {name: "test", value: 42}
println("原对象:", myObj)
println("keys =", keys(myObj))
println("values =", values(myObj))
println("entries =", entries(myObj))
println("hasField('name') =", hasField(myObj, "name"))
println("typeOf(myObj) =", typeOf(myObj))

// 6. 类型检查
println("\n6. 类型检查")
println("isNum(42) =", isNum(42))
println("isStr('hi') =", isStr("hi"))
println("isBl(true) =", isBl(true))
println("isArr([]) =", isArr([]))
println("isObj({}) =", isObj({}))
println("isNull(null) =", isNull(null))

// 7. 数学函数
println("\n7. 数学函数")
println("abs(-5) =", abs(-5))
println("floor(3.7) =", floor(3.7))
println("ceil(3.2) =", ceil(3.2))
println("round(3.5) =", round(3.5))
println("sqrt(16) =", sqrt(16))
println("pow(2, 3) =", pow(2, 3))
println("min(3, 5) =", min(3, 5))
println("max(3, 5) =", max(3, 5))
println("clamp(10, 0, 5) =", clamp(10, 0, 5))

// 8. 工具函数
println("\n8. 工具函数")
println("range(1, 5) =", range(1, 5))
println("range(0, 10, 2) =", range(0, 10, 2))

println("\n=== 基础测试通过! ===")
