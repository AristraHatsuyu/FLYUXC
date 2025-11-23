/* Test member access in function arguments */
main := () {
    person := {name: "Alice", age: 25}
    
    // Test 1: assign to variable first (should work)
    n := person.name
    printf("Name: %v\n", n)
    
    // Test 2: direct member access in printf (might fail)
    printf("Name: %v\n", person.name)
}
