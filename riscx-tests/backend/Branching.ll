; ModuleID = 'top'
source_filename = "top"


define i32 @app() {
BB_1:
  %0 = alloca i32
  store i32 1, i32* %0
  %1 = alloca i32
  store i32 5, i32* %1
  %2 = getelementptr i32, i32* %1, i32 0
  %3 = load i32, i32* %2
  %4 = icmp slt i32 %3, 10
  %5 = zext i1 %4 to i32
  %6 = icmp ne i32 %5, 0
  br i1 %6, label %BB_2, label %BB_3

BB_2:                                             ; preds = %BB_1
  %7 = getelementptr i32, i32* %0, i32 0
  store i32 2, i32* %7
  br label %BB_4

BB_3:                                             ; preds = %BB_1
  %8 = getelementptr i32, i32* %0, i32 0
  store i32 3, i32* %8
  br label %BB_4

BB_4:                                             ; preds = %BB_3, %BB_2
  %9 = getelementptr i32, i32* %0, i32 0
  %10 = load i32, i32* %9
  ret i32 %10
}

