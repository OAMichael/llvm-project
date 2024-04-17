#ifndef LLVM_LIB_TARGET_RISCX_MCTARGETDESC_RISCXINSTPRINTER_H
#define LLVM_LIB_TARGET_RISCX_MCTARGETDESC_RISCXINSTPRINTER_H

#include "MCTargetDesc/RISCXMCTargetDesc.h"
#include "llvm/MC/MCInstPrinter.h"

namespace llvm {

class RISCXInstPrinter : public MCInstPrinter {
public:
  RISCXInstPrinter(const MCAsmInfo &MAI, const MCInstrInfo &MII, const MCRegisterInfo &MRI)
      : MCInstPrinter(MAI, MII, MRI) {}

  void printInst(const MCInst *MI, uint64_t Address, StringRef Annot, const MCSubtargetInfo &STI, raw_ostream &O) override;
  void printRegName(raw_ostream &O, MCRegister Reg) const override;
  void printOperand(const MCInst *MI, unsigned OpNo, const MCSubtargetInfo &STI, raw_ostream &O, const char *Modifier = nullptr);
  void printBranchOperand(const MCInst *MI, uint64_t Address, unsigned OpNo, const MCSubtargetInfo &STI, raw_ostream &O);

  std::pair<const char *, uint64_t> getMnemonic(const MCInst *MI) override;
  void printInstruction(const MCInst *MI, uint64_t Address, const MCSubtargetInfo &STI, raw_ostream &O);

  static const char *getRegisterName(MCRegister Reg);
  static const char *getRegisterName(MCRegister Reg, unsigned AltIdx);
};
} // namespace llvm

#endif  // LLVM_LIB_TARGET_RISCX_MCTARGETDESC_RISCXINSTPRINTER_H