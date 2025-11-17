/* testfx/test_string_basic.fx - 基础字符串测试 */
main:=(){
    // 先测试纯数字（确保向后兼容）
    x := 42
    print(x)
    
    // 测试字符串字面量
    s := "Hello"
    // 暂时不能直接 print 字符串，用数字代替
    print(100)
    
    // 测试字符串数组（数字占位）
    arr := [1, 2, 3]
    print(arr[0])
}
