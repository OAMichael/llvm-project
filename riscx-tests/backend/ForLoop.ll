; ModuleID = 'top'
source_filename = "top"


define i32 @app() {
BB_1:
  %0 = alloca i32
  store i32 1, i32* %0
  %1 = alloca i32
  store i32 1, i32* %1
  br label %BB_2

BB_2:                                             ; preds = %BB_4, %BB_1
  %2 = getelementptr i32, i32* %1, i32 0
  %3 = load i32, i32* %2
  %4 = icmp slt i32 %3, 7
  %5 = zext i1 %4 to i32
  %6 = icmp ne i32 %5, 0
  br i1 %6, label %BB_3, label %BB_5

BB_3:                                             ; preds = %BB_2
  %7 = getelementptr i32, i32* %0, i32 0
  %8 = getelementptr i32, i32* %0, i32 0
  %9 = load i32, i32* %8
  %10 = getelementptr i32, i32* %1, i32 0
  %11 = load i32, i32* %10
  %12 = mul i32 %9, %11
  store i32 %12, i32* %7
  br label %BB_4

BB_4:                                             ; preds = %BB_3
  %13 = getelementptr i32, i32* %1, i32 0
  %14 = getelementptr i32, i32* %1, i32 0
  %15 = load i32, i32* %14
  %16 = add i32 %15, 1
  store i32 %16, i32* %13
  br label %BB_2

BB_5:                                             ; preds = %BB_2
  %17 = getelementptr i32, i32* %0, i32 0
  %18 = load i32, i32* %17
  ret i32 %18
}

