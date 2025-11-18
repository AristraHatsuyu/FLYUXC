add:<num>=(x, y) {
    R> x + y;
};

main := () {
    x := 10;
    y := 20;
    sum := add(x, y);
    print(sum);
    
    counter := 0;
    counter++;
    print(counter);
    
    L>(j := 0; j < 3; j++) {
        print(j);
    };
};
