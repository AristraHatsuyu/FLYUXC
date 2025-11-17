; ModuleID = 'flyux_module'
target datalayout = "e-m:o-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-apple-macosx10.15.0"

declare i32 @printf(i8*, ...)

@.str.0 = private unnamed_addr constant [4 x i8] c"%f\0A\00"
@.str.1 = private unnamed_addr constant [4 x i8] c"%f\0A\00"


define i32 @main() {
  %main = alloca double
  %_00001 = alloca double
  %t0 = sitofp i32 5 to double
  store double %t0, double* %_00001
  %t1 = load double, double* %_00001
  %t2 = sitofp i32 3 to double
  %t3 = fcmp ogt double %t1, %t2
  br i1 %t3, label %label0, label %label1

label0:
  %t4 = sitofp i32 1 to double
  %t5 = getelementptr inbounds [4 x i8], [4 x i8]* @.str.0, i32 0, i32 0
  call i32 (i8*, ...) @printf(i8* %t5, double %t4)
  br label %label2

label1:
  %t6 = sitofp i32 2 to double
  %t7 = getelementptr inbounds [4 x i8], [4 x i8]* @.str.1, i32 0, i32 0
  call i32 (i8*, ...) @printf(i8* %t7, double %t6)
  br label %label2

label2:
  ret i32 0
}
