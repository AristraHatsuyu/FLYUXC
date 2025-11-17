; ModuleID = 'flyux_module'
target datalayout = "e-m:o-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-apple-macosx10.15.0"

declare i32 @printf(i8*, ...)

@.str.0 = private unnamed_addr constant [4 x i8] c"%f\0A\00"


define double @_00001(double %param__00002, double %param__00003) {
  %_00002 = alloca double
  store double %param__00002, double* %_00002
  %_00003 = alloca double
  store double %param__00003, double* %_00003
  %t0 = load double, double* %_00002
  %t1 = load double, double* %_00003
  %t2 = fadd double %t0, %t1
  ret double %t2
  ret double 0.0
}

define i32 @main() {
  %_00004 = alloca double
  %t3 = sitofp i32 3 to double
  %t4 = sitofp i32 5 to double
  %t5 = call double @_00001(double %t3, double %t4)
  store double %t5, double* %_00004
  %t6 = load double, double* %_00004
  %t7 = getelementptr inbounds [4 x i8], [4 x i8]* @.str.0, i32 0, i32 0
  call i32 (i8*, ...) @printf(i8* %t7, double %t6)
  ret i32 0
}
