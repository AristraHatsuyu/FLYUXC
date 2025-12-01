/* Complete Working Demo - All Features */

add:<num>=(x, y){
    R> x + y;
};

multiply:<num>=(a, b){
    R> a * b;
};

compute:<num>=(val, factor){
    R> val + factor * val;
};

main:=(){
    /* Basic arithmetic */
    x := 10;
    y := 20;
    sum := add(x, y);
    product := multiply(x, y);
    
    println(sum);
    println(product);
    
    /* Increment operators */
    counter := 0;
    counter++;
    ++counter;
    println(counter);
    
    /* For loop with all parts */
    total := 0;
    L>(i:=0; i<5; i++){
        total := total + i;
    };
    println(total);
    
    /* Nested computation */
    result := compute(5, 3);
    println(result);
    
    /* Arrays (read-only) */
    arr := [100, 200, 300];
    println(arr[0]);
    println(arr[1]);
    
    /* Objects (read-only) */
    data := {value: 999, name: "test"};
    println(data.value);
    
    /* Conditional */
    if(x > 5){
        println(777);
    }{
        println(888);
    };
    
    /* Complex expression */
    complex := (x + y) * 2 - 10 / 2;
    println(complex);
    
    println(42);
};
