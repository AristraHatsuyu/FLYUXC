; ModuleID = 'flyux_module'
target datalayout = "e-m:o-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-apple-macosx10.15.0"

declare i32 @printf(i8*, ...)

@.str.0 = private unnamed_addr constant [4 x i8] c"%f\0A\00"
@.str.1 = private unnamed_addr constant [4 x i8] c"%f\0A\00"
@.str.2 = private unnamed_addr constant [4 x i8] c"%f\0A\00"


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
  %main = alloca double
  %_00002 = alloca double
  %t3 = sitofp i32 10 to double
  store double %t3, double* %_00002
  %_00003 = alloca double
  %t4 = sitofp i32 20 to double
  store double %t4, double* %_00003
  %_00004 = alloca double
  %t5 = load double, double* %_00002
  %t6 = load double, double* %_00003
  %t7 = call double @_00001(double %t5, double %t6)
  store double %t7, double* %_00004
  %t8 = load double, double* %_00004
  %t9 = getelementptr inbounds [4 x i8], [4 x i8]* @.str.0, i32 0, i32 0
  call i32 (i8*, ...) @printf(i8* %t9, double %t8)
  %_00005 = alloca double
  %t10 = sitofp i32 0 to double
  store double %t10, double* %_00005
  %t11 = load double, double* %_00005
  %t12 = fadd double %t11, 1.0
  store double %t12, double* %_00005
  %t13 = load double, double* %_00005
  %t14 = getelementptr inbounds [4 x i8], [4 x i8]* @.str.1, i32 0, i32 0
  call i32 (i8*, ...) @printf(i8* %t14, double %t13)
  %_00006 = alloca double
  %t15 = sitofp i32 0 to double
  store double %t15, double* %_00006
  br label %label0

label0:
  %t16 = load double, double* %_00006
  %t17 = sitofp i32 3 to double
  %t18 = fcmp olt double %t16, %t17
  br i1 %t18, label %label1, label %label3

label1:
  %t19 = load double, double* %_00006
  %t20 = getelementptr inbounds [4 x i8], [4 x i8]* @.str.2, i32 0, i32 0
  call i32 (i8*, ...) @printf(i8* %t20, double %t19)
  br label %label2

label2:
  %t21 = load double, double* %_00006
  %t22 = fadd double %t21, 1.0
  store double %t22, double* %_00006
  br label %label0

label3:
  ret i32 0
}
