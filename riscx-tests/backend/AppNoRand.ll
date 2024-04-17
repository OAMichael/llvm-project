; ModuleID = 'top'
source_filename = "top"

@allCells = internal global [5000 x i32] zeroinitializer, align 16
@swapCells = internal global [5000 x i32] zeroinitializer, align 16

declare void @simPutPixel(i32, i32, i32)

declare void @simFlush()

declare i32 @simRand()

define i32 @makeColor(i32 %0, i32 %1, i32 %2) {
BB_1:
  %3 = alloca i32
  store i32 %0, i32* %3
  %4 = alloca i32
  store i32 %1, i32* %4
  %5 = alloca i32
  store i32 %2, i32* %5
  %6 = alloca i32
  store i32 255, i32* %6
  %7 = alloca i32
  store i32 0, i32* %7
  %8 = getelementptr i32, i32* %7, i32 0
  %9 = getelementptr i32, i32* %7, i32 0
  %10 = load i32, i32* %9
  %11 = getelementptr i32, i32* %6, i32 0
  %12 = load i32, i32* %11
  %13 = and i32 %12, 255
  %14 = shl i32 %13, 24
  %15 = or i32 %10, %14
  store i32 %15, i32* %8
  %16 = getelementptr i32, i32* %7, i32 0
  %17 = getelementptr i32, i32* %7, i32 0
  %18 = load i32, i32* %17
  %19 = getelementptr i32, i32* %3, i32 0
  %20 = load i32, i32* %19
  %21 = and i32 %20, 255
  %22 = shl i32 %21, 16
  %23 = or i32 %18, %22
  store i32 %23, i32* %16
  %24 = getelementptr i32, i32* %7, i32 0
  %25 = getelementptr i32, i32* %7, i32 0
  %26 = load i32, i32* %25
  %27 = getelementptr i32, i32* %4, i32 0
  %28 = load i32, i32* %27
  %29 = and i32 %28, 255
  %30 = shl i32 %29, 8
  %31 = or i32 %26, %30
  store i32 %31, i32* %24
  %32 = getelementptr i32, i32* %7, i32 0
  %33 = getelementptr i32, i32* %7, i32 0
  %34 = load i32, i32* %33
  %35 = getelementptr i32, i32* %5, i32 0
  %36 = load i32, i32* %35
  %37 = and i32 %36, 255
  %38 = shl i32 %37, 0
  %39 = or i32 %34, %38
  store i32 %39, i32* %32
  %40 = getelementptr i32, i32* %7, i32 0
  %41 = load i32, i32* %40
  ret i32 %41
}

