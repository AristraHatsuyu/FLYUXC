/* testfx/test_logic.fx */
main:=(){
    // 测试 &&
    if(1 && 1){
        print(1)
    }{
        print(0)
    }
    
    // 测试 ||
    if(0 || 1){
        print(1)
    }{
        print(0)
    }
    
    // 测试 !
    if(!0){
        print(1)
    }{
        print(0)
    }
    
    if(!1){
        print(0)
    }{
        print(1)
    }
}
