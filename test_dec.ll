; ModuleID = 'flyux_module'
target datalayout = "e-m:o-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-apple-macosx10.15.0"

declare i32 @printf(i8*, ...)



define i32 @main() {
  %_00001 = alloca double
  %t0 = sitofp i32 5 to double
  store double %t0, double* %_00001
  %t1 = load double, double* %_00001
  %t2 = fsub double %t1, 1.0
  store double %t2, double* %_00001
  %t3 = load double, double* %_00001
  %_00002 = alloca double
  %t4 = load double, double* %_00001
  %t5 = fsub double %t4, 1.0
  store double %t5, double* %_00001
  store double %t4, double* %_00002
  %_00003 = alloca double
  %t6 = load double, double* %_00001
  %t7 = fsub double %t6, 1.0
  store double %t7, double* %_00001
  store double %t7, double* %_00003
  ret i32 0
}
