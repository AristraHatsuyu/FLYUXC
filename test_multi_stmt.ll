; ModuleID = 'flyux_module'
target datalayout = "e-m:o-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-apple-macosx10.15.0"

declare i32 @printf(i8*, ...)



define i32 @main() {
  %main = alloca double
  %_00001 = alloca double
  %t0 = sitofp i32 1 to double
  store double %t0, double* %_00001
  %_00002 = alloca double
  %t1 = sitofp i32 2 to double
  store double %t1, double* %_00002
  %_00003 = alloca double
  %t2 = sitofp i32 3 to double
  store double %t2, double* %_00003
  ret i32 0
}
