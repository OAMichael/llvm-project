; ModuleID = 'top'
source_filename = "top"

declare void @llvm.riscx.putpixel(i32, i32, i32)

declare void @llvm.riscx.flush()

declare i32 @llvm.riscx.rand()

define i32 @app() {
BB_1:
  ret i32 12
}
