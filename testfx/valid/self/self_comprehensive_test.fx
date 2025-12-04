// Self binding comprehensive test
println("=== SELF BINDING COMPREHENSIVE TEST ===")

// Test 1: Basic self in method
println("\n--- Test 1: Basic self access ---")
person := {
    name: "Alice",
    greet: () {
        R> "Hello, " + self.name
    }
}
println("person.greet() = " + person.greet())

// Test 2: Bound method stays bound when reassigned
println("\n--- Test 2: Bound method stays bound ---")
boundGreet := person.greet
println("boundGreet() = " + boundGreet())

// Test 3: Unbound method with .@ syntax
println("\n--- Test 3: Unbound method .@ syntax ---")
unboundGreet := person.@greet
println("Extracted with .@")

// Test 4: Rebind unbound method to another object
println("\n--- Test 4: Rebind unbound to another ---")
person2 := {
    name: "Bob",
    greet: person.@greet
}
println("person2.greet() = " + person2.greet())

// Test 5: Index access with binding
println("\n--- Test 5: Index access obj[key] ---")
key := "greet"
boundFromIndex := person[key]
println("person[key]() = " + boundFromIndex())

// Test 6: Unbound index access with @[
println("\n--- Test 6: Unbound index @[key] ---")
unboundFromIndex := person@[key]
println("Extracted with @[]")

// Test 7: Bound vs unbound in object literal
println("\n--- Test 7: Bound vs unbound in object literal ---")
person3 := {
    name: "Charlie",
    boundMethod: person[key],
    unboundMethod: person@[key]
}
println("person3.boundMethod() = " + person3.boundMethod())
println("person3.unboundMethod() = " + person3.unboundMethod())

// Test 8: Object property names can be reserved keywords
println("\n--- Test 8: Reserved keywords as property names ---")
special := {
    fn: "function value",
    obj: "object value",
    num: 123,
    str: "string value"
}
println("special.fn = " + special.fn)
println("special.obj = " + special.obj)
println("special.num = " + special.num)
println("special.str = " + special.str)

// Test 9: Self modification
println("\n--- Test 9: Self modification ---")
counter := {
    count: 0,
    increment: () {
        self.count = self.count + 1
        R> self
    },
    getCount: () {
        R> self.count
    }
}
counter.increment()
counter.increment()
counter.increment()
println("counter.getCount() = " + counter.getCount())

// Test 10: Chained calls with self
println("\n--- Test 10: Chained calls ---")
builder := {
    value: "",
    append: (s) {
        self.value = self.value + s
        R> self
    },
    get: () {
        R> self.value
    }
}
builder.append("Hello").append(" ").append("World")
println("builder.get() = " + builder.get())

println("\n=== ALL TESTS COMPLETED ===")
