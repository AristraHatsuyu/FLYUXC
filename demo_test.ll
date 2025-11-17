; ModuleID = 'flyux_module'
target datalayout = "e-m:o-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-apple-macosx10.15.0"

declare i32 @printf(i8*, ...)

@.str.0 = private unnamed_addr constant [4 x i8] c"%f\0A\00"


define i32 @main() {
  %main = alloca double
  %_00001 = alloca double
  %t0 = fadd double 0.0, 0.0  ; array placeholder
  store double %t0, double* %_00001
  %t1 = fadd double 0.0, 0.0  ; index access placeholder
  %t2 = getelementptr inbounds [4 x i8], [4 x i8]* @.str.0, i32 0, i32 0
  call i32 (i8*, ...) @printf(i8* %t2, double %t1)
  ret i32 0
}
