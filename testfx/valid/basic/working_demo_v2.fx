/* Working Demo - Carefully Avoiding Bugs */

add:<num>=(x, y){
    R> x + y;
};

main:=(){
    a := 10;
    b := 20;
    result := add(a, b);
    print(result);
    
    L>(k:=0; k<3; k++){
        print(k);
    };
    
    arr := [100, 200, 300];
    print(arr[0]);
    
    if(a > 5){
        print(999);
    };
    
    print(42);
};
