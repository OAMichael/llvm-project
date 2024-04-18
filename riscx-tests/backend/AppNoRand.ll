; ModuleID = 'top'
source_filename = "top"

@allCells = internal global [5000 x i32] zeroinitializer, align 16
@swapCells = internal global [5000 x i32] zeroinitializer, align 16

declare void @llvm.riscx.putpixel(i32, i32, i32)

declare void @llvm.riscx.flush()

declare i32 @llvm.riscx.rand()

define i32 @makeColor(i32 %0, i32 %1, i32 %2) {
BB_1:
  %3 = alloca i32, align 4
  store i32 %0, ptr %3, align 4
  %4 = alloca i32, align 4
  store i32 %1, ptr %4, align 4
  %5 = alloca i32, align 4
  store i32 %2, ptr %5, align 4
  %6 = alloca i32, align 4
  store i32 255, ptr %6, align 4
  %7 = alloca i32, align 4
  store i32 0, ptr %7, align 4
  %8 = getelementptr i32, ptr %7, i32 0
  %9 = getelementptr i32, ptr %7, i32 0
  %10 = load i32, ptr %9, align 4
  %11 = getelementptr i32, ptr %6, i32 0
  %12 = load i32, ptr %11, align 4
  %13 = and i32 %12, 255
  %14 = shl i32 %13, 24
  %15 = or i32 %10, %14
  store i32 %15, ptr %8, align 4
  %16 = getelementptr i32, ptr %7, i32 0
  %17 = getelementptr i32, ptr %7, i32 0
  %18 = load i32, ptr %17, align 4
  %19 = getelementptr i32, ptr %3, i32 0
  %20 = load i32, ptr %19, align 4
  %21 = and i32 %20, 255
  %22 = shl i32 %21, 16
  %23 = or i32 %18, %22
  store i32 %23, ptr %16, align 4
  %24 = getelementptr i32, ptr %7, i32 0
  %25 = getelementptr i32, ptr %7, i32 0
  %26 = load i32, ptr %25, align 4
  %27 = getelementptr i32, ptr %4, i32 0
  %28 = load i32, ptr %27, align 4
  %29 = and i32 %28, 255
  %30 = shl i32 %29, 8
  %31 = or i32 %26, %30
  store i32 %31, ptr %24, align 4
  %32 = getelementptr i32, ptr %7, i32 0
  %33 = getelementptr i32, ptr %7, i32 0
  %34 = load i32, ptr %33, align 4
  %35 = getelementptr i32, ptr %5, i32 0
  %36 = load i32, ptr %35, align 4
  %37 = and i32 %36, 255
  %38 = shl i32 %37, 0
  %39 = or i32 %34, %38
  store i32 %39, ptr %32, align 4
  %40 = getelementptr i32, ptr %7, i32 0
  %41 = load i32, ptr %40, align 4
  ret i32 %41
}

