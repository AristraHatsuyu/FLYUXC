; ModuleID = 'flyux_module'
target datalayout = "e-m:o-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-apple-macosx10.15.0"

declare i32 @printf(i8*, ...)



define double @_00001(double %param__00002) {
  %_00002 = alloca double
  store double %param__00002, double* %_00002
  %t0 = load double, double* %_00002
  ret double %t0
  ret double 0.0
}

define i32 @main() {
  %_00003 = alloca double
  %t1 = sitofp i32 5 to double
  store double %t1, double* %_00003
  %_00004 = alloca double
  %t2 = sitofp i32 1 to double
  store double %t2, double* %_00004
  %_00002 = alloca double
  %t3 = load double, double* %_00003
  %t4 = load double, double* %_00004
  %t5 = call double @_00001(double %t3, double %t4)
  store double %t5, double* %_00002
  ret i32 0
}
