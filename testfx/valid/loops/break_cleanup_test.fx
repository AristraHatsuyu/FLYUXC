// Break Cleanup Test - Tests that loop-local variables are released on break

main := () {
    println("=== Break Cleanup Test ===")
    
    // Test 1: Simple loop break with local variable
    println("Test 1: Simple loop break")
    count := 0
    L> [5] {
        localStr := "loop local"
        count = count + 1
        if (count == 3) { B> }
    }
    println("After loop, count:", count)
    
    // Test 2: Nested loop break
    println("Test 2: Nested loop")
    outer := 0
    L> [3] {
        outerLocal := "outer"
        outer = outer + 1
        inner := 0
        L> [10] {
            innerLocal := "inner"
            inner = inner + 1
            if (inner == 2) { B> }
        }
        println("Inner broke at:", inner)
    }
    println("Outer completed:", outer)
    
    // Test 3: For loop with break and local array
    println("Test 3: For loop with array")
    last := 0
    L> (i := 0; i < 100; i++) {
        arr := [i, i+1, i+2]
        last = i
        if (i == 5) { B> }
    }
    println("For loop last:", last)
    
    // Test 4: Break with object variable
    println("Test 4: Break with object")
    objIdx := 0
    L> [5] {
        myObj := { name: "test", idx: objIdx }
        objIdx = objIdx + 1
        if (objIdx == 2) { B> }
    }
    println("Object index:", objIdx)
    
    println("=== All Break Cleanup Tests Passed ===")
}
