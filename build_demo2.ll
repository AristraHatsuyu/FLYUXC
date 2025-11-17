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
  %t6 = sitofp i32 0 to double
  store double %t6, double* %_00005
  br label %label0

label0:
  %t7 = load double, double* %_00005
  %t8 = sitofp i32 2 to double
  %t9 = fcmp olt double %t7, %t8
  br i1 %t9, label %label1, label %label3

label1:
  %t10 = fadd double 0.0, 0.0  ; index access placeholder
  %t11 = getelementptr inbounds [4 x i8], [4 x i8]* @.str.0, i32 0, i32 0
  call i32 (i8*, ...) @printf(i8* %t11, double %t10)
  br label %label2

label2:
  %t12 = load double, double* %_00005
  %t13 = fadd double %t12, 1.0
  store double %t13, double* %_00005
  br label %label0

label3:
  ret i32 0
}