define void @paintCellPixels(i32 %0, i32 %1, i32 %2) {
BB_1:
  %3 = alloca i32
  store i32 %0, i32* %3
  %4 = alloca i32
  store i32 %1, i32* %4
  %5 = alloca i32
  store i32 %2, i32* %5
  %6 = getelementptr i32, i32* %3, i32 0
  %7 = load i32, i32* %6
  %8 = mul i32 %7, 16
  %9 = add i32 %8, 1
  %10 = alloca i32
  store i32 %9, i32* %10
  %11 = getelementptr i32, i32* %4, i32 0
  %12 = load i32, i32* %11
  %13 = mul i32 %12, 16
  %14 = add i32 %13, 1
  %15 = alloca i32
  store i32 %14, i32* %15
  %16 = getelementptr i32, i32* %3, i32 0
  %17 = load i32, i32* %16
  %18 = mul i32 %17, 16
  %19 = add i32 %18, 16
  %20 = sub i32 %19, 1
  %21 = alloca i32
  store i32 %20, i32* %21
  %22 = getelementptr i32, i32* %4, i32 0
  %23 = load i32, i32* %22
  %24 = mul i32 %23, 16
  %25 = add i32 %24, 16
  %26 = sub i32 %25, 1
  %27 = alloca i32
  store i32 %26, i32* %27
  %28 = getelementptr i32, i32* %15, i32 0
  %29 = load i32, i32* %28
  %30 = alloca i32
  store i32 %29, i32* %30
  br label %BB_2

BB_2:                                             ; preds = %BB_4, %BB_1
  %31 = getelementptr i32, i32* %30, i32 0
  %32 = load i32, i32* %31
  %33 = getelementptr i32, i32* %27, i32 0
  %34 = load i32, i32* %33
  %35 = icmp slt i32 %32, %34
  %36 = zext i1 %35 to i32
  %37 = icmp ne i32 %36, 0
  br i1 %37, label %BB_3, label %BB_5

BB_3:                                             ; preds = %BB_2
  %38 = getelementptr i32, i32* %10, i32 0
  %39 = load i32, i32* %38
  %40 = alloca i32
  store i32 %39, i32* %40
  br label %BB_6

BB_4:                                             ; preds = %BB_9
  %41 = getelementptr i32, i32* %30, i32 0
  %42 = getelementptr i32, i32* %30, i32 0
  %43 = load i32, i32* %42
  %44 = add i32 %43, 1
  store i32 %44, i32* %41
  br label %BB_2

BB_5:                                             ; preds = %BB_2
  ret void

BB_6:                                             ; preds = %BB_8, %BB_3
  %45 = getelementptr i32, i32* %40, i32 0
  %46 = load i32, i32* %45
  %47 = getelementptr i32, i32* %21, i32 0
  %48 = load i32, i32* %47
  %49 = icmp slt i32 %46, %48
  %50 = zext i1 %49 to i32
  %51 = icmp ne i32 %50, 0
  br i1 %51, label %BB_7, label %BB_9

BB_7:                                             ; preds = %BB_6
  %52 = getelementptr i32, i32* %40, i32 0
  %53 = load i32, i32* %52
  %54 = getelementptr i32, i32* %30, i32 0
  %55 = load i32, i32* %54
  %56 = getelementptr i32, i32* %5, i32 0
  %57 = load i32, i32* %56
  call void @simPutPixel(i32 %53, i32 %55, i32 %57)
  br label %BB_8

BB_8:                                             ; preds = %BB_7
  %58 = getelementptr i32, i32* %40, i32 0
  %59 = getelementptr i32, i32* %40, i32 0
  %60 = load i32, i32* %59
  %61 = add i32 %60, 1
  store i32 %61, i32* %58
  br label %BB_6

BB_9:                                             ; preds = %BB_6
  br label %BB_4
}

define void @initCells() {
BB_1:
  %0 = alloca i32
  store i32 0, i32* %0
  %1 = alloca i32
  store i32 0, i32* %1
  %2 = getelementptr i32, i32* %1, i32 0
  store i32 0, i32* %2
  br label %BB_2

BB_2:                                             ; preds = %BB_4, %BB_1
  %3 = getelementptr i32, i32* %1, i32 0
  %4 = load i32, i32* %3
  %5 = icmp slt i32 %4, 50
  %6 = zext i1 %5 to i32
  %7 = icmp ne i32 %6, 0
  br i1 %7, label %BB_3, label %BB_5

BB_3:                                             ; preds = %BB_2
  %8 = getelementptr i32, i32* %0, i32 0
  store i32 0, i32* %8
  br label %BB_6

BB_4:                                             ; preds = %BB_9
  %9 = getelementptr i32, i32* %1, i32 0
  %10 = getelementptr i32, i32* %1, i32 0
  %11 = load i32, i32* %10
  %12 = add i32 %11, 1
  store i32 %12, i32* %9
  br label %BB_2

BB_5:                                             ; preds = %BB_2
  ret void

BB_6:                                             ; preds = %BB_8, %BB_3
  %13 = getelementptr i32, i32* %0, i32 0
  %14 = load i32, i32* %13
  %15 = icmp slt i32 %14, 50
  %16 = zext i1 %15 to i32
  %17 = icmp ne i32 %16, 0
  br i1 %17, label %BB_7, label %BB_9

BB_7:                                             ; preds = %BB_6
  %18 = call i32 @simRand()
  %19 = alloca i32
  store i32 %18, i32* %19
  %20 = getelementptr i32, i32* %19, i32 0
  %21 = load i32, i32* %20
  %22 = srem i32 %21, 31
  %23 = icmp eq i32 %22, 0
  %24 = zext i1 %23 to i32
  %25 = icmp ne i32 %24, 0
  br i1 %25, label %BB_10, label %BB_11

BB_8:                                             ; preds = %BB_11
  %26 = getelementptr i32, i32* %0, i32 0
  %27 = getelementptr i32, i32* %0, i32 0
  %28 = load i32, i32* %27
  %29 = add i32 %28, 1
  store i32 %29, i32* %26
  br label %BB_6

BB_9:                                             ; preds = %BB_6
  br label %BB_4

BB_10:                                            ; preds = %BB_7
  %30 = getelementptr i32, i32* %0, i32 0
  %31 = load i32, i32* %30
  %32 = getelementptr i32, i32* %1, i32 0
  %33 = load i32, i32* %32
  %34 = mul i32 %33, 50
  %35 = add i32 %31, %34
  %36 = mul i32 2, %35
  %37 = getelementptr [5000 x i32], [5000 x i32]* @allCells, i32 0, i32 %36
  store i32 1, i32* %37
  %38 = getelementptr i32, i32* %0, i32 0
  %39 = load i32, i32* %38
  %40 = getelementptr i32, i32* %1, i32 0
  %41 = load i32, i32* %40
  %42 = mul i32 %41, 50
  %43 = add i32 %39, %42
  %44 = mul i32 2, %43
  %45 = add i32 %44, 1
  %46 = getelementptr [5000 x i32], [5000 x i32]* @allCells, i32 0, i32 %45
  store i32 1, i32* %46
  %47 = call i32 @makeColor(i32 0, i32 255, i32 0)
  %48 = alloca i32
  store i32 %47, i32* %48
  %49 = getelementptr i32, i32* %0, i32 0
  %50 = load i32, i32* %49
  %51 = getelementptr i32, i32* %1, i32 0
  %52 = load i32, i32* %51
  %53 = getelementptr i32, i32* %48, i32 0
  %54 = load i32, i32* %53
  call void @paintCellPixels(i32 %50, i32 %52, i32 %54)
  br label %BB_11

BB_11:                                            ; preds = %BB_10, %BB_7
  br label %BB_8
}

