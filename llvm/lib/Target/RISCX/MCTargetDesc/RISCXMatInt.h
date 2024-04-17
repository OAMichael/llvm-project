#ifndef LLVM_LIB_TARGET_RISCX_MCTARGETDESC_MATINT_H
#define LLVM_LIB_TARGET_RISCX_MCTARGETDESC_MATINT_H

#include "llvm/ADT/SmallVector.h"
#include "llvm/MC/MCSubtargetInfo.h"
#include <cstdint>

namespace llvm {
class APInt;

namespace RISCXMatInt {

enum OpndKind {
  RegImm,
  Imm
};

class Inst {
  unsigned Opc;
  int32_t Imm;

public:
  Inst(unsigned Opc, int64_t I) : Opc(Opc), Imm(I) {}

  unsigned getOpcode() const { return Opc; }
  int64_t getImm() const { return Imm; }

  OpndKind getOpndKind() const;
};
using InstSeq = SmallVector<Inst, 8>;

InstSeq generateInstSeq(int64_t Val, const MCSubtargetInfo &STI);
InstSeq generateTwoRegInstSeq(int64_t Val, const MCSubtargetInfo &STI, unsigned &ShiftAmt, unsigned &AddOpc);
} // namespace RISCXMatInt
} // namespace llvm
#endif  // LLVM_LIB_TARGET_RISCX_MCTARGETDESC_MATINT_H