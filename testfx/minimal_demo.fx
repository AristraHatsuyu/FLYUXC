/* Minimal Working Demo */

add:<num>=(x, y){
    R> x + y;
};

main:=(){
    a := 10;
    b := 20;
    c := add(a, b);
    print(c);
    
    i := 0;
    L>(j:=0; j<3; j++){
        i := i + 1;
    };
    print(i);
    
    arr := [1, 2, 3];
    print(arr[1]);
    
    if(a > 5){
        print(999);
    };
};
