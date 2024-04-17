#include "RISCXSubtarget.h"
#include "RISCX.h"
#include "RISCXFrameLowering.h"
#include "RISCXTargetMachine.h"
#include "llvm/MC/TargetRegistry.h"

using namespace llvm;

#define GET_SUBTARGETINFO_TARGET_DESC
#define GET_SUBTARGETINFO_CTOR
#include "RISCXGenSubtargetInfo.inc"

RISCXSubtarget::RISCXSubtarget(const Triple &TT, StringRef CPU, StringRef FS, const TargetMachine &TM)
    : RISCXGenSubtargetInfo(TT, CPU, CPU, FS),
      FrameLowering(*this),
      InstrInfo(*this), RegInfo(getHwMode()), TLInfo(TM, *this) {
}

unsigned RISCXSubtarget::getMaxBuildIntsCost() const {
  return getSchedModel().LoadLatency + 1;
}