add:<num>=(a,b){ R> a + b; };

x := -10;
result := add(x, 20);

L>(i:=0; i<3; i=i+1){
    print(result + i);
};
