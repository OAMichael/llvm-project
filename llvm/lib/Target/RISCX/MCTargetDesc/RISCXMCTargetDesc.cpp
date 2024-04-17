#include "RISCXMCTargetDesc.h"
#include "RISCXBaseInfo.h"
#include "RISCXInstPrinter.h"
#include "RISCXMCAsmInfo.h"
#include "RISCXTargetStreamer.h"
#include "TargetInfo/RISCXTargetInfo.h"
#include "llvm/MC/MCAsmBackend.h"
#include "llvm/MC/MCAsmInfo.h"
#include "llvm/MC/MCCodeEmitter.h"
#include "llvm/MC/MCInstrInfo.h"
#include "llvm/MC/MCObjectWriter.h"
#include "llvm/MC/MCRegisterInfo.h"
#include "llvm/MC/MCSubtargetInfo.h"
#include "llvm/MC/TargetRegistry.h"
#include <bitset>

#define GET_INSTRINFO_MC_DESC
#define ENABLE_INSTR_PREDICATE_VERIFIER
#include "RISCXGenInstrInfo.inc"

#define GET_REGINFO_MC_DESC
#include "RISCXGenRegisterInfo.inc"

#define GET_SUBTARGETINFO_MC_DESC
#include "RISCXGenSubtargetInfo.inc"

using namespace llvm;

static MCInstrInfo *createRISCXMCInstrInfo() {
  MCInstrInfo *X = new MCInstrInfo();
  InitRISCXMCInstrInfo(X);
  return X;
}

static MCRegisterInfo *createRISCXMCRegisterInfo(const Triple &TT) {
  MCRegisterInfo *X = new MCRegisterInfo();
  InitRISCXMCRegisterInfo(X, RISCX::X1);
  return X;
}

static MCAsmInfo *createRISCXMCAsmInfo(const MCRegisterInfo &MRI,
                                       const Triple &TT,
                                       const MCTargetOptions &Options) {
  return new RISCXMCAsmInfo(TT);
}

static MCSubtargetInfo *createRISCXMCSubtargetInfo(const Triple &TT, StringRef CPU, StringRef FS) {
  return createRISCXMCSubtargetInfoImpl(TT, CPU, CPU, FS);
}

static MCInstPrinter *createRISCXMCInstPrinter(const Triple &T,
                                               unsigned SyntaxVariant,
                                               const MCAsmInfo &MAI,
                                               const MCInstrInfo &MII,
                                               const MCRegisterInfo &MRI) {
  return new RISCXInstPrinter(MAI, MII, MRI);
}

static MCTargetStreamer *createRISCXTargetAsmStreamer(MCStreamer &S,
                                                      formatted_raw_ostream &OS,
                                                      MCInstPrinter *InstPrint,
                                                      bool isVerboseAsm) {
  return new RISCXTargetStreamer(S);
}


extern "C" LLVM_EXTERNAL_VISIBILITY void LLVMInitializeRISCXTargetMC() {
  Target *TheRISCXTarget = &getTheRISCXTarget();

  // Register the MCAsmInfo
  TargetRegistry::RegisterMCAsmInfo(*TheRISCXTarget, createRISCXMCAsmInfo);
  // Register the MC register info.
  TargetRegistry::RegisterMCRegInfo(*TheRISCXTarget, createRISCXMCRegisterInfo);
  // Register the MC instruction info.
  TargetRegistry::RegisterMCInstrInfo(*TheRISCXTarget, createRISCXMCInstrInfo);
  // Register the MC subtarget info.
  TargetRegistry::RegisterMCSubtargetInfo(*TheRISCXTarget, createRISCXMCSubtargetInfo);
  // Register the MCInstPrinter
  TargetRegistry::RegisterMCInstPrinter(*TheRISCXTarget, createRISCXMCInstPrinter);
  // Register the AsmTargetStreamer.
  TargetRegistry::RegisterAsmTargetStreamer(*TheRISCXTarget, createRISCXTargetAsmStreamer);
  // Register the MC Code Emitter.
  TargetRegistry::RegisterMCCodeEmitter(*TheRISCXTarget, createRISCXMCCodeEmitter);
  // Register the asm backend.
  TargetRegistry::RegisterMCAsmBackend(*TheRISCXTarget, createRISCXAsmBackend);
}
