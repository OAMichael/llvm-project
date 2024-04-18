; ModuleID = 'top'
source_filename = "top"

declare void @llvm.riscx.putpixel(i32, i32, i32)

declare void @llvm.riscx.flush()

declare i32 @llvm.riscx.rand()

define i32 @fact(i32 %0) {
BB_1:
  %1 = alloca i32, align 4
  store i32 %0, ptr %1, align 4
  %2 = alloca i32, align 4
  store i32 0, ptr %2, align 4
  %3 = getelementptr i32, ptr %1, i32 0
  %4 = load i32, ptr %3, align 4
  %5 = icmp slt i32 %4, 2
  %6 = zext i1 %5 to i32
  %7 = icmp ne i32 %6, 0
  br i1 %7, label %BB_2, label %BB_3

BB_2:                                             ; preds = %BB_1
  %8 = getelementptr i32, ptr %2, i32 0
  store i32 1, ptr %8, align 4
  br label %BB_4

BB_3:                                             ; preds = %BB_1
  %9 = getelementptr i32, ptr %2, i32 0
  %10 = getelementptr i32, ptr %1, i32 0
  %11 = load i32, ptr %10, align 4
  %12 = getelementptr i32, ptr %1, i32 0
  %13 = load i32, ptr %12, align 4
  %14 = sub i32 %13, 1
  %15 = call i32 @fact(i32 %14)
  %16 = mul i32 %11, %15
  store i32 %16, ptr %9, align 4
  br label %BB_4

BB_4:                                             ; preds = %BB_3, %BB_2
  %17 = getelementptr i32, ptr %2, i32 0
  %18 = load i32, ptr %17, align 4
  ret i32 %18
}

define i32 @app() {
BB_1:
  %0 = call i32 @fact(i32 8)
  ret i32 %0
}
