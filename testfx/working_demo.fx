/* Working Demo - 展示编译器的所有功能 */

/* 用户定义函数 */
add:<num>=(x, y) {
    R> x + y;
};

multiply:<num>=(a, b) {
    R> a * b;
};

/* 主函数 */
main := () {
    /* 变量声明和基本运算 */
    x := 10;
    y := 20;
    sum := add(x, y);
    product := multiply(x, y);
    
    print(sum);
    print(product);
    
    /* 递增/递减运算符 */
    counter := 0;
    counter++;
    ++counter;
    print(counter);
    
    /* for 循环 */
    i := 0;
    L>(j := 0; j < 5; j++) {
        i := i + 1;
    };
    print(i);
    
    /* 数组 (placeholder) */
    arr := [1, 2, 3, 4, 5];
    
    /* 对象 (placeholder) */
    data := {name: "test", value: 42};
    
    /* 条件语句 */
    if (x > 5) {
        print(999);
    };
    
    print(1234);
};
