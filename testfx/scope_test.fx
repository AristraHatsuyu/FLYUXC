// 测试变量作用域问题
main := () {
    counter := 0;
    
    L>(i := 0; i < 3; i++) {
        counter = counter + 1;  // 使用 = 赋值而不是 := 声明
        print(counter);
    };
    
    print(counter);
};
