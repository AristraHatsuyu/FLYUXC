/* Working Demo - Complete Feature Showcase */

myFunc:<num>=(a,b){
    R>a + b * a;
};

main:=(){
    arr := [10, 20, 30, 40];
    
    L>(i:=0; i<3; i++){
        print(arr[i]);
        arr[i] = arr[i] + i;
    };
    
    print(myFunc(5, 3));
    
    counter := 0;
    L>(j:=0; j<5; j++){
        counter++;
    };
    print(counter);
};
