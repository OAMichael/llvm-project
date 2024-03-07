#include "TargetInfo/RISCXTargetInfo.h"
#include "llvm/MC/TargetRegistry.h"
using namespace llvm;

Target &llvm::getTheRISCXTarget() {
  static Target TheRISCXTarget;
  return TheRISCXTarget;
}

extern "C" LLVM_EXTERNAL_VISIBILITY void LLVMInitializeRISCXTargetInfo() {
  RegisterTarget<Triple::riscx> X(getTheRISCXTarget(), "riscx", "RISC-X 64",
                                   "RISC-X");
}

