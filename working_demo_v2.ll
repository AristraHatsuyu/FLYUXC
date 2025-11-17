; ModuleID = 'flyux_module'
target datalayout = "e-m:o-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-apple-macosx10.15.0"

declare i32 @printf(i8*, ...)

@.str.0 = private unnamed_addr constant [4 x i8] c"%f\0A\00"
@.str.1 = private unnamed_addr constant [4 x i8] c"%f\0A\00"
@.str.2 = private unnamed_addr constant [4 x i8] c"%f\0A\00"
@.str.3 = private unnamed_addr constant [4 x i8] c"%f\0A\00"
@.str.4 = private unnamed_addr constant [4 x i8] c"%f\0A\00"


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
  %_00004 = alloca double
  %t3 = sitofp i32 10 to double
  store double %t3, double* %_00004
  %_00005 = alloca double
  %t4 = sitofp i32 20 to double
  store double %t4, double* %_00005
  %_00006 = alloca double
  %t5 = load double, double* %_00004
  %t6 = load double, double* %_00005
  %t7 = call double @_00001(double %t5, double %t6)
  store double %t7, double* %_00006
  %t8 = load double, double* %_00006
  %t9 = getelementptr inbounds [4 x i8], [4 x i8]* @.str.0, i32 0, i32 0
  call i32 (i8*, ...) @printf(i8* %t9, double %t8)
  %_00007 = alloca double
  %t10 = sitofp i32 0 to double
  store double %t10, double* %_00007
  br label %label0

label0:
  %t11 = load double, double* %_00007
  %t12 = sitofp i32 3 to double
  %t13 = fcmp olt double %t11, %t12
  br i1 %t13, label %label1, label %label3

label1:
  %t14 = load double, double* %_00007
  %t15 = getelementptr inbounds [4 x i8], [4 x i8]* @.str.1, i32 0, i32 0
  call i32 (i8*, ...) @printf(i8* %t15, double %t14)
  br label %label2

label2:
  %t16 = load double, double* %_00007
  %t17 = fadd double %t16, 1.0
  store double %t17, double* %_00007
  br label %label0

label3:
  %_00008 = alloca double
  %t18 = fadd double 0.0, 0.0  ; array placeholder
  store double %t18, double* %_00008
  %t19 = fadd double 0.0, 0.0  ; index access placeholder
  %t20 = getelementptr inbounds [4 x i8], [4 x i8]* @.str.2, i32 0, i32 0
  call i32 (i8*, ...) @printf(i8* %t20, double %t19)
  %t21 = load double, double* %_00004
  %t22 = sitofp i32 5 to double
  %t23 = fcmp ogt double %t21, %t22
  br i1 %t23, label %label4, label %label5

label4:
  %t24 = sitofp i32 999 to double
  %t25 = getelementptr inbounds [4 x i8], [4 x i8]* @.str.3, i32 0, i32 0
  call i32 (i8*, ...) @printf(i8* %t25, double %t24)
  br label %label6

label5:
  br label %label6

label6:
  %t26 = sitofp i32 42 to double
  %t27 = getelementptr inbounds [4 x i8], [4 x i8]* @.str.4, i32 0, i32 0
  call i32 (i8*, ...) @printf(i8* %t27, double %t26)
  ret i32 0
}
