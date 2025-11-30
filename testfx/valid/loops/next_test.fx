// Next (continue) 功能测试 - 使用 N> 语法

main := () {
    println("=== Next (Continue) Test ===")
    
    // Test 1: Simple loop with next
    println("Test 1: Simple loop - skip even numbers")
    sum := 0
    L> (10) {
        sum = sum + 1
        if (sum % 2 == 0) { N> }
        println("odd:", sum)
    }
    println("Final sum:", sum)
    
    // Test 2: For loop with next
    println("\nTest 2: For loop - skip multiples of 3")
    result := 0
    L> (i := 1; i <= 10; i++) {
        if (i % 3 == 0) { N> }
        result = result + i
        println("adding:", i)
    }
    println("Result:", result)
    
    // Test 3: Foreach loop with next
    println("\nTest 3: Foreach loop - skip 'skip' item")
    arr := ["a", "skip", "b", "skip", "c"]
    output := ""
    L> (arr : item) {
        if (item == "skip") { N> }
        output = output + item
    }
    println("Output:", output)
    
    // Test 4: Nested loops with next
    println("\nTest 4: Nested loops - inner next")
    outer := 0
    L> (3) {
        outer = outer + 1
        println("Outer:", outer)
        inner := 0
        L> (5) {
            inner = inner + 1
            if (inner == 2) { N> }
            println("  Inner:", inner)
        }
    }
    
    // Test 5: Next with local variable cleanup
    println("\nTest 5: Next with local variable")
    count := 0
    L> (5) {
        localVar := "iteration"
        count = count + 1
        if (count == 2 || count == 4) {
            println("Skipping:", count)
            N>
        }
        println("Processing:", count, localVar)
    }
    
    println("\n=== All Next Tests Passed ===")
}
