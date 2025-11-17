; ModuleID = 'flyux_module'
target datalayout = "e-m:o-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-apple-macosx10.15.0"

declare i32 @printf(i8*, ...)



define double @_00001(double %param__00002, double %param__00003) {
  %_00002 = alloca double
  store double %param__00002, double* %_00002
  %_00003 = alloca double
  store double %param__00003, double* %_00003
  %t0 = load double, double* %_00002
  %t1 = load double, double* %_00003
  %t2 = load double, double* %_00002
  %t3 = fmul double %t1, %t2
  %t4 = fadd double %t0, %t3
  ret double %t4
  ret double 0.0
}

define i32 @main() {
  ret i32 0
}
