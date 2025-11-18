/* Complex test: nested parentheses, object inside array, main filtering, unicode id */
/* leading unicode name */
🐶 :[num]= 1
arr :[obj]= [{a:1},{b:2}];
// root-level expression that should be removed when main exists
_global := 999;

testfunc := () {
    R> 123;
}

testfunc()

main := () {
    x := ((1 + 2));
    y := arr[0].a
    z := ( ( (x) ) );
    print(x,y,z)
    w := {a: 1}
}

// trailing newlines and spaces

