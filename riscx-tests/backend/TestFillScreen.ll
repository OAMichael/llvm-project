; ModuleID = 'top'
source_filename = "top"

define void @simPutPixel(i32, i32, i32) {
BB_1:
  ret void
}

define void @simFlush() {
  ret void
}

define i32 @app() {
BB_1:
  %0 = alloca i32
  store i32 0, i32* %0
  br label %BB_2

BB_2:                                             ; preds = %BB_4, %BB_1
  %1 = getelementptr i32, i32* %0, i32 0
  %2 = load i32, i32* %1
  %3 = icmp slt i32 %2, 800
  %4 = zext i1 %3 to i32
  %5 = icmp ne i32 %4, 0
  br i1 %5, label %BB_3, label %BB_5

BB_3:                                             ; preds = %BB_2
  %6 = alloca i32
  store i32 0, i32* %6
  br label %BB_6

BB_4:                                             ; preds = %BB_9
  %7 = getelementptr i32, i32* %0, i32 0
  %8 = getelementptr i32, i32* %0, i32 0
  %9 = load i32, i32* %8
  %10 = add i32 %9, 1
  store i32 %10, i32* %7
  br label %BB_2

BB_5:                                             ; preds = %BB_2
  call void @simFlush()
  br label %BB_10

BB_6:                                             ; preds = %BB_8, %BB_3
  %11 = getelementptr i32, i32* %6, i32 0
  %12 = load i32, i32* %11
  %13 = icmp slt i32 %12, 800
  %14 = zext i1 %13 to i32
  %15 = icmp ne i32 %14, 0
  br i1 %15, label %BB_7, label %BB_9

BB_7:                                             ; preds = %BB_6
  %16 = getelementptr i32, i32* %0, i32 0
  %17 = load i32, i32* %16
  %18 = getelementptr i32, i32* %6, i32 0
  %19 = load i32, i32* %18
  call void @simPutPixel(i32 %17, i32 %19, i32 -16711936)
  br label %BB_8

BB_8:                                             ; preds = %BB_7
  %20 = getelementptr i32, i32* %6, i32 0
  %21 = getelementptr i32, i32* %6, i32 0
  %22 = load i32, i32* %21
  %23 = add i32 %22, 1
  store i32 %23, i32* %20
  br label %BB_6

BB_9:                                             ; preds = %BB_6
  br label %BB_4

BB_10:                                            ; preds = %BB_11, %BB_5
  br i1 true, label %BB_11, label %BB_12

BB_11:                                            ; preds = %BB_10
  br label %BB_10

BB_12:                                            ; preds = %BB_10
  ret i32 1
}

