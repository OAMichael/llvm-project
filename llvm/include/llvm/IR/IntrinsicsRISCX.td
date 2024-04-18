let TargetPrefix = "riscx" in {  // All intrinsics start with "llvm.riscx.".

def int_riscx_putpixel: ClangBuiltin<"putPixel">,
  Intrinsic<[], [llvm_i32_ty, llvm_i32_ty, llvm_i32_ty], [IntrNoMem, IntrHasSideEffects]>;

def int_riscx_flush: ClangBuiltin<"flush">,
  Intrinsic<[], [], [IntrNoMem, IntrHasSideEffects]>;

def int_riscx_rand: ClangBuiltin<"rand">,
  Intrinsic<[llvm_i32_ty], [], [IntrNoMem, IntrHasSideEffects]>;

} // TargetPrefix = "riscx"