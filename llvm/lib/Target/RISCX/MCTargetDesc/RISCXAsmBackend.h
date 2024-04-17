#ifndef LLVM_LIB_TARGET_RISCX_MCTARGETDESC_RISCXASMBACKEND_H
#define LLVM_LIB_TARGET_RISCX_MCTARGETDESC_RISCXASMBACKEND_H

#include "MCTargetDesc/RISCXBaseInfo.h"
#include "MCTargetDesc/RISCXFixupKinds.h"
#include "MCTargetDesc/RISCXMCTargetDesc.h"
#include "llvm/MC/MCAsmBackend.h"
#include "llvm/MC/MCFixupKindInfo.h"
#include "llvm/MC/MCSubtargetInfo.h"

namespace llvm {
class MCAssembler;
class MCObjectTargetWriter;
class raw_ostream;

class RISCXAsmBackend : public MCAsmBackend {
  const MCSubtargetInfo &STI;
  uint8_t OSABI;
  const MCTargetOptions &TargetOptions;

public:
  RISCXAsmBackend(const MCSubtargetInfo &STI, uint8_t OSABI, const MCTargetOptions &Options)
      : MCAsmBackend(llvm::endianness::little, RISCX::fixup_riscx_relax),
        STI(STI), OSABI(OSABI), TargetOptions(Options) {
  }
  ~RISCXAsmBackend() override = default;

  void applyFixup(const MCAssembler &Asm, const MCFixup &Fixup,
                  const MCValue &Target, MutableArrayRef<char> Data,
                  uint64_t Value, bool IsResolved,
                  const MCSubtargetInfo *STI) const override;

  std::unique_ptr<MCObjectTargetWriter> createObjectTargetWriter() const override;

  bool fixupNeedsRelaxation(const MCFixup &Fixup, uint64_t Value,
                            const MCRelaxableFragment *DF,
                            const MCAsmLayout &Layout) const override {
    llvm_unreachable("fixupNeedsRelaxation() unimplemented");
    return false;
  }

  unsigned getNumFixupKinds() const override { return RISCX::NumTargetFixupKinds; }

  std::optional<MCFixupKind> getFixupKind(StringRef Name) const override;

  const MCFixupKindInfo &getFixupKindInfo(MCFixupKind Kind) const override;

  bool writeNopData(raw_ostream &OS, uint64_t Count, const MCSubtargetInfo *STI) const override;

  bool evaluateTargetFixup(const MCAssembler &Asm, const MCAsmLayout &Layout,
                          const MCFixup &Fixup, const MCFragment *DF,
                           const MCValue &Target, const MCSubtargetInfo *STI,
                           uint64_t &Value, bool &WasForced) override;
};
}

#endif  // LLVM_LIB_TARGET_RISCX_MCTARGETDESC_RISCXASMBACKEND_H