define void @paintCellPixels(i32 %0, i32 %1, i32 %2) {
BB_1:
  %3 = alloca i32, align 4
  store i32 %0, ptr %3, align 4
  %4 = alloca i32, align 4
  store i32 %1, ptr %4, align 4
  %5 = alloca i32, align 4
  store i32 %2, ptr %5, align 4
  %6 = getelementptr i32, ptr %3, i32 0
  %7 = load i32, ptr %6, align 4
  %8 = mul i32 %7, 16
  %9 = add i32 %8, 1
  %10 = alloca i32, align 4
  store i32 %9, ptr %10, align 4
  %11 = getelementptr i32, ptr %4, i32 0
  %12 = load i32, ptr %11, align 4
  %13 = mul i32 %12, 16
  %14 = add i32 %13, 1
  %15 = alloca i32, align 4
  store i32 %14, ptr %15, align 4
  %16 = getelementptr i32, ptr %3, i32 0
  %17 = load i32, ptr %16, align 4
  %18 = mul i32 %17, 16
  %19 = add i32 %18, 16
  %20 = sub i32 %19, 1
  %21 = alloca i32, align 4
  store i32 %20, ptr %21, align 4
  %22 = getelementptr i32, ptr %4, i32 0
  %23 = load i32, ptr %22, align 4
  %24 = mul i32 %23, 16
  %25 = add i32 %24, 16
  %26 = sub i32 %25, 1
  %27 = alloca i32, align 4
  store i32 %26, ptr %27, align 4
  %28 = getelementptr i32, ptr %15, i32 0
  %29 = load i32, ptr %28, align 4
  %30 = alloca i32, align 4
  store i32 %29, ptr %30, align 4
  br label %BB_2

BB_2:                                             ; preds = %BB_4, %BB_1
  %31 = getelementptr i32, ptr %30, i32 0
  %32 = load i32, ptr %31, align 4
  %33 = getelementptr i32, ptr %27, i32 0
  %34 = load i32, ptr %33, align 4
  %35 = icmp slt i32 %32, %34
  %36 = zext i1 %35 to i32
  %37 = icmp ne i32 %36, 0
  br i1 %37, label %BB_3, label %BB_5

BB_3:                                             ; preds = %BB_2
  %38 = getelementptr i32, ptr %10, i32 0
  %39 = load i32, ptr %38, align 4
  %40 = alloca i32, align 4
  store i32 %39, ptr %40, align 4
  br label %BB_6

BB_4:                                             ; preds = %BB_9
  %41 = getelementptr i32, ptr %30, i32 0
  %42 = getelementptr i32, ptr %30, i32 0
  %43 = load i32, ptr %42, align 4
  %44 = add i32 %43, 1
  store i32 %44, ptr %41, align 4
  br label %BB_2

BB_5:                                             ; preds = %BB_2
  ret void

BB_6:                                             ; preds = %BB_8, %BB_3
  %45 = getelementptr i32, ptr %40, i32 0
  %46 = load i32, ptr %45, align 4
  %47 = getelementptr i32, ptr %21, i32 0
  %48 = load i32, ptr %47, align 4
  %49 = icmp slt i32 %46, %48
  %50 = zext i1 %49 to i32
  %51 = icmp ne i32 %50, 0
  br i1 %51, label %BB_7, label %BB_9

BB_7:                                             ; preds = %BB_6
  %52 = getelementptr i32, ptr %40, i32 0
  %53 = load i32, ptr %52, align 4
  %54 = getelementptr i32, ptr %30, i32 0
  %55 = load i32, ptr %54, align 4
  %56 = getelementptr i32, ptr %5, i32 0
  %57 = load i32, ptr %56, align 4
  call void @llvm.riscx.putpixel(i32 %53, i32 %55, i32 %57)
  br label %BB_8

BB_8:                                             ; preds = %BB_7
  %58 = getelementptr i32, ptr %40, i32 0
  %59 = getelementptr i32, ptr %40, i32 0
  %60 = load i32, ptr %59, align 4
  %61 = add i32 %60, 1
  store i32 %61, ptr %58, align 4
  br label %BB_6

BB_9:                                             ; preds = %BB_6
  br label %BB_4
}

define void @initCells() {
BB_1:
  %0 = alloca i32, align 4
  store i32 0, ptr %0, align 4
  br label %BB_2

BB_2:                                             ; preds = %BB_4, %BB_1
  %1 = getelementptr i32, ptr %0, i32 0
  %2 = load i32, ptr %1, align 4
  %3 = icmp slt i32 %2, 50
  %4 = zext i1 %3 to i32
  %5 = icmp ne i32 %4, 0
  br i1 %5, label %BB_3, label %BB_5

BB_3:                                             ; preds = %BB_2
  %6 = alloca i32, align 4
  store i32 0, ptr %6, align 4
  br label %BB_6

BB_4:                                             ; preds = %BB_9
  %7 = getelementptr i32, ptr %0, i32 0
  %8 = getelementptr i32, ptr %0, i32 0
  %9 = load i32, ptr %8, align 4
  %10 = add i32 %9, 1
  store i32 %10, ptr %7, align 4
  br label %BB_2

BB_5:                                             ; preds = %BB_2
  ret void

BB_6:                                             ; preds = %BB_8, %BB_3
  %11 = getelementptr i32, ptr %6, i32 0
  %12 = load i32, ptr %11, align 4
  %13 = icmp slt i32 %12, 50
  %14 = zext i1 %13 to i32
  %15 = icmp ne i32 %14, 0
  br i1 %15, label %BB_7, label %BB_9

BB_7:                                             ; preds = %BB_6
  %16 = getelementptr i32, ptr %6, i32 0
  %17 = load i32, ptr %16, align 4
  %18 = getelementptr i32, ptr %0, i32 0
  %19 = load i32, ptr %18, align 4
  %20 = add i32 %17, %19
  %21 = alloca i32, align 4
  store i32 %20, ptr %21, align 4
  %22 = getelementptr i32, ptr %21, i32 0
  %23 = load i32, ptr %22, align 4
  %24 = srem i32 %23, 31
  %25 = icmp eq i32 %24, 0
  %26 = zext i1 %25 to i32
  %27 = icmp ne i32 %26, 0
  br i1 %27, label %BB_10, label %BB_11

BB_8:                                             ; preds = %BB_11
  %28 = getelementptr i32, ptr %6, i32 0
  %29 = getelementptr i32, ptr %6, i32 0
  %30 = load i32, ptr %29, align 4
  %31 = add i32 %30, 1
  store i32 %31, ptr %28, align 4
  br label %BB_6

BB_9:                                             ; preds = %BB_6
  br label %BB_4

BB_10:                                            ; preds = %BB_7
  %32 = getelementptr i32, ptr %6, i32 0
  %33 = load i32, ptr %32, align 4
  %34 = getelementptr i32, ptr %0, i32 0
  %35 = load i32, ptr %34, align 4
  %36 = mul i32 %35, 50
  %37 = add i32 %33, %36
  %38 = mul i32 2, %37
  %39 = getelementptr [5000 x i32], ptr @allCells, i32 0, i32 %38
  store i32 1, ptr %39, align 4
  %40 = getelementptr i32, ptr %6, i32 0
  %41 = load i32, ptr %40, align 4
  %42 = getelementptr i32, ptr %0, i32 0
  %43 = load i32, ptr %42, align 4
  %44 = mul i32 %43, 50
  %45 = add i32 %41, %44
  %46 = mul i32 2, %45
  %47 = add i32 %46, 1
  %48 = getelementptr [5000 x i32], ptr @allCells, i32 0, i32 %47
  store i32 1, ptr %48, align 4
  %49 = call i32 @makeColor(i32 0, i32 255, i32 0)
  %50 = alloca i32, align 4
  store i32 %49, ptr %50, align 4
  %51 = getelementptr i32, ptr %6, i32 0
  %52 = load i32, ptr %51, align 4
  %53 = getelementptr i32, ptr %0, i32 0
  %54 = load i32, ptr %53, align 4
  %55 = getelementptr i32, ptr %50, i32 0
  %56 = load i32, ptr %55, align 4
  call void @paintCellPixels(i32 %52, i32 %54, i32 %56)
  br label %BB_11

BB_11:                                            ; preds = %BB_10, %BB_7
  br label %BB_8
}

