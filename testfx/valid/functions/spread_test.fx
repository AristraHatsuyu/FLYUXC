// Test spread operator for objects and arrays

// 1. Basic object spread
println("=== Object Spread ===")
obj1 := { a: 1, b: 2 }
println("obj1:")
println(obj1)

copy1 := { ...obj1 }
println("copy1 := { ...obj1 }:")
println(copy1)

// Verify it's a shallow copy, not a reference
obj1.a = 100
println("After obj1.a = 100:")
println("obj1:")
println(obj1)
println("copy1 (should be unchanged):")
println(copy1)

// 2. Object spread with additional properties
println("\n=== Object Spread with additions ===")
obj2 := { x: 10, y: 20 }
extended := { ...obj2, z: 30 }
println("obj2:")
println(obj2)
println("extended := { ...obj2, z: 30 }:")
println(extended)

// 3. Object merge (multiple spreads)
println("\n=== Object Merge ===")
defaults := { width: 100, height: 200 }
custom := { height: 300, color: "red" }
merged := { ...defaults, ...custom }
println("defaults:")
println(defaults)
println("custom:")
println(custom)
println("merged := { ...defaults, ...custom }:")
println(merged)

// 4. Array spread
println("\n=== Array Spread ===")
arr1 := [1, 2, 3]
println("arr1:")
println(arr1)

copy_arr := [...arr1]
println("copy_arr := [...arr1]:")
println(copy_arr)

// Verify array copy
arr1[0] = 999
println("After arr1[0] = 999:")
println("arr1:")
println(arr1)
println("copy_arr (should be unchanged):")
println(copy_arr)

// 5. Array spread with additions
println("\n=== Array Spread with additions ===")
arr2 := [4, 5]
combined := [...arr1, ...arr2, 6, 7]
println("arr1:")
println(arr1)
println("arr2:")
println(arr2)
println("combined := [...arr1, ...arr2, 6, 7]:")
println(combined)

// 6. Multi-level reference test (nested objects with spread)
println("\n=== Multi-level Reference Test ===")
inner := { val: 42 }
middle := { inner: inner, name: "middle" }
outer := { ...middle, extra: "data" }
println("inner:")
println(inner)
println("middle:")
println(middle)
println("outer (spread from middle):")
println(outer)

// Modify inner and see if it affects outer
inner.val = 999
println("\nAfter inner.val = 999:")
println("inner:")
println(inner)
println("middle.inner (should also change - same ref):")
println(middle.inner)
println("outer.inner (spread is shallow - should also change):")
println(outer.inner)

// 7. Deep clone vs spread (shallow)
println("\n=== Deep Clone vs Spread (Shallow) Test ===")
data := { nested: { value: 100 } }
shallowCopy := { ...data }
deepCopyData := deepClone(data)

println("Original data:")
println(data)
println("Shallow copy (spread):")
println(shallowCopy)
println("Deep copy:")
println(deepCopyData)

// Modify the nested object in original
data.nested.value = 500
println("\nAfter data.nested.value = 500:")
println("data:")
println(data)
println("shallowCopy (should also change - nested is same ref):")
println(shallowCopy)
println("deepCopyData (should NOT change - truly independent):")
println(deepCopyData)

// 8. Multi-level Chain Reference Test (a -> b -> c)
println("\n=== Multi-level Chain Reference Test (a -> b -> c) ===")
c := { val: "c_value" }
b := { ref_c: c, val: "b_value" }
a := { ref_b: b, val: "a_value" }

println("Original chain:")
println("a:")
println(a)
println("b:")
println(b)
println("c:")
println(c)

// Modify c and check the chain
c.val = "c_modified"
println("\nAfter c.val = 'c_modified':")
println("c.val:")
println(c.val)
println("b.ref_c.val (should also change):")
println(b.ref_c.val)
println("a.ref_b.ref_c.val (should also change):")
println(a.ref_b.ref_c.val)

// 9. Spread with multi-level chain
println("\n=== Spread with Multi-level Chain ===")
a_copy := { ...a }
println("a_copy := { ...a }:")
println(a_copy)

// Modify b
b.val = "b_modified_after_spread"
println("\nAfter b.val = 'b_modified_after_spread':")
println("a.ref_b.val (should change - same ref):")
println(a.ref_b.val)
println("a_copy.ref_b.val (should also change - shallow spread):")
println(a_copy.ref_b.val)

// 10. Deep clone breaks the chain
println("\n=== Deep Clone Breaks the Chain ===")
a_deep := deepClone(a)
println("a_deep := deepClone(a):")
println(a_deep)

b.val = "b_final_change"
println("\nAfter b.val = 'b_final_change':")
println("a.ref_b.val (should change):")
println(a.ref_b.val)
println("a_deep.ref_b.val (should NOT change - deep cloned):")
println(a_deep.ref_b.val)

// 11. Circular reference test
println("\n=== Circular Reference Test ===")
node1 := { name: "node1" }
node2 := { name: "node2" }
node3 := { name: "node3" }

// Create a chain: node1 -> node2 -> node3
node1.next = node2
node2.next = node3

println("Chain: node1 -> node2 -> node3")
println("node1.next.name:")
println(node1.next.name)
println("node1.next.next.name:")
println(node1.next.next.name)

// Note: Circular references (node3.next = node1) would cause infinite
// recursion in deepClone. This is expected JS-like behavior.
// We test the warning case separately if needed.

println("\n=== All tests completed ===")
