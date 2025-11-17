/* testfx/test_chain.fx */
// 测试链式调用：arr.>length.>multiply(2)

multiply:<num>=(a, b){
    R>a * b
}

main:=(){
    arr := [10, 20, 30]
    
    // 链式调用: arr.>length 返回 3，然后 .>multiply(2) 返回 6
    result := arr.>length.>multiply(2)
    print(result)
}
