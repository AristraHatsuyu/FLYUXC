; ModuleID = 'flyux_module'
target datalayout = "e-m:o-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-apple-macosx10.15.0"

declare i32 @printf(i8*, ...)

@.str.0 = private unnamed_addr constant [4 x i8] c"%f\0A\00"


define i32 @main() {
  %_00001 = alloca double
  %t0 = sitofp i32 0 to double
  store double %t0, double* %_00001
  %t1 = sitofp i32 0 to double
  store double %t1, double* %_00001
  br label %label0

label0:
  %t2 = load double, double* %_00001
  %t3 = sitofp i32 3 to double
  %t4 = fcmp olt double %t2, %t3
  br i1 %t4, label %label1, label %label3

label1:
  %t5 = load double, double* %_00001
  %t6 = getelementptr inbounds [4 x i8], [4 x i8]* @.str.0, i32 0, i32 0
  call i32 (i8*, ...) @printf(i8* %t6, double %t5)
  br label %label2

label2:
  %t7 = load double, double* %_00001
  %t8 = sitofp i32 1 to double
  %t9 = fadd double %t7, %t8
  store double %t9, double* %_00001
  br label %label0

label3:
  ret i32 0
}
