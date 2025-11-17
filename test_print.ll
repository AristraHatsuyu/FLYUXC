; ModuleID = 'flyux_module'
target datalayout = "e-m:o-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-apple-macosx10.15.0"

declare i32 @printf(i8*, ...)

@.str.0 = private unnamed_addr constant [4 x i8] c"%f\0A\00"
@.str.1 = private unnamed_addr constant [4 x i8] c"%f\0A\00"


define i32 @main() {
  %_00001 = alloca double
  %t0 = sitofp i32 42 to double
  store double %t0, double* %_00001
  %t1 = load double, double* %_00001
  %t2 = getelementptr inbounds [4 x i8], [4 x i8]* @.str.0, i32 0, i32 0
  call i32 (i8*, ...) @printf(i8* %t2, double %t1)
  %_00002 = alloca double
  %t3 = load double, double* %_00001
  %t4 = sitofp i32 2 to double
  %t5 = fmul double %t3, %t4
  store double %t5, double* %_00002
  %t6 = load double, double* %_00002
  %t7 = getelementptr inbounds [4 x i8], [4 x i8]* @.str.1, i32 0, i32 0
  call i32 (i8*, ...) @printf(i8* %t7, double %t6)
  ret i32 0
}
