; ModuleID = 'top'
source_filename = "top"

declare void @llvm.riscx.putpixel(i32, i32, i32)

declare void @llvm.riscx.flush()

declare i32 @llvm.riscx.rand()

define i32 @app() {
BB_1:
  %0 = alloca i32, align 4
  store i32 1, ptr %0, align 4
  %1 = alloca i32, align 4
  store i32 5, ptr %1, align 4
  %2 = getelementptr i32, ptr %1, i32 0
  %3 = load i32, ptr %2, align 4
  %4 = icmp slt i32 %3, 10
  %5 = zext i1 %4 to i32
  %6 = icmp ne i32 %5, 0
  br i1 %6, label %BB_2, label %BB_3

BB_2:                                             ; preds = %BB_1
  %7 = getelementptr i32, ptr %0, i32 0
  store i32 2, ptr %7, align 4
  br label %BB_4

BB_3:                                             ; preds = %BB_1
  %8 = getelementptr i32, ptr %0, i32 0
  store i32 3, ptr %8, align 4
  br label %BB_4

BB_4:                                             ; preds = %BB_3, %BB_2
  %9 = getelementptr i32, ptr %0, i32 0
  %10 = load i32, ptr %9, align 4
  ret i32 %10
}
