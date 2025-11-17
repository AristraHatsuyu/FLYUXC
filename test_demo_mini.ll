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
  %main = alloca double
  %_00004 = alloca double
  %t5 = fadd double 0.0, 0.0  ; array placeholder
  store double %t5, double* %_00004
  %_00005 = alloca double
  %t6 = fadd double 0.0, 0.0  ; object placeholder
  store double %t6, double* %_00005
  %_00006 = alloca double
  %t7 = load double, double* %_00004
  %t8 = call double @length(double %t7)
  %t9 = sitofp i32 2 to double
  %t10 = call double @_00001(double %t8, double %t9)
  store double %t10, double* %_00006
  ret i32 0
}
