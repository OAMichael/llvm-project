#ifndef LLVM_LIB_TARGET_RISCX_RISCXTARGETMACHINE_H
#define LLVM_LIB_TARGET_RISCX_RISCXTARGETMACHINE_H

#include "MCTargetDesc/RISCXMCTargetDesc.h"
#include "RISCXSubtarget.h"
#include "llvm/CodeGen/SelectionDAGTargetInfo.h"
#include "llvm/IR/DataLayout.h"
#include "llvm/Target/TargetMachine.h"
#include <optional>

namespace llvm {
class RISCXTargetMachine : public LLVMTargetMachine {
  std::unique_ptr<TargetLoweringObjectFile> TLOF;
  RISCXSubtarget Subtarget;

public:
  RISCXTargetMachine(const Target &T, const Triple &TT, StringRef CPU,
                     StringRef FS, const TargetOptions &Options,
                     std::optional<Reloc::Model> RM,
                     std::optional<CodeModel::Model> CM, CodeGenOptLevel OL,
                     bool JIT);

  const RISCXSubtarget *getSubtargetImpl(const Function &) const override { return &Subtarget; }
  const RISCXSubtarget *getSubtargetImpl() const { return &Subtarget; }

  TargetPassConfig *createPassConfig(PassManagerBase &PM) override;
  TargetLoweringObjectFile *getObjFileLowering() const override { return TLOF.get(); }
};
} // namespace llvm

#endif  // LLVM_LIB_TARGET_RISCX_RISCXTARGETMACHINE_H