#include "RISCXInstPrinter.h"
#include "RISCXBaseInfo.h"
#include "RISCXMCExpr.h"
#include "llvm/MC/MCAsmInfo.h"
#include "llvm/MC/MCExpr.h"
#include "llvm/MC/MCInst.h"
#include "llvm/MC/MCInstPrinter.h"
#include "llvm/MC/MCRegisterInfo.h"
#include "llvm/MC/MCSubtargetInfo.h"
#include "llvm/MC/MCSymbol.h"
#include "llvm/Support/FormattedStream.h"
using namespace llvm;

#include "RISCXGenAsmWriter.inc"

void RISCXInstPrinter::printInst(const MCInst *MI, uint64_t Address, StringRef Annot, const MCSubtargetInfo &STI, raw_ostream &O) {
  const MCInst *NewMI = MI;
  printInstruction(NewMI, Address, STI, O);
  printAnnotation(O, Annot);
}

void RISCXInstPrinter::printRegName(raw_ostream &O, MCRegister Reg) const {
  markup(O, Markup::Register) << getRegisterName(Reg);
}

void RISCXInstPrinter::printOperand(const MCInst *MI, unsigned OpNo, const MCSubtargetInfo &STI, raw_ostream &O, const char *Modifier) {
  const MCOperand &MO = MI->getOperand(OpNo);

  if (MO.isReg()) {
    printRegName(O, MO.getReg());
    return;
  }

  if (MO.isImm()) {
    markup(O, Markup::Immediate) << formatImm(MO.getImm());
    return;
  }

  MO.getExpr()->print(O, &MAI);
}

void RISCXInstPrinter::printBranchOperand(const MCInst *MI, uint64_t Address,
                                          unsigned OpNo,
                                          const MCSubtargetInfo &STI,
                                          raw_ostream &O) {
  const MCOperand &MO = MI->getOperand(OpNo);
  if (!MO.isImm())
    return printOperand(MI, OpNo, STI, O);

  if (PrintBranchImmAsAddress) {
    uint64_t Target = Address + MO.getImm();
    markup(O, Markup::Target) << formatHex(Target);
  } else {
    markup(O, Markup::Target) << formatImm(MO.getImm());
  }
}

const char *RISCXInstPrinter::getRegisterName(MCRegister Reg) {
  return getRegisterName(Reg, RISCX::NoRegAltName);
}