define void @updatePixels() {
BB_1:
  %0 = alloca i32, align 4
  store i32 0, ptr %0, align 4
  br label %BB_2

BB_2:                                             ; preds = %BB_4, %BB_1
  %1 = getelementptr i32, ptr %0, i32 0
  %2 = load i32, ptr %1, align 4
  %3 = icmp slt i32 %2, 50
  %4 = zext i1 %3 to i32
  %5 = icmp ne i32 %4, 0
  br i1 %5, label %BB_3, label %BB_5

BB_3:                                             ; preds = %BB_2
  %6 = alloca i32, align 4
  store i32 0, ptr %6, align 4
  br label %BB_6

BB_4:                                             ; preds = %BB_9
  %7 = getelementptr i32, ptr %0, i32 0
  %8 = getelementptr i32, ptr %0, i32 0
  %9 = load i32, ptr %8, align 4
  %10 = add i32 %9, 1
  store i32 %10, ptr %7, align 4
  br label %BB_2

BB_5:                                             ; preds = %BB_2
  %11 = alloca i32, align 4
  store i32 0, ptr %11, align 4
  br label %BB_25

BB_6:                                             ; preds = %BB_8, %BB_3
  %12 = getelementptr i32, ptr %6, i32 0
  %13 = load i32, ptr %12, align 4
  %14 = icmp slt i32 %13, 50
  %15 = zext i1 %14 to i32
  %16 = icmp ne i32 %15, 0
  br i1 %16, label %BB_7, label %BB_9

BB_7:                                             ; preds = %BB_6
  %17 = getelementptr i32, ptr %6, i32 0
  %18 = load i32, ptr %17, align 4
  %19 = getelementptr i32, ptr %0, i32 0
  %20 = load i32, ptr %19, align 4
  %21 = mul i32 %20, 50
  %22 = add i32 %18, %21
  %23 = alloca i32, align 4
  store i32 %22, ptr %23, align 4
  %24 = getelementptr i32, ptr %6, i32 0
  %25 = load i32, ptr %24, align 4
  %26 = sub i32 %25, 1
  %27 = add i32 %26, 50
  %28 = srem i32 %27, 50
  %29 = alloca i32, align 4
  store i32 %28, ptr %29, align 4
  %30 = getelementptr i32, ptr %6, i32 0
  %31 = load i32, ptr %30, align 4
  %32 = add i32 %31, 0
  %33 = srem i32 %32, 50
  %34 = alloca i32, align 4
  store i32 %33, ptr %34, align 4
  %35 = getelementptr i32, ptr %6, i32 0
  %36 = load i32, ptr %35, align 4
  %37 = add i32 %36, 1
  %38 = srem i32 %37, 50
  %39 = alloca i32, align 4
  store i32 %38, ptr %39, align 4
  %40 = getelementptr i32, ptr %0, i32 0
  %41 = load i32, ptr %40, align 4
  %42 = sub i32 %41, 1
  %43 = add i32 %42, 50
  %44 = srem i32 %43, 50
  %45 = alloca i32, align 4
  store i32 %44, ptr %45, align 4
  %46 = getelementptr i32, ptr %0, i32 0
  %47 = load i32, ptr %46, align 4
  %48 = add i32 %47, 0
  %49 = srem i32 %48, 50
  %50 = alloca i32, align 4
  store i32 %49, ptr %50, align 4
  %51 = getelementptr i32, ptr %0, i32 0
  %52 = load i32, ptr %51, align 4
  %53 = add i32 %52, 1
  %54 = srem i32 %53, 50
  %55 = alloca i32, align 4
  store i32 %54, ptr %55, align 4
  %56 = alloca [8 x i32], align 4
  %57 = getelementptr [8 x i32], ptr %56, i32 0, i32 0
  %58 = getelementptr i32, ptr %29, i32 0
  %59 = load i32, ptr %58, align 4
  %60 = getelementptr i32, ptr %45, i32 0
  %61 = load i32, ptr %60, align 4
  %62 = mul i32 %61, 50
  %63 = add i32 %59, %62
  %64 = mul i32 2, %63
  %65 = getelementptr [5000 x i32], ptr @allCells, i32 0, i32 %64
  %66 = load i32, ptr %65, align 4
  store i32 %66, ptr %57, align 4
  %67 = getelementptr [8 x i32], ptr %56, i32 0, i32 1
  %68 = getelementptr i32, ptr %29, i32 0
  %69 = load i32, ptr %68, align 4
  %70 = getelementptr i32, ptr %50, i32 0
  %71 = load i32, ptr %70, align 4
  %72 = mul i32 %71, 50
  %73 = add i32 %69, %72
  %74 = mul i32 2, %73
  %75 = getelementptr [5000 x i32], ptr @allCells, i32 0, i32 %74
  %76 = load i32, ptr %75, align 4
  store i32 %76, ptr %67, align 4
  %77 = getelementptr [8 x i32], ptr %56, i32 0, i32 2
  %78 = getelementptr i32, ptr %29, i32 0
  %79 = load i32, ptr %78, align 4
  %80 = getelementptr i32, ptr %55, i32 0
  %81 = load i32, ptr %80, align 4
  %82 = mul i32 %81, 50
  %83 = add i32 %79, %82
  %84 = mul i32 2, %83
  %85 = getelementptr [5000 x i32], ptr @allCells, i32 0, i32 %84
  %86 = load i32, ptr %85, align 4
  store i32 %86, ptr %77, align 4
  %87 = getelementptr [8 x i32], ptr %56, i32 0, i32 3
  %88 = getelementptr i32, ptr %34, i32 0
  %89 = load i32, ptr %88, align 4
  %90 = getelementptr i32, ptr %45, i32 0
  %91 = load i32, ptr %90, align 4
  %92 = mul i32 %91, 50
  %93 = add i32 %89, %92
  %94 = mul i32 2, %93
  %95 = getelementptr [5000 x i32], ptr @allCells, i32 0, i32 %94
  %96 = load i32, ptr %95, align 4
  store i32 %96, ptr %87, align 4
  %97 = getelementptr [8 x i32], ptr %56, i32 0, i32 4
  %98 = getelementptr i32, ptr %34, i32 0
  %99 = load i32, ptr %98, align 4
  %100 = getelementptr i32, ptr %55, i32 0
  %101 = load i32, ptr %100, align 4
  %102 = mul i32 %101, 50
  %103 = add i32 %99, %102
  %104 = mul i32 2, %103
  %105 = getelementptr [5000 x i32], ptr @allCells, i32 0, i32 %104
  %106 = load i32, ptr %105, align 4
  store i32 %106, ptr %97, align 4
  %107 = getelementptr [8 x i32], ptr %56, i32 0, i32 5
  %108 = getelementptr i32, ptr %39, i32 0
  %109 = load i32, ptr %108, align 4
  %110 = getelementptr i32, ptr %45, i32 0
  %111 = load i32, ptr %110, align 4
  %112 = mul i32 %111, 50
  %113 = add i32 %109, %112
  %114 = mul i32 2, %113
  %115 = getelementptr [5000 x i32], ptr @allCells, i32 0, i32 %114
  %116 = load i32, ptr %115, align 4
  store i32 %116, ptr %107, align 4
  %117 = getelementptr [8 x i32], ptr %56, i32 0, i32 6
  %118 = getelementptr i32, ptr %39, i32 0
  %119 = load i32, ptr %118, align 4
  %120 = getelementptr i32, ptr %50, i32 0
  %121 = load i32, ptr %120, align 4
  %122 = mul i32 %121, 50
  %123 = add i32 %119, %122
  %124 = mul i32 2, %123
  %125 = getelementptr [5000 x i32], ptr @allCells, i32 0, i32 %124
  %126 = load i32, ptr %125, align 4
  store i32 %126, ptr %117, align 4
  %127 = getelementptr [8 x i32], ptr %56, i32 0, i32 7
  %128 = getelementptr i32, ptr %39, i32 0
  %129 = load i32, ptr %128, align 4
  %130 = getelementptr i32, ptr %55, i32 0
  %131 = load i32, ptr %130, align 4
  %132 = mul i32 %131, 50
  %133 = add i32 %129, %132
  %134 = mul i32 2, %133
  %135 = getelementptr [5000 x i32], ptr @allCells, i32 0, i32 %134
  %136 = load i32, ptr %135, align 4
  store i32 %136, ptr %127, align 4
  %137 = alloca i32, align 4
  store i32 0, ptr %137, align 4
  %138 = alloca i32, align 4
  store i32 0, ptr %138, align 4
  br label %BB_10

BB_8:                                             ; preds = %BB_21
  %139 = getelementptr i32, ptr %6, i32 0
  %140 = getelementptr i32, ptr %6, i32 0
  %141 = load i32, ptr %140, align 4
  %142 = add i32 %141, 1
  store i32 %142, ptr %139, align 4
  br label %BB_6

BB_9:                                             ; preds = %BB_6
  br label %BB_4

BB_10:                                            ; preds = %BB_12, %BB_7
  %143 = getelementptr i32, ptr %138, i32 0
  %144 = load i32, ptr %143, align 4
  %145 = icmp slt i32 %144, 8
  %146 = zext i1 %145 to i32
  %147 = icmp ne i32 %146, 0
  br i1 %147, label %BB_11, label %BB_13

BB_11:                                            ; preds = %BB_10
  %148 = getelementptr i32, ptr %138, i32 0
  %149 = load i32, ptr %148, align 4
  %150 = getelementptr [8 x i32], ptr %56, i32 0, i32 %149
  %151 = load i32, ptr %150, align 4
  %152 = icmp eq i32 %151, 1
  %153 = zext i1 %152 to i32
  %154 = icmp ne i32 %153, 0
  br i1 %154, label %BB_14, label %BB_15

BB_12:                                            ; preds = %BB_15
  %155 = getelementptr i32, ptr %138, i32 0
  %156 = getelementptr i32, ptr %138, i32 0
  %157 = load i32, ptr %156, align 4
  %158 = add i32 %157, 1
  store i32 %158, ptr %155, align 4
  br label %BB_10

BB_13:                                            ; preds = %BB_10
  %159 = getelementptr i32, ptr %23, i32 0
  %160 = load i32, ptr %159, align 4
  %161 = mul i32 2, %160
  %162 = getelementptr [5000 x i32], ptr @allCells, i32 0, i32 %161
  %163 = load i32, ptr %162, align 4
  %164 = icmp eq i32 %163, 1
  %165 = zext i1 %164 to i32
  %166 = icmp ne i32 %165, 0
  br i1 %166, label %BB_16, label %BB_17

BB_14:                                            ; preds = %BB_11
  %167 = getelementptr i32, ptr %137, i32 0
  %168 = getelementptr i32, ptr %137, i32 0
  %169 = load i32, ptr %168, align 4
  %170 = add i32 %169, 1
  store i32 %170, ptr %167, align 4
  br label %BB_15

BB_15:                                            ; preds = %BB_14, %BB_11
  br label %BB_12

BB_16:                                            ; preds = %BB_13
  %171 = getelementptr i32, ptr %137, i32 0
  %172 = load i32, ptr %171, align 4
  %173 = icmp eq i32 %172, 0
  %174 = zext i1 %173 to i32
  %175 = getelementptr i32, ptr %137, i32 0
  %176 = load i32, ptr %175, align 4
  %177 = icmp eq i32 %176, 3
  %178 = zext i1 %177 to i32
  %179 = getelementptr i32, ptr %137, i32 0
  %180 = load i32, ptr %179, align 4
  %181 = icmp eq i32 %180, 4
  %182 = zext i1 %181 to i32
  %183 = getelementptr i32, ptr %137, i32 0
  %184 = load i32, ptr %183, align 4
  %185 = icmp eq i32 %184, 5
  %186 = zext i1 %185 to i32
  %187 = icmp sgt i32 %182, 0
  %188 = zext i1 %187 to i32
  %189 = icmp sgt i32 %186, 0
  %190 = zext i1 %189 to i32
  %191 = or i32 %188, %190
  %192 = icmp sgt i32 %178, 0
  %193 = zext i1 %192 to i32
  %194 = icmp sgt i32 %191, 0
  %195 = zext i1 %194 to i32
  %196 = or i32 %193, %195
  %197 = icmp sgt i32 %174, 0
  %198 = zext i1 %197 to i32
  %199 = icmp sgt i32 %196, 0
  %200 = zext i1 %199 to i32
  %201 = or i32 %198, %200
  %202 = icmp ne i32 %201, 0
  br i1 %202, label %BB_18, label %BB_19

BB_17:                                            ; preds = %BB_13
  %203 = getelementptr i32, ptr %137, i32 0
  %204 = load i32, ptr %203, align 4
  %205 = icmp eq i32 %204, 2
  %206 = zext i1 %205 to i32
  %207 = icmp ne i32 %206, 0
  br i1 %207, label %BB_22, label %BB_23

BB_18:                                            ; preds = %BB_16
  %208 = getelementptr i32, ptr %23, i32 0
  %209 = load i32, ptr %208, align 4
  %210 = mul i32 2, %209
  %211 = getelementptr [5000 x i32], ptr @swapCells, i32 0, i32 %210
  store i32 1, ptr %211, align 4
  %212 = getelementptr i32, ptr %23, i32 0
  %213 = load i32, ptr %212, align 4
  %214 = mul i32 2, %213
  %215 = add i32 %214, 1
  %216 = getelementptr [5000 x i32], ptr @swapCells, i32 0, i32 %215
  %217 = getelementptr i32, ptr %23, i32 0
  %218 = load i32, ptr %217, align 4
  %219 = mul i32 2, %218
  %220 = add i32 %219, 1
  %221 = getelementptr [5000 x i32], ptr @allCells, i32 0, i32 %220
  %222 = load i32, ptr %221, align 4
  %223 = add i32 %222, 1
  store i32 %223, ptr %216, align 4
  br label %BB_20

BB_19:                                            ; preds = %BB_16
  %224 = getelementptr i32, ptr %23, i32 0
  %225 = load i32, ptr %224, align 4
  %226 = mul i32 2, %225
  %227 = getelementptr [5000 x i32], ptr @swapCells, i32 0, i32 %226
  store i32 0, ptr %227, align 4
  %228 = getelementptr i32, ptr %23, i32 0
  %229 = load i32, ptr %228, align 4
  %230 = mul i32 2, %229
  %231 = add i32 %230, 1
  %232 = getelementptr [5000 x i32], ptr @swapCells, i32 0, i32 %231
  store i32 0, ptr %232, align 4
  br label %BB_20

BB_20:                                            ; preds = %BB_19, %BB_18
  br label %BB_21

BB_21:                                            ; preds = %BB_24, %BB_20
  br label %BB_8

BB_22:                                            ; preds = %BB_17
  %233 = getelementptr i32, ptr %23, i32 0
  %234 = load i32, ptr %233, align 4
  %235 = mul i32 2, %234
  %236 = getelementptr [5000 x i32], ptr @swapCells, i32 0, i32 %235
  store i32 1, ptr %236, align 4
  %237 = getelementptr i32, ptr %23, i32 0
  %238 = load i32, ptr %237, align 4
  %239 = mul i32 2, %238
  %240 = add i32 %239, 1
  %241 = getelementptr [5000 x i32], ptr @swapCells, i32 0, i32 %240
  store i32 1, ptr %241, align 4
  br label %BB_24

BB_23:                                            ; preds = %BB_17
  %242 = getelementptr i32, ptr %23, i32 0
  %243 = load i32, ptr %242, align 4
  %244 = mul i32 2, %243
  %245 = getelementptr [5000 x i32], ptr @swapCells, i32 0, i32 %244
  store i32 0, ptr %245, align 4
  %246 = getelementptr i32, ptr %23, i32 0
  %247 = load i32, ptr %246, align 4
  %248 = mul i32 2, %247
  %249 = add i32 %248, 1
  %250 = getelementptr [5000 x i32], ptr @swapCells, i32 0, i32 %249
  store i32 0, ptr %250, align 4
  br label %BB_24

BB_24:                                            ; preds = %BB_23, %BB_22
  br label %BB_21

BB_25:                                            ; preds = %BB_27, %BB_5
  %251 = getelementptr i32, ptr %11, i32 0
  %252 = load i32, ptr %251, align 4
  %253 = icmp slt i32 %252, 50
  %254 = zext i1 %253 to i32
  %255 = icmp ne i32 %254, 0
  br i1 %255, label %BB_26, label %BB_28

BB_26:                                            ; preds = %BB_25
  %256 = alloca i32, align 4
  store i32 0, ptr %256, align 4
  br label %BB_29

BB_27:                                            ; preds = %BB_32
  %257 = getelementptr i32, ptr %11, i32 0
  %258 = getelementptr i32, ptr %11, i32 0
  %259 = load i32, ptr %258, align 4
  %260 = add i32 %259, 1
  store i32 %260, ptr %257, align 4
  br label %BB_25

BB_28:                                            ; preds = %BB_25
  ret void

BB_29:                                            ; preds = %BB_31, %BB_26
  %261 = getelementptr i32, ptr %256, i32 0
  %262 = load i32, ptr %261, align 4
  %263 = icmp slt i32 %262, 50
  %264 = zext i1 %263 to i32
  %265 = icmp ne i32 %264, 0
  br i1 %265, label %BB_30, label %BB_32

BB_30:                                            ; preds = %BB_29
  %266 = getelementptr i32, ptr %256, i32 0
  %267 = load i32, ptr %266, align 4
  %268 = getelementptr i32, ptr %11, i32 0
  %269 = load i32, ptr %268, align 4
  %270 = mul i32 %269, 50
  %271 = add i32 %267, %270
  %272 = alloca i32, align 4
  store i32 %271, ptr %272, align 4
  %273 = getelementptr i32, ptr %272, i32 0
  %274 = load i32, ptr %273, align 4
  %275 = mul i32 2, %274
  %276 = getelementptr [5000 x i32], ptr @allCells, i32 0, i32 %275
  %277 = getelementptr i32, ptr %272, i32 0
  %278 = load i32, ptr %277, align 4
  %279 = mul i32 2, %278
  %280 = getelementptr [5000 x i32], ptr @swapCells, i32 0, i32 %279
  %281 = load i32, ptr %280, align 4
  store i32 %281, ptr %276, align 4
  %282 = getelementptr i32, ptr %272, i32 0
  %283 = load i32, ptr %282, align 4
  %284 = mul i32 2, %283
  %285 = add i32 %284, 1
  %286 = getelementptr [5000 x i32], ptr @allCells, i32 0, i32 %285
  %287 = getelementptr i32, ptr %272, i32 0
  %288 = load i32, ptr %287, align 4
  %289 = mul i32 2, %288
  %290 = add i32 %289, 1
  %291 = getelementptr [5000 x i32], ptr @swapCells, i32 0, i32 %290
  %292 = load i32, ptr %291, align 4
  store i32 %292, ptr %286, align 4
  %293 = alloca i32, align 4
  store i32 0, ptr %293, align 4
  %294 = getelementptr i32, ptr %272, i32 0
  %295 = load i32, ptr %294, align 4
  %296 = mul i32 2, %295
  %297 = add i32 %296, 1
  %298 = getelementptr [5000 x i32], ptr @allCells, i32 0, i32 %297
  %299 = load i32, ptr %298, align 4
  %300 = icmp eq i32 %299, 10
  %301 = zext i1 %300 to i32
  %302 = icmp ne i32 %301, 0
  br i1 %302, label %BB_33, label %BB_34

BB_31:                                            ; preds = %BB_36
  %303 = getelementptr i32, ptr %256, i32 0
  %304 = getelementptr i32, ptr %256, i32 0
  %305 = load i32, ptr %304, align 4
  %306 = add i32 %305, 1
  store i32 %306, ptr %303, align 4
  br label %BB_29

BB_32:                                            ; preds = %BB_29
  br label %BB_27

BB_33:                                            ; preds = %BB_30
  %307 = getelementptr i32, ptr %272, i32 0
  %308 = load i32, ptr %307, align 4
  %309 = mul i32 2, %308
  %310 = getelementptr [5000 x i32], ptr @allCells, i32 0, i32 %309
  store i32 0, ptr %310, align 4
  %311 = getelementptr i32, ptr %272, i32 0
  %312 = load i32, ptr %311, align 4
  %313 = mul i32 2, %312
  %314 = add i32 %313, 1
  %315 = getelementptr [5000 x i32], ptr @allCells, i32 0, i32 %314
  store i32 0, ptr %315, align 4
  br label %BB_34

BB_34:                                            ; preds = %BB_33, %BB_30
  %316 = getelementptr i32, ptr %272, i32 0
  %317 = load i32, ptr %316, align 4
  %318 = mul i32 2, %317
  %319 = getelementptr [5000 x i32], ptr @allCells, i32 0, i32 %318
  %320 = load i32, ptr %319, align 4
  %321 = icmp eq i32 %320, 1
  %322 = zext i1 %321 to i32
  %323 = icmp ne i32 %322, 0
  br i1 %323, label %BB_35, label %BB_36

BB_35:                                            ; preds = %BB_34
  %324 = getelementptr i32, ptr %272, i32 0
  %325 = load i32, ptr %324, align 4
  %326 = mul i32 2, %325
  %327 = add i32 %326, 1
  %328 = getelementptr [5000 x i32], ptr @allCells, i32 0, i32 %327
  %329 = load i32, ptr %328, align 4
  %330 = alloca i32, align 4
  store i32 %329, ptr %330, align 4
  %331 = alloca i32, align 4
  store i32 0, ptr %331, align 4
  %332 = alloca i32, align 4
  store i32 0, ptr %332, align 4
  %333 = alloca i32, align 4
  store i32 0, ptr %333, align 4
  %334 = getelementptr i32, ptr %330, i32 0
  %335 = load i32, ptr %334, align 4
  %336 = icmp sgt i32 %335, 6
  %337 = zext i1 %336 to i32
  %338 = icmp ne i32 %337, 0
  br i1 %338, label %BB_37, label %BB_38

BB_36:                                            ; preds = %BB_42, %BB_34
  %339 = getelementptr i32, ptr %256, i32 0
  %340 = load i32, ptr %339, align 4
  %341 = getelementptr i32, ptr %11, i32 0
  %342 = load i32, ptr %341, align 4
  %343 = getelementptr i32, ptr %293, i32 0
  %344 = load i32, ptr %343, align 4
  call void @paintCellPixels(i32 %340, i32 %342, i32 %344)
  br label %BB_31

BB_37:                                            ; preds = %BB_35
  %345 = getelementptr i32, ptr %331, i32 0
  %346 = getelementptr i32, ptr %330, i32 0
  %347 = load i32, ptr %346, align 4
  %348 = sub i32 10, %347
  %349 = mul i32 2, %348
  %350 = sub i32 127, %349
  store i32 %350, ptr %345, align 4
  br label %BB_38

BB_38:                                            ; preds = %BB_37, %BB_35
  %351 = getelementptr i32, ptr %330, i32 0
  %352 = load i32, ptr %351, align 4
  %353 = icmp slt i32 %352, 5
  %354 = zext i1 %353 to i32
  %355 = icmp ne i32 %354, 0
  br i1 %355, label %BB_39, label %BB_40

BB_39:                                            ; preds = %BB_38
  %356 = getelementptr i32, ptr %332, i32 0
  %357 = getelementptr i32, ptr %272, i32 0
  %358 = load i32, ptr %357, align 4
  %359 = mul i32 2, %358
  %360 = add i32 %359, 1
  %361 = getelementptr [5000 x i32], ptr @allCells, i32 0, i32 %360
  %362 = load i32, ptr %361, align 4
  %363 = sdiv i32 255, %362
  store i32 %363, ptr %356, align 4
  br label %BB_40

BB_40:                                            ; preds = %BB_39, %BB_38
  %364 = getelementptr i32, ptr %330, i32 0
  %365 = load i32, ptr %364, align 4
  %366 = icmp sge i32 %365, 5
  %367 = zext i1 %366 to i32
  %368 = getelementptr i32, ptr %330, i32 0
  %369 = load i32, ptr %368, align 4
  %370 = icmp sle i32 %369, 6
  %371 = zext i1 %370 to i32
  %372 = icmp sgt i32 %367, 0
  %373 = zext i1 %372 to i32
  %374 = icmp sgt i32 %371, 0
  %375 = zext i1 %374 to i32
  %376 = and i32 %373, %375
  %377 = icmp ne i32 %376, 0
  br i1 %377, label %BB_41, label %BB_42

BB_41:                                            ; preds = %BB_40
  %378 = getelementptr i32, ptr %333, i32 0
  store i32 127, ptr %378, align 4
  br label %BB_42

BB_42:                                            ; preds = %BB_41, %BB_40
  %379 = getelementptr i32, ptr %293, i32 0
  %380 = getelementptr i32, ptr %331, i32 0
  %381 = load i32, ptr %380, align 4
  %382 = getelementptr i32, ptr %332, i32 0
  %383 = load i32, ptr %382, align 4
  %384 = getelementptr i32, ptr %333, i32 0
  %385 = load i32, ptr %384, align 4
  %386 = call i32 @makeColor(i32 %381, i32 %383, i32 %385)
  store i32 %386, ptr %379, align 4
  br label %BB_36
}

define void @app() {
BB_1:
  call void @initCells()
  call void @llvm.riscx.flush()
  br label %BB_2

BB_2:                                             ; preds = %BB_3, %BB_1
  br i1 true, label %BB_3, label %BB_4

BB_3:                                             ; preds = %BB_2
  call void @updatePixels()
  call void @llvm.riscx.flush()
  br label %BB_2

BB_4:                                             ; preds = %BB_2
  ret void
}
