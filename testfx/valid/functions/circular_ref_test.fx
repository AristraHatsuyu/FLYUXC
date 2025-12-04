// Test circular reference scenarios

println("=== Circular Reference Chain Test ===")

// Test 1: Basic chain and reassignment
println("\n--- Test 1: Chain through variables ---")
node1 := { a: 123, name: "node1" }
node2 := node1  // node2 is a reference to node1
node3 := node2  // node3 is also a reference to node1

println("After: node1 := {a:123}, node2 := node1, node3 := node2")
println("node1.a: " + node1.a)
println("node2.a: " + node2.a)
println("node3.a: " + node3.a)

// Now reassign node1 using spread from node3
node1 = { ...node3, new_field: "added" }
println("\nAfter: node1 = { ...node3, new_field: 'added' }")
println("node1:")
println(node1)
println("node2 (should still point to OLD object):")
println(node2)
println("node3 (should still point to OLD object):")
println(node3)

// Test 2: Modify through chain
println("\n--- Test 2: Modify through reference chain ---")
a := { val: 1 }
b := a
c := b
c.val = 999
println("After: a := {val:1}, b := a, c := b, c.val = 999")
println("a.val (should be 999): " + a.val)
println("b.val (should be 999): " + b.val)
println("c.val (should be 999): " + c.val)

// Test 3: Self-referential object (object contains reference to itself)
println("\n--- Test 3: Self-referential object ---")
self := { name: "self" }
self.me = self  // Now self contains a reference to itself!
println("After: self := {name:'self'}, self.me = self")
println("self.name: " + self.name)
println("self.me.name: " + self.me.name)
println("self.me.me.name: " + self.me.me.name)
println("(Infinite depth available!)")

// Test 4: Mutual reference (two objects referencing each other)
println("\n--- Test 4: Mutual reference ---")
objA := { name: "A" }
objB := { name: "B" }
objA.ref = objB
objB.ref = objA  // Now they reference each other
println("After: objA.ref = objB, objB.ref = objA")
println("objA.ref.name: " + objA.ref.name)
println("objB.ref.name: " + objB.ref.name)
println("objA.ref.ref.name (back to A): " + objA.ref.ref.name)
println("objA.ref.ref.ref.name (back to B): " + objA.ref.ref.ref.name)

// Test 5: What happens when we spread a self-referential object?
println("\n--- Test 5: Spread self-referential object ---")
selfCopy := { ...self }
println("selfCopy := { ...self }")
println("selfCopy.name: " + selfCopy.name)
println("selfCopy.me.name (shallow copy, still refs original self): " + selfCopy.me.name)

// Modify self's name
self.name = "modified_self"
println("\nAfter self.name = 'modified_self':")
println("self.name: " + self.name)
println("selfCopy.name (should be unchanged): " + selfCopy.name)
println("selfCopy.me.name (should change - points to original): " + selfCopy.me.name)

// Test 6: Deep clone with circular reference
// WARNING: This might cause stack overflow or infinite loop!
println("\n--- Test 6: Deep clone self-referential (CAUTION) ---")
println("Skipping deepClone(self) - would cause infinite recursion!")
println("In a real implementation, deepClone should detect cycles.")

println("\n=== All Circular Reference Tests Completed ===")
