/* testfx/demo_simplified.fx - 简化版 demo */
myFunc:<num>=(a,b){
    R>a + b * a;
};

main:=(){
    arr := ["A", 3, "B", 5];
    myobj := {key1:"value", key2:[7, "text"]};
    result := arr.>length.>myFunc(2);
    
    L>(i:=0; i<arr.>length; i++){
        item := arr[i];
        if((item > 3*(i+2) && !i) || item.>myFunc(i)){
            arr[i] = item.>myFunc(result);
        }{
            arr[i] = item + i;
            tmp := {k:"v"};
        };
    };
    
    final := {data:arr, text:myobj.key1};
    print(final.data[1], final.text, result, arr.>length);
};
