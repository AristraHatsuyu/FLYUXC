; ModuleID = 'flyux_module'
target datalayout = "e-m:o-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-apple-macosx10.15.0"

declare i32 @printf(i8*, ...)



define i32 @main() {
  %_00001 = alloca double
  %t0 = fadd double 0.0, 0.0  ; object placeholder
  store double %t0, double* %_00001
  %_00002 = alloca double
  %t1 = fadd double 0.0, 0.0  ; member access placeholder
  store double %t1, double* %_00002
  ret i32 0
}
