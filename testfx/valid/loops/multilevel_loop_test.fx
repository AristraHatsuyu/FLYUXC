// 多级 break/next 测试

main := () {
    println("=== Multilevel Break/Next Test ===")
    
    // Test 1: 多级 break - 从内层直接跳出外层
    println("\nTest 1: Multilevel break from inner to outer")
    count := 0
    L> [3]:outer1 {
        println("Outer:", count + 1)
        L> [5]:inner1 {
            count = count + 1
            if (count == 3) {
                println("  Breaking out to outer at count:", count)
                B> outer1
            }
            println("  Inner count:", count)
        }
        println("This should not print after multilevel break")
    }
    println("After outer loop, count:", count)
    
    // Test 2: 多级 next - 从内层继续外层下一次迭代
    println("\nTest 2: Multilevel next from inner to outer")
    outer_count := 0
    inner_total := 0
    L> [4]:outer2 {
        outer_count = outer_count + 1
        println("Outer iteration:", outer_count)
        L> [3]:inner2 {
            inner_total = inner_total + 1
            if (outer_count == 2) {
                println("  Skipping rest of outer iteration 2")
                N> outer2
            }
            println("  Inner total:", inner_total)
        }
    }
    println("Final: outer_count =", outer_count, "inner_total =", inner_total)
    
    // Test 3: 三层嵌套 break
    println("\nTest 3: Three-level nested break")
    L> [2]:level1 {
        println("Level 1")
        L> [2]:level2 {
            println("  Level 2")
            L> [3]:level3 {
                println("    Level 3")
                if (true) {
                    println("    Breaking to level 1")
                    B> level1
                }
            }
        }
    }
    println("Exited all three levels")
    
    // Test 4: 混合普通 break/next 和多级
    println("\nTest 4: Mixed regular and multilevel break/next")
    result := 0
    L> [5]:outer4 {
        L> [5]:inner4 {
            result = result + 1
            if (result % 3 == 0) {
                N>  // 普通 next，继续当前循环
            }
            if (result == 7) {
                B> outer4  // 多级 break
            }
            println("Result:", result)
        }
    }
    println("Final result:", result)
    
    // Test 5: for 循环带标签
    println("\nTest 5: For loop with label")
    sum := 0
    L> (i := 0; i < 3; i++):forOuter {
        L> (j := 0; j < 3; j++):forInner {
            sum = sum + 1
            if (i == 1 && j == 1) {
                println("Breaking for loops at i=", i, "j=", j)
                B> forOuter
            }
            println("i=", i, "j=", j, "sum=", sum)
        }
    }
    println("Sum:", sum)
    
    // Test 6: foreach 循环带标签
    println("\nTest 6: Foreach loop with label")
    arr1 := ["a", "b", "c"]
    arr2 := [1, 2, 3]
    output := ""
    L> arr1 : x : foreachOuter {
        L> arr2 : y : foreachInner {
            output = output + x + toStr(y) + " "
            if (x == "b" && y == 2) {
                println("Continuing outer foreach at x=", x, "y=", y)
                N> foreachOuter
            }
        }
    }
    println("Output:", output)
    
    // Test 7: 带括号的 foreach 语法
    println("\nTest 7: Foreach with parentheses syntax")
    items := ["x", "y", "z"]
    found := ""
    L>(items : item):pforeach {
        if (item == "y") {
            found = item
            B> pforeach
        }
        println("Checking:", item)
    }
    println("Found:", found)
    
    println("\n=== All Multilevel Tests Completed ===")
}
