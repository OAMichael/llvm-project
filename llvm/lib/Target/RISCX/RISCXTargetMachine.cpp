#include "RISCXTargetMachine.h"
#include "MCTargetDesc/RISCXBaseInfo.h"
#include "RISCX.h"
#include "TargetInfo/RISCXTargetInfo.h"
#include "llvm/CodeGen/Passes.h"
#include "llvm/CodeGen/TargetLoweringObjectFileImpl.h"
#include "llvm/CodeGen/TargetPassConfig.h"
#include "llvm/MC/TargetRegistry.h"
#include "llvm/Target/TargetOptions.h"

using namespace llvm;


extern "C" LLVM_EXTERNAL_VISIBILITY void LLVMInitializeRISCXTarget() {
  RegisterTargetMachine<RISCXTargetMachine> X(getTheRISCXTarget());
  auto *PR = PassRegistry::getPassRegistry();
  initializeRISCXDAGToDAGISelPass(*PR);
}


RISCXTargetMachine::RISCXTargetMachine(const Target &T, const Triple &TT,
                                       StringRef CPU, StringRef FS,
                                       const TargetOptions &Options,
                                       std::optional<Reloc::Model> RM,
                                       std::optional<CodeModel::Model> CM,
                                       CodeGenOptLevel OL, bool JIT)
    : LLVMTargetMachine(T, "e-m:e-p:64:64-i64:64-i128:128-n32:64-S128", TT, CPU, FS, Options, Reloc::Static,
                        getEffectiveCodeModel(CM, CodeModel::Small), OL), TLOF(std::make_unique<TargetLoweringObjectFileELF>()),
                        Subtarget(TT, std::string(CPU), std::string(FS), *this) {
  initAsmInfo();
}

namespace {

class RISCXPassConfig : public TargetPassConfig {
public:
  RISCXPassConfig(RISCXTargetMachine &TM, PassManagerBase &PM)
      : TargetPassConfig(TM, PM) {}

  RISCXTargetMachine &getRISCXTargetMachine() const { return getTM<RISCXTargetMachine>(); }

  bool addInstSelector() override;
};
} // namespace

TargetPassConfig *RISCXTargetMachine::createPassConfig(PassManagerBase &PM) {
  return new RISCXPassConfig(*this, PM);
}

bool RISCXPassConfig::addInstSelector() {
  addPass(createRISCXISelDag(getRISCXTargetMachine(), getOptLevel()));
  return false;
}