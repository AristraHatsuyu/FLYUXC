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
  %t3 = sitofp i32 10 to double
  %t4 = fsub double 0.0, %t3
  store double %t4, double* %_00004
  %_00005 = alloca double
  %t5 = load double, double* %_00004
  %t6 = sitofp i32 20 to double
  %t7 = call double @_00001(double %t5, double %t6)
  store double %t7, double* %_00005
  %_00006 = alloca double
  %t8 = sitofp i32 0 to double
  store double %t8, double* %_00006
  br label %label0

label0:
  %t9 = load double, double* %_00006
  %t10 = sitofp i32 3 to double
  %t11 = fcmp olt double %t9, %t10
  br i1 %t11, label %label1, label %label3

label1:
  %t12 = load double, double* %_00005
  %t13 = load double, double* %_00006
  %t14 = fadd double %t12, %t13
  %t15 = getelementptr inbounds [4 x i8], [4 x i8]* @.str.0, i32 0, i32 0
  call i32 (i8*, ...) @printf(i8* %t15, double %t14)
  br label %label2

label2:
  %t16 = load double, double* %_00006
  %t17 = sitofp i32 1 to double
  %t18 = fadd double %t16, %t17
  store double %t18, double* %_00006
  br label %label0

label3:
  ret i32 0
}
