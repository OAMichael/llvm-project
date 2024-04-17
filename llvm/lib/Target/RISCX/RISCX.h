#ifndef LLVM_LIB_TARGET_RISCX_RISCX_H
#define LLVM_LIB_TARGET_RISCX_RISCX_H

#include "MCTargetDesc/RISCXBaseInfo.h"
#include "llvm/Target/TargetMachine.h"

namespace llvm {
class FunctionPass;
class PassRegistry;
class RISCXTargetMachine;

FunctionPass *createRISCXISelDag(RISCXTargetMachine &TM, CodeGenOptLevel OptLevel);

void initializeRISCXDAGToDAGISelPass(PassRegistry &);
} // namespace llvm

#endif  // LLVM_LIB_TARGET_RISCX_RISCX_H