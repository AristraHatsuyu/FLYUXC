/* testfx/test_mixed_simple.fx - 简单混合类型测试 */
main:=(){
    // 测试数字
    x := 42
    print(x)
    
    // 测试字符串（暂时用数字代替）
    y := 100  // 将来可以是 "hello"
    print(y)
    
    // 测试混合数组
    arr := [1, 2, 3]
    print(arr[0])
    print(arr[1])
}
