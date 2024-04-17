#ifndef LLVM_LIB_TARGET_RISCX_MCTARGETDESC_RISCXMCTARGETDESC_H
#define LLVM_LIB_TARGET_RISCX_MCTARGETDESC_RISCXMCTARGETDESC_H

#include "llvm/Config/config.h"
#include "llvm/MC/MCTargetOptions.h"
#include "llvm/Support/DataTypes.h"
#include <memory>

namespace llvm {
class MCAsmBackend;
class MCCodeEmitter;
class MCContext;
class MCInstrInfo;
class MCObjectTargetWriter;
class MCRegisterInfo;
class MCSubtargetInfo;
class Target;

MCCodeEmitter *createRISCXMCCodeEmitter(const MCInstrInfo &MCII, MCContext &Ctx);
MCAsmBackend *createRISCXAsmBackend(const Target &T, const MCSubtargetInfo &STI,
                                    const MCRegisterInfo &MRI,
                                    const MCTargetOptions &Options);
std::unique_ptr<MCObjectTargetWriter> createRISCXELFObjectWriter(uint8_t OSABI);
}

#define GET_REGINFO_ENUM
#include "RISCXGenRegisterInfo.inc"

#define GET_INSTRINFO_ENUM
#include "RISCXGenInstrInfo.inc"

#endif  // LLVM_LIB_TARGET_RISCX_MCTARGETDESC_RISCXMCTARGETDESC_H