; ModuleID = 'flyux_module'
target datalayout = "e-m:o-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-apple-macosx10.15.0"

declare i32 @printf(i8*, ...)

@.str.0 = private unnamed_addr constant [4 x i8] c"%f\0A\00"


define i32 @main() {
  %_00001 = alloca double
  %t0 = sitofp i32 5 to double
  %t1 = fsub double 0.0, %t0
  store double %t1, double* %_00001
  %t2 = load double, double* %_00001
  %t3 = getelementptr inbounds [4 x i8], [4 x i8]* @.str.0, i32 0, i32 0
  call i32 (i8*, ...) @printf(i8* %t3, double %t2)
  ret i32 0
}
