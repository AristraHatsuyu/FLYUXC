; ModuleID = 'flyux_module'
target datalayout = "e-m:o-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-apple-macosx10.15.0"

declare i32 @printf(i8*, ...)



define double @_00001(double %param__00002) {
  %_00002 = alloca double
  store double %param__00002, double* %_00002
  %t0 = load double, double* %_00002
  %t1 = sitofp i32 1 to double
  %t2 = fadd double %t0, %t1
  ret double %t2
  ret double 0.0
}

define i32 @main() {
  %_00003 = alloca double
  %t3 = fadd double 0.0, 0.0  ; array placeholder
  store double %t3, double* %_00003
  %_00002 = alloca double
  %t4 = load double, double* %_00003
  %t5 = call double @length(double %t4)
  %t6 = sitofp i32 2 to double
  %t7 = call double @_00001(double %t5, double %t6)
  store double %t7, double* %_00002
  ret i32 0
}
