; ModuleID = 'top'
source_filename = "top"


define i32 @app() {
BB_1:
  %0 = alloca i32
  store i32 0, i32* %0
  %1 = alloca i32
  store i32 0, i32* %1
  br label %BB_2

BB_2:                                             ; preds = %BB_4, %BB_1
  %2 = getelementptr i32, i32* %1, i32 0
  %3 = load i32, i32* %2
  %4 = icmp slt i32 %3, 20
  %5 = zext i1 %4 to i32
  %6 = icmp ne i32 %5, 0
  br i1 %6, label %BB_3, label %BB_5

BB_3:                                             ; preds = %BB_2
  %7 = alloca i32
  store i32 0, i32* %7
  br label %BB_6

BB_4:                                             ; preds = %BB_9
  %8 = getelementptr i32, i32* %1, i32 0
  %9 = getelementptr i32, i32* %1, i32 0
  %10 = load i32, i32* %9
  %11 = add i32 %10, 1
  store i32 %11, i32* %8
  br label %BB_2

BB_5:                                             ; preds = %BB_2
  %12 = getelementptr i32, i32* %0, i32 0
  %13 = load i32, i32* %12
  ret i32 %13

BB_6:                                             ; preds = %BB_8, %BB_3
  %14 = getelementptr i32, i32* %7, i32 0
  %15 = load i32, i32* %14
  %16 = icmp slt i32 %15, 20
  %17 = zext i1 %16 to i32
  %18 = icmp ne i32 %17, 0
  br i1 %18, label %BB_7, label %BB_9

BB_7:                                             ; preds = %BB_6
  %19 = getelementptr i32, i32* %0, i32 0
  %20 = getelementptr i32, i32* %0, i32 0
  %21 = load i32, i32* %20
  %22 = add i32 %21, 1
  store i32 %22, i32* %19
  br label %BB_8

BB_8:                                             ; preds = %BB_7
  %23 = getelementptr i32, i32* %7, i32 0
  %24 = getelementptr i32, i32* %7, i32 0
  %25 = load i32, i32* %24
  %26 = add i32 %25, 1
  store i32 %26, i32* %23
  br label %BB_6

BB_9:                                             ; preds = %BB_6
  br label %BB_4
}

