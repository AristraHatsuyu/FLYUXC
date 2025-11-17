; ModuleID = 'flyux_module'
target datalayout = "e-m:o-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-apple-macosx10.15.0"

declare i32 @printf(i8*, ...)



define i32 @main() {
  %_00001 = alloca double
  %t0 = sitofp i32 5 to double
  store double %t0, double* %_00001
  %_00002 = alloca double
  %t1 = sitofp i32 1 to double
  store double %t1, double* %_00002
  %_00003 = alloca double
  %t2 = load double, double* %_00001
  %t3 = sitofp i32 3 to double
  %t4 = load double, double* %_00002
  %t5 = sitofp i32 2 to double
  %t6 = fadd double %t4, %t5
  %t7 = fmul double %t3, %t6
  %t8 = fcmp ogt double %t2, %t7
  store double %t8, double* %_00003
  ret i32 0
}
