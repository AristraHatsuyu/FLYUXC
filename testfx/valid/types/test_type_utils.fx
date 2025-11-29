// 测试类型检查函数和工具函数

println("=== 类型检查函数测试 ===")

// 测试数据
myNum := 42
myStr := "hello"
myBool := true
myArr := [1, 2, 3]
myObj := {a: 1, b: 2}
myNull :[obj]= null

// isNum
println("\n--- isNum ---")
println("isNum(42) =", isNum(myNum))        // true
println("isNum('hello') =", isNum(myStr))   // false
println("isNum([1,2]) =", isNum(myArr))     // false

// isStr
println("\n--- isStr ---")
println("isStr('hello') =", isStr(myStr))   // true
println("isStr(42) =", isStr(myNum))        // false

// isBl
println("\n--- isBl ---")
println("isBl(true) =", isBl(myBool))       // true
println("isBl(42) =", isBl(myNum))          // false

// isArr
println("\n--- isArr ---")
println("isArr([1,2,3]) =", isArr(myArr))   // true
println("isArr({a:1}) =", isArr(myObj))     // false

// isObj
println("\n--- isObj ---")
println("isObj({a:1}) =", isObj(myObj))     // true
println("isObj([1,2]) =", isObj(myArr))     // false

// isNull
println("\n--- isNull ---")
println("isNull(null) =", isNull(myNull))   // true
println("isNull(42) =", isNull(myNum))      // false

// isUndef - 注意：直接访问未定义变量会报错，所以用另一种方式测试
println("\n--- isUndef ---")
println("isUndef(null) =", isUndef(myNull)) // false (null 不是 undef)

println("\n=== 工具函数测试 ===")

// range
println("\n--- range ---")
println("range(1, 5) =", range(1, 5))           // [1, 2, 3, 4]
println("range(0, 10, 2) =", range(0, 10, 2))   // [0, 2, 4, 6, 8]
println("range(5, 0, -1) =", range(5, 0, -1))   // [5, 4, 3, 2, 1]

// assert (只测试通过的情况，失败会终止程序)
println("\n--- assert ---")
assert(1 == 1, "1 should equal 1")
assert(true, "true should be truthy")
println("All assertions passed!")