define void @updatePixels() {
BB_1:
  %0 = alloca i32
  store i32 0, i32* %0
  %1 = alloca i32
  store i32 0, i32* %1
  %2 = alloca i32
  store i32 0, i32* %2
  %3 = alloca i32
  store i32 0, i32* %3
  %4 = alloca i32
  store i32 0, i32* %4
  %5 = alloca i32
  store i32 0, i32* %5
  %6 = alloca i32
  store i32 0, i32* %6
  %7 = alloca i32
  store i32 0, i32* %7
  %8 = alloca i32
  store i32 0, i32* %8
  %9 = alloca [8 x i32]
  %10 = alloca i32
  store i32 0, i32* %10
  br label %BB_2

BB_2:                                             ; preds = %BB_4, %BB_1
  %11 = getelementptr i32, i32* %10, i32 0
  %12 = load i32, i32* %11
  %13 = icmp slt i32 %12, 50
  %14 = zext i1 %13 to i32
  %15 = icmp ne i32 %14, 0
  br i1 %15, label %BB_3, label %BB_5

BB_3:                                             ; preds = %BB_2
  %16 = alloca i32
  store i32 0, i32* %16
  br label %BB_6

BB_4:                                             ; preds = %BB_9
  %17 = getelementptr i32, i32* %10, i32 0
  %18 = getelementptr i32, i32* %10, i32 0
  %19 = load i32, i32* %18
  %20 = add i32 %19, 1
  store i32 %20, i32* %17
  br label %BB_2

BB_5:                                             ; preds = %BB_2
  %21 = alloca i32
  store i32 0, i32* %21
  br label %BB_25

BB_6:                                             ; preds = %BB_8, %BB_3
  %22 = getelementptr i32, i32* %16, i32 0
  %23 = load i32, i32* %22
  %24 = icmp slt i32 %23, 50
  %25 = zext i1 %24 to i32
  %26 = icmp ne i32 %25, 0
  br i1 %26, label %BB_7, label %BB_9

BB_7:                                             ; preds = %BB_6
  %27 = getelementptr i32, i32* %1, i32 0
  %28 = getelementptr i32, i32* %16, i32 0
  %29 = load i32, i32* %28
  %30 = getelementptr i32, i32* %10, i32 0
  %31 = load i32, i32* %30
  %32 = mul i32 %31, 50
  %33 = add i32 %29, %32
  store i32 %33, i32* %27
  %34 = getelementptr i32, i32* %2, i32 0
  %35 = getelementptr i32, i32* %16, i32 0
  %36 = load i32, i32* %35
  %37 = sub i32 %36, 1
  %38 = add i32 %37, 50
  %39 = srem i32 %38, 50
  store i32 %39, i32* %34
  %40 = getelementptr i32, i32* %3, i32 0
  %41 = getelementptr i32, i32* %16, i32 0
  %42 = load i32, i32* %41
  %43 = add i32 %42, 0
  %44 = srem i32 %43, 50
  store i32 %44, i32* %40
  %45 = getelementptr i32, i32* %4, i32 0
  %46 = getelementptr i32, i32* %16, i32 0
  %47 = load i32, i32* %46
  %48 = add i32 %47, 1
  %49 = srem i32 %48, 50
  store i32 %49, i32* %45
  %50 = getelementptr i32, i32* %5, i32 0
  %51 = getelementptr i32, i32* %10, i32 0
  %52 = load i32, i32* %51
  %53 = sub i32 %52, 1
  %54 = add i32 %53, 50
  %55 = srem i32 %54, 50
  store i32 %55, i32* %50
  %56 = getelementptr i32, i32* %6, i32 0
  %57 = getelementptr i32, i32* %10, i32 0
  %58 = load i32, i32* %57
  %59 = add i32 %58, 0
  %60 = srem i32 %59, 50
  store i32 %60, i32* %56
  %61 = getelementptr i32, i32* %7, i32 0
  %62 = getelementptr i32, i32* %10, i32 0
  %63 = load i32, i32* %62
  %64 = add i32 %63, 1
  %65 = srem i32 %64, 50
  store i32 %65, i32* %61
  %66 = getelementptr [8 x i32], [8 x i32]* %9, i32 0, i32 0
  %67 = getelementptr i32, i32* %2, i32 0
  %68 = load i32, i32* %67
  %69 = getelementptr i32, i32* %5, i32 0
  %70 = load i32, i32* %69
  %71 = mul i32 %70, 50
  %72 = add i32 %68, %71
  %73 = mul i32 2, %72
  %74 = getelementptr [5000 x i32], [5000 x i32]* @allCells, i32 0, i32 %73
  %75 = load i32, i32* %74
  store i32 %75, i32* %66
  %76 = getelementptr [8 x i32], [8 x i32]* %9, i32 0, i32 1
  %77 = getelementptr i32, i32* %2, i32 0
  %78 = load i32, i32* %77
  %79 = getelementptr i32, i32* %6, i32 0
  %80 = load i32, i32* %79
  %81 = mul i32 %80, 50
  %82 = add i32 %78, %81
  %83 = mul i32 2, %82
  %84 = getelementptr [5000 x i32], [5000 x i32]* @allCells, i32 0, i32 %83
  %85 = load i32, i32* %84
  store i32 %85, i32* %76
  %86 = getelementptr [8 x i32], [8 x i32]* %9, i32 0, i32 2
  %87 = getelementptr i32, i32* %2, i32 0
  %88 = load i32, i32* %87
  %89 = getelementptr i32, i32* %7, i32 0
  %90 = load i32, i32* %89
  %91 = mul i32 %90, 50
  %92 = add i32 %88, %91
  %93 = mul i32 2, %92
  %94 = getelementptr [5000 x i32], [5000 x i32]* @allCells, i32 0, i32 %93
  %95 = load i32, i32* %94
  store i32 %95, i32* %86
  %96 = getelementptr [8 x i32], [8 x i32]* %9, i32 0, i32 3
  %97 = getelementptr i32, i32* %3, i32 0
  %98 = load i32, i32* %97
  %99 = getelementptr i32, i32* %5, i32 0
  %100 = load i32, i32* %99
  %101 = mul i32 %100, 50
  %102 = add i32 %98, %101
  %103 = mul i32 2, %102
  %104 = getelementptr [5000 x i32], [5000 x i32]* @allCells, i32 0, i32 %103
  %105 = load i32, i32* %104
  store i32 %105, i32* %96
  %106 = getelementptr [8 x i32], [8 x i32]* %9, i32 0, i32 4
  %107 = getelementptr i32, i32* %3, i32 0
  %108 = load i32, i32* %107
  %109 = getelementptr i32, i32* %7, i32 0
  %110 = load i32, i32* %109
  %111 = mul i32 %110, 50
  %112 = add i32 %108, %111
  %113 = mul i32 2, %112
  %114 = getelementptr [5000 x i32], [5000 x i32]* @allCells, i32 0, i32 %113
  %115 = load i32, i32* %114
  store i32 %115, i32* %106
  %116 = getelementptr [8 x i32], [8 x i32]* %9, i32 0, i32 5
  %117 = getelementptr i32, i32* %4, i32 0
  %118 = load i32, i32* %117
  %119 = getelementptr i32, i32* %5, i32 0
  %120 = load i32, i32* %119
  %121 = mul i32 %120, 50
  %122 = add i32 %118, %121
  %123 = mul i32 2, %122
  %124 = getelementptr [5000 x i32], [5000 x i32]* @allCells, i32 0, i32 %123
  %125 = load i32, i32* %124
  store i32 %125, i32* %116
  %126 = getelementptr [8 x i32], [8 x i32]* %9, i32 0, i32 6
  %127 = getelementptr i32, i32* %4, i32 0
  %128 = load i32, i32* %127
  %129 = getelementptr i32, i32* %6, i32 0
  %130 = load i32, i32* %129
  %131 = mul i32 %130, 50
  %132 = add i32 %128, %131
  %133 = mul i32 2, %132
  %134 = getelementptr [5000 x i32], [5000 x i32]* @allCells, i32 0, i32 %133
  %135 = load i32, i32* %134
  store i32 %135, i32* %126
  %136 = getelementptr [8 x i32], [8 x i32]* %9, i32 0, i32 7
  %137 = getelementptr i32, i32* %4, i32 0
  %138 = load i32, i32* %137
  %139 = getelementptr i32, i32* %7, i32 0
  %140 = load i32, i32* %139
  %141 = mul i32 %140, 50
  %142 = add i32 %138, %141
  %143 = mul i32 2, %142
  %144 = getelementptr [5000 x i32], [5000 x i32]* @allCells, i32 0, i32 %143
  %145 = load i32, i32* %144
  store i32 %145, i32* %136
  %146 = getelementptr i32, i32* %8, i32 0
  store i32 0, i32* %146
  %147 = getelementptr i32, i32* %0, i32 0
  store i32 0, i32* %147
  br label %BB_10

BB_8:                                             ; preds = %BB_21
  %148 = getelementptr i32, i32* %16, i32 0
  %149 = getelementptr i32, i32* %16, i32 0
  %150 = load i32, i32* %149
  %151 = add i32 %150, 1
  store i32 %151, i32* %148
  br label %BB_6

BB_9:                                             ; preds = %BB_6
  br label %BB_4

BB_10:                                            ; preds = %BB_12, %BB_7
  %152 = getelementptr i32, i32* %0, i32 0
  %153 = load i32, i32* %152
  %154 = icmp slt i32 %153, 8
  %155 = zext i1 %154 to i32
  %156 = icmp ne i32 %155, 0
  br i1 %156, label %BB_11, label %BB_13

BB_11:                                            ; preds = %BB_10
  %157 = getelementptr i32, i32* %0, i32 0
  %158 = load i32, i32* %157
  %159 = getelementptr [8 x i32], [8 x i32]* %9, i32 0, i32 %158
  %160 = load i32, i32* %159
  %161 = icmp eq i32 %160, 1
  %162 = zext i1 %161 to i32
  %163 = icmp ne i32 %162, 0
  br i1 %163, label %BB_14, label %BB_15

BB_12:                                            ; preds = %BB_15
  %164 = getelementptr i32, i32* %0, i32 0
  %165 = getelementptr i32, i32* %0, i32 0
  %166 = load i32, i32* %165
  %167 = add i32 %166, 1
  store i32 %167, i32* %164
  br label %BB_10

BB_13:                                            ; preds = %BB_10
  %168 = getelementptr i32, i32* %1, i32 0
  %169 = load i32, i32* %168
  %170 = mul i32 2, %169
  %171 = getelementptr [5000 x i32], [5000 x i32]* @allCells, i32 0, i32 %170
  %172 = load i32, i32* %171
  %173 = icmp eq i32 %172, 1
  %174 = zext i1 %173 to i32
  %175 = icmp ne i32 %174, 0
  br i1 %175, label %BB_16, label %BB_17

BB_14:                                            ; preds = %BB_11
  %176 = getelementptr i32, i32* %8, i32 0
  %177 = getelementptr i32, i32* %8, i32 0
  %178 = load i32, i32* %177
  %179 = add i32 %178, 1
  store i32 %179, i32* %176
  br label %BB_15

BB_15:                                            ; preds = %BB_14, %BB_11
  br label %BB_12

BB_16:                                            ; preds = %BB_13
  %180 = getelementptr i32, i32* %8, i32 0
  %181 = load i32, i32* %180
  %182 = icmp eq i32 %181, 0
  %183 = zext i1 %182 to i32
  %184 = getelementptr i32, i32* %8, i32 0
  %185 = load i32, i32* %184
  %186 = icmp eq i32 %185, 3
  %187 = zext i1 %186 to i32
  %188 = getelementptr i32, i32* %8, i32 0
  %189 = load i32, i32* %188
  %190 = icmp eq i32 %189, 4
  %191 = zext i1 %190 to i32
  %192 = getelementptr i32, i32* %8, i32 0
  %193 = load i32, i32* %192
  %194 = icmp eq i32 %193, 5
  %195 = zext i1 %194 to i32
  %196 = icmp sgt i32 %191, 0
  %197 = zext i1 %196 to i32
  %198 = icmp sgt i32 %195, 0
  %199 = zext i1 %198 to i32
  %200 = or i32 %197, %199
  %201 = icmp sgt i32 %187, 0
  %202 = zext i1 %201 to i32
  %203 = icmp sgt i32 %200, 0
  %204 = zext i1 %203 to i32
  %205 = or i32 %202, %204
  %206 = icmp sgt i32 %183, 0
  %207 = zext i1 %206 to i32
  %208 = icmp sgt i32 %205, 0
  %209 = zext i1 %208 to i32
  %210 = or i32 %207, %209
  %211 = icmp ne i32 %210, 0
  br i1 %211, label %BB_18, label %BB_19

BB_17:                                            ; preds = %BB_13
  %212 = getelementptr i32, i32* %8, i32 0
  %213 = load i32, i32* %212
  %214 = icmp eq i32 %213, 2
  %215 = zext i1 %214 to i32
  %216 = icmp ne i32 %215, 0
  br i1 %216, label %BB_22, label %BB_23

BB_18:                                            ; preds = %BB_16
  %217 = getelementptr i32, i32* %1, i32 0
  %218 = load i32, i32* %217
  %219 = mul i32 2, %218
  %220 = getelementptr [5000 x i32], [5000 x i32]* @swapCells, i32 0, i32 %219
  store i32 1, i32* %220
  %221 = getelementptr i32, i32* %1, i32 0
  %222 = load i32, i32* %221
  %223 = mul i32 2, %222
  %224 = add i32 %223, 1
  %225 = getelementptr [5000 x i32], [5000 x i32]* @swapCells, i32 0, i32 %224
  %226 = getelementptr i32, i32* %1, i32 0
  %227 = load i32, i32* %226
  %228 = mul i32 2, %227
  %229 = add i32 %228, 1
  %230 = getelementptr [5000 x i32], [5000 x i32]* @allCells, i32 0, i32 %229
  %231 = load i32, i32* %230
  %232 = add i32 %231, 1
  store i32 %232, i32* %225
  br label %BB_20

BB_19:                                            ; preds = %BB_16
  %233 = getelementptr i32, i32* %1, i32 0
  %234 = load i32, i32* %233
  %235 = mul i32 2, %234
  %236 = getelementptr [5000 x i32], [5000 x i32]* @swapCells, i32 0, i32 %235
  store i32 0, i32* %236
  %237 = getelementptr i32, i32* %1, i32 0
  %238 = load i32, i32* %237
  %239 = mul i32 2, %238
  %240 = add i32 %239, 1
  %241 = getelementptr [5000 x i32], [5000 x i32]* @swapCells, i32 0, i32 %240
  store i32 0, i32* %241
  br label %BB_20

BB_20:                                            ; preds = %BB_19, %BB_18
  br label %BB_21

BB_21:                                            ; preds = %BB_24, %BB_20
  br label %BB_8

BB_22:                                            ; preds = %BB_17
  %242 = getelementptr i32, i32* %1, i32 0
  %243 = load i32, i32* %242
  %244 = mul i32 2, %243
  %245 = getelementptr [5000 x i32], [5000 x i32]* @swapCells, i32 0, i32 %244
  store i32 1, i32* %245
  %246 = getelementptr i32, i32* %1, i32 0
  %247 = load i32, i32* %246
  %248 = mul i32 2, %247
  %249 = add i32 %248, 1
  %250 = getelementptr [5000 x i32], [5000 x i32]* @swapCells, i32 0, i32 %249
  store i32 1, i32* %250
  br label %BB_24

BB_23:                                            ; preds = %BB_17
  %251 = getelementptr i32, i32* %1, i32 0
  %252 = load i32, i32* %251
  %253 = mul i32 2, %252
  %254 = getelementptr [5000 x i32], [5000 x i32]* @swapCells, i32 0, i32 %253
  store i32 0, i32* %254
  %255 = getelementptr i32, i32* %1, i32 0
  %256 = load i32, i32* %255
  %257 = mul i32 2, %256
  %258 = add i32 %257, 1
  %259 = getelementptr [5000 x i32], [5000 x i32]* @swapCells, i32 0, i32 %258
  store i32 0, i32* %259
  br label %BB_24

BB_24:                                            ; preds = %BB_23, %BB_22
  br label %BB_21

BB_25:                                            ; preds = %BB_27, %BB_5
  %260 = getelementptr i32, i32* %21, i32 0
  %261 = load i32, i32* %260
  %262 = icmp slt i32 %261, 50
  %263 = zext i1 %262 to i32
  %264 = icmp ne i32 %263, 0
  br i1 %264, label %BB_26, label %BB_28

BB_26:                                            ; preds = %BB_25
  %265 = alloca i32
  store i32 0, i32* %265
  br label %BB_29

BB_27:                                            ; preds = %BB_32
  %266 = getelementptr i32, i32* %21, i32 0
  %267 = getelementptr i32, i32* %21, i32 0
  %268 = load i32, i32* %267
  %269 = add i32 %268, 1
  store i32 %269, i32* %266
  br label %BB_25

BB_28:                                            ; preds = %BB_25
  ret void

BB_29:                                            ; preds = %BB_31, %BB_26
  %270 = getelementptr i32, i32* %265, i32 0
  %271 = load i32, i32* %270
  %272 = icmp slt i32 %271, 50
  %273 = zext i1 %272 to i32
  %274 = icmp ne i32 %273, 0
  br i1 %274, label %BB_30, label %BB_32

BB_30:                                            ; preds = %BB_29
  %275 = getelementptr i32, i32* %1, i32 0
  %276 = getelementptr i32, i32* %265, i32 0
  %277 = load i32, i32* %276
  %278 = getelementptr i32, i32* %21, i32 0
  %279 = load i32, i32* %278
  %280 = mul i32 %279, 50
  %281 = add i32 %277, %280
  store i32 %281, i32* %275
  %282 = getelementptr i32, i32* %1, i32 0
  %283 = load i32, i32* %282
  %284 = mul i32 2, %283
  %285 = getelementptr [5000 x i32], [5000 x i32]* @allCells, i32 0, i32 %284
  %286 = getelementptr i32, i32* %1, i32 0
  %287 = load i32, i32* %286
  %288 = mul i32 2, %287
  %289 = getelementptr [5000 x i32], [5000 x i32]* @swapCells, i32 0, i32 %288
  %290 = load i32, i32* %289
  store i32 %290, i32* %285
  %291 = getelementptr i32, i32* %1, i32 0
  %292 = load i32, i32* %291
  %293 = mul i32 2, %292
  %294 = add i32 %293, 1
  %295 = getelementptr [5000 x i32], [5000 x i32]* @allCells, i32 0, i32 %294
  %296 = getelementptr i32, i32* %1, i32 0
  %297 = load i32, i32* %296
  %298 = mul i32 2, %297
  %299 = add i32 %298, 1
  %300 = getelementptr [5000 x i32], [5000 x i32]* @swapCells, i32 0, i32 %299
  %301 = load i32, i32* %300
  store i32 %301, i32* %295
  %302 = alloca i32
  store i32 0, i32* %302
  %303 = getelementptr i32, i32* %1, i32 0
  %304 = load i32, i32* %303
  %305 = mul i32 2, %304
  %306 = getelementptr [5000 x i32], [5000 x i32]* @allCells, i32 0, i32 %305
  %307 = load i32, i32* %306
  %308 = icmp eq i32 %307, 1
  %309 = zext i1 %308 to i32
  %310 = icmp ne i32 %309, 0
  br i1 %310, label %BB_33, label %BB_34

BB_31:                                            ; preds = %BB_34
  %311 = getelementptr i32, i32* %265, i32 0
  %312 = getelementptr i32, i32* %265, i32 0
  %313 = load i32, i32* %312
  %314 = add i32 %313, 1
  store i32 %314, i32* %311
  br label %BB_29

BB_32:                                            ; preds = %BB_29
  br label %BB_27

BB_33:                                            ; preds = %BB_30
  %315 = getelementptr i32, i32* %1, i32 0
  %316 = load i32, i32* %315
  %317 = mul i32 2, %316
  %318 = add i32 %317, 1
  %319 = getelementptr [5000 x i32], [5000 x i32]* @allCells, i32 0, i32 %318
  %320 = load i32, i32* %319
  %321 = alloca i32
  store i32 %320, i32* %321
  %322 = alloca i32
  store i32 0, i32* %322
  %323 = alloca i32
  store i32 0, i32* %323
  %324 = alloca i32
  store i32 0, i32* %324
  %325 = getelementptr i32, i32* %321, i32 0
  %326 = load i32, i32* %325
  %327 = icmp sgt i32 %326, 6
  %328 = zext i1 %327 to i32
  %329 = icmp ne i32 %328, 0
  br i1 %329, label %BB_35, label %BB_36

BB_34:                                            ; preds = %BB_40, %BB_30
  %330 = getelementptr i32, i32* %265, i32 0
  %331 = load i32, i32* %330
  %332 = getelementptr i32, i32* %21, i32 0
  %333 = load i32, i32* %332
  %334 = getelementptr i32, i32* %302, i32 0
  %335 = load i32, i32* %334
  call void @paintCellPixels(i32 %331, i32 %333, i32 %335)
  br label %BB_31

BB_35:                                            ; preds = %BB_33
  %336 = getelementptr i32, i32* %322, i32 0
  %337 = getelementptr i32, i32* %321, i32 0
  %338 = load i32, i32* %337
  %339 = sub i32 10, %338
  %340 = mul i32 2, %339
  %341 = sub i32 127, %340
  store i32 %341, i32* %336
  br label %BB_36

BB_36:                                            ; preds = %BB_35, %BB_33
  %342 = getelementptr i32, i32* %321, i32 0
  %343 = load i32, i32* %342
  %344 = icmp slt i32 %343, 5
  %345 = zext i1 %344 to i32
  %346 = icmp ne i32 %345, 0
  br i1 %346, label %BB_37, label %BB_38

BB_37:                                            ; preds = %BB_36
  %347 = getelementptr i32, i32* %323, i32 0
  %348 = getelementptr i32, i32* %1, i32 0
  %349 = load i32, i32* %348
  %350 = mul i32 2, %349
  %351 = add i32 %350, 1
  %352 = getelementptr [5000 x i32], [5000 x i32]* @allCells, i32 0, i32 %351
  %353 = load i32, i32* %352
  %354 = sdiv i32 255, %353
  store i32 %354, i32* %347
  br label %BB_38

BB_38:                                            ; preds = %BB_37, %BB_36
  %355 = getelementptr i32, i32* %321, i32 0
  %356 = load i32, i32* %355
  %357 = icmp sge i32 %356, 5
  %358 = zext i1 %357 to i32
  %359 = getelementptr i32, i32* %321, i32 0
  %360 = load i32, i32* %359
  %361 = icmp sle i32 %360, 6
  %362 = zext i1 %361 to i32
  %363 = icmp sgt i32 %358, 0
  %364 = zext i1 %363 to i32
  %365 = icmp sgt i32 %362, 0
  %366 = zext i1 %365 to i32
  %367 = and i32 %364, %366
  %368 = icmp ne i32 %367, 0
  br i1 %368, label %BB_39, label %BB_40

BB_39:                                            ; preds = %BB_38
  %369 = getelementptr i32, i32* %324, i32 0
  store i32 127, i32* %369
  br label %BB_40

BB_40:                                            ; preds = %BB_39, %BB_38
  %370 = getelementptr i32, i32* %302, i32 0
  %371 = getelementptr i32, i32* %322, i32 0
  %372 = load i32, i32* %371
  %373 = getelementptr i32, i32* %323, i32 0
  %374 = load i32, i32* %373
  %375 = getelementptr i32, i32* %324, i32 0
  %376 = load i32, i32* %375
  %377 = call i32 @makeColor(i32 %372, i32 %374, i32 %376)
  store i32 %377, i32* %370
  br label %BB_34
}

define void @app() {
BB_1:
  call void @initCells()
  call void @simFlush()
  br label %BB_2

BB_2:                                             ; preds = %BB_3, %BB_1
  br i1 true, label %BB_3, label %BB_4

BB_3:                                             ; preds = %BB_2
  call void @updatePixels()
  call void @simFlush()
  br label %BB_2

BB_4:                                             ; preds = %BB_2
  ret void
}

