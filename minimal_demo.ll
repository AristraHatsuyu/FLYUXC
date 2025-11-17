; ModuleID = 'flyux_module'
target datalayout = "e-m:o-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-apple-macosx10.15.0"

declare i32 @printf(i8*, ...)

@.str.0 = private unnamed_addr constant [4 x i8] c"%f\0A\00"
@.str.1 = private unnamed_addr constant [4 x i8] c"%f\0A\00"
@.str.2 = private unnamed_addr constant [4 x i8] c"%f\0A\00"
@.str.3 = private unnamed_addr constant [4 x i8] c"%f\0A\00"


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
  %_00008 = alloca double
  %t11 = sitofp i32 0 to double
  store double %t11, double* %_00008
  br label %label0

label0:
  %t12 = load double, double* %_00008
  %t13 = sitofp i32 3 to double
  %t14 = fcmp olt double %t12, %t13
  br i1 %t14, label %label1, label %label3

label1:
  %_00007 = alloca double
  %t15 = load double, double* %_00007
  %t16 = sitofp i32 1 to double
  %t17 = fadd double %t15, %t16
  store double %t17, double* %_00007
  br label %label2

label2:
  %t18 = load double, double* %_00008
  %t19 = fadd double %t18, 1.0
  store double %t19, double* %_00008
  br label %label0

label3:
  %t20 = load double, double* %_00007
  %t21 = getelementptr inbounds [4 x i8], [4 x i8]* @.str.1, i32 0, i32 0
  call i32 (i8*, ...) @printf(i8* %t21, double %t20)
  %_00009 = alloca double
  %t22 = fadd double 0.0, 0.0  ; array placeholder
  store double %t22, double* %_00009
  %t23 = fadd double 0.0, 0.0  ; index access placeholder
  %t24 = getelementptr inbounds [4 x i8], [4 x i8]* @.str.2, i32 0, i32 0
  call i32 (i8*, ...) @printf(i8* %t24, double %t23)
  %t25 = load double, double* %_00004
  %t26 = sitofp i32 5 to double
  %t27 = fcmp ogt double %t25, %t26
  br i1 %t27, label %label4, label %label5

label4:
  %t28 = sitofp i32 999 to double
  %t29 = getelementptr inbounds [4 x i8], [4 x i8]* @.str.3, i32 0, i32 0
  call i32 (i8*, ...) @printf(i8* %t29, double %t28)
  br label %label6

label5:
  br label %label6

label6:
  ret i32 0
}
