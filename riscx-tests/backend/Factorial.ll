; ModuleID = 'top'
source_filename = "top"


define i32 @fact(i32 %0) {
BB_1:
  %1 = alloca i32
  store i32 %0, i32* %1
  %2 = alloca i32
  store i32 0, i32* %2
  %3 = getelementptr i32, i32* %1, i32 0
  %4 = load i32, i32* %3
  %5 = icmp slt i32 %4, 2
  %6 = zext i1 %5 to i32
  %7 = icmp ne i32 %6, 0
  br i1 %7, label %BB_2, label %BB_3

BB_2:                                             ; preds = %BB_1
  %8 = getelementptr i32, i32* %2, i32 0
  store i32 1, i32* %8
  br label %BB_4

BB_3:                                             ; preds = %BB_1
  %9 = getelementptr i32, i32* %2, i32 0
  %10 = getelementptr i32, i32* %1, i32 0
  %11 = load i32, i32* %10
  %12 = getelementptr i32, i32* %1, i32 0
  %13 = load i32, i32* %12
  %14 = sub i32 %13, 1
  %15 = call i32 @fact(i32 %14)
  %16 = mul i32 %11, %15
  store i32 %16, i32* %9
  br label %BB_4

BB_4:                                             ; preds = %BB_3, %BB_2
  %17 = getelementptr i32, i32* %2, i32 0
  %18 = load i32, i32* %17
  ret i32 %18
}

define i32 @app() {
BB_1:
  %0 = call i32 @fact(i32 8)
  ret i32 %0
}

