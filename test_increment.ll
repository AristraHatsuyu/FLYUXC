; ModuleID = 'flyux_module'
target datalayout = "e-m:o-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-apple-macosx10.15.0"

declare i32 @printf(i8*, ...)

@.str.0 = private unnamed_addr constant [4 x i8] c"%f\0A\00"
@.str.1 = private unnamed_addr constant [4 x i8] c"%f\0A\00"
@.str.2 = private unnamed_addr constant [4 x i8] c"%f\0A\00"
@.str.3 = private unnamed_addr constant [4 x i8] c"%f\0A\00"
@.str.4 = private unnamed_addr constant [4 x i8] c"%f\0A\00"


define i32 @main() {
  %_00001 = alloca double
  %t0 = sitofp i32 0 to double
  store double %t0, double* %_00001
  %t1 = load double, double* %_00001
  %t2 = getelementptr inbounds [4 x i8], [4 x i8]* @.str.0, i32 0, i32 0
  call i32 (i8*, ...) @printf(i8* %t2, double %t1)
  %t3 = load double, double* %_00001
  %t5 = fadd double %t3, (null)
  %t6 = getelementptr inbounds [4 x i8], [4 x i8]* @.str.1, i32 0, i32 0
  call i32 (i8*, ...) @printf(i8* %t6, double %t5)
  %t7 = load double, double* %_00001
  %t8 = getelementptr inbounds [4 x i8], [4 x i8]* @.str.2, i32 0, i32 0
  call i32 (i8*, ...) @printf(i8* %t8, double %t7)
  %t9 = load double, double* %_00001
  %t12 = getelementptr inbounds [4 x i8], [4 x i8]* @.str.3, i32 0, i32 0
  call i32 (i8*, ...) @printf(i8* %t12, double %t9)
  %t13 = load double, double* %_00001
  %t14 = getelementptr inbounds [4 x i8], [4 x i8]* @.str.4, i32 0, i32 0
  call i32 (i8*, ...) @printf(i8* %t14, double %t13)
  ret i32 0
}
