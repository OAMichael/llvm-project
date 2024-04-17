#include "RISCXMatInt.h"
#include "MCTargetDesc/RISCXMCTargetDesc.h"
#include "llvm/ADT/APInt.h"
#include "llvm/Support/MathExtras.h"
using namespace llvm;

static void generateInstSeqImpl(int64_t Val, const MCSubtargetInfo &STI, RISCXMatInt::InstSeq &Res) {
  if (isInt<32>(Val)) {
    int64_t Hi20 = ((Val + 0x800) >> 12) & 0xFFFFF;
    int64_t Lo12 = SignExtend64<12>(Val);

    if (Hi20)
      Res.emplace_back(RISCX::LUI, Hi20);

    if (Lo12 || Hi20 == 0) {
      unsigned AddiOpc = Hi20 ? RISCX::ADDIW : RISCX::ADDI;
      Res.emplace_back(AddiOpc, Lo12);
    }
    return;
  }

  int64_t Lo12 = SignExtend64<12>(Val);
  Val = (uint64_t)Val - (uint64_t)Lo12;

  int ShiftAmount = 0;
  if (!isInt<32>(Val)) {
    ShiftAmount = llvm::countr_zero((uint64_t)Val);
    Val >>= ShiftAmount;

    if (ShiftAmount > 12 && !isInt<12>(Val)) {
      if (isInt<32>((uint64_t)Val << 12)) {
        ShiftAmount -= 12;
        Val = (uint64_t)Val << 12;
      }
    }
  }

  generateInstSeqImpl(Val, STI, Res);

  if (Lo12)
    Res.emplace_back(RISCX::ADDI, Lo12);
}

static void generateInstSeqLeadingZeros(int64_t Val, const MCSubtargetInfo &STI, RISCXMatInt::InstSeq &Res) {
  unsigned LeadingZeros = llvm::countl_zero((uint64_t)Val);
  uint64_t ShiftedVal = (uint64_t)Val << LeadingZeros;
  ShiftedVal |= maskTrailingOnes<uint64_t>(LeadingZeros);

  RISCXMatInt::InstSeq TmpSeq;
  generateInstSeqImpl(ShiftedVal, STI, TmpSeq);

  if ((TmpSeq.size() + 1) < Res.size() || (Res.empty() && TmpSeq.size() < 8)) {
    TmpSeq.emplace_back(RISCX::SRLI, LeadingZeros);
    Res = TmpSeq;
  }

  ShiftedVal &= maskTrailingZeros<uint64_t>(LeadingZeros);
  TmpSeq.clear();
  generateInstSeqImpl(ShiftedVal, STI, TmpSeq);

  if ((TmpSeq.size() + 1) < Res.size() || (Res.empty() && TmpSeq.size() < 8)) {
    TmpSeq.emplace_back(RISCX::SRLI, LeadingZeros);
    Res = TmpSeq;
  }
}

namespace llvm::RISCXMatInt {
InstSeq generateInstSeq(int64_t Val, const MCSubtargetInfo &STI) {
  RISCXMatInt::InstSeq Res;
  generateInstSeqImpl(Val, STI, Res);

  if ((Val & 0xfff) != 0 && (Val & 1) == 0 && Res.size() >= 2) {
    unsigned TrailingZeros = llvm::countr_zero((uint64_t)Val);
    int64_t ShiftedVal = Val >> TrailingZeros;
    bool IsShiftedCompressible = isInt<6>(ShiftedVal);
    RISCXMatInt::InstSeq TmpSeq;
    generateInstSeqImpl(ShiftedVal, STI, TmpSeq);

    if ((TmpSeq.size() + 1) < Res.size() || IsShiftedCompressible) {
      TmpSeq.emplace_back(RISCX::SLLI, TrailingZeros);
      Res = TmpSeq;
    }
  }

  if (Res.size() <= 2)
    return Res;

  if ((Val & 0xfff) != 0 && (Val & 0x1800) == 0x1000) {
    int64_t Imm12 = -(0x800 - (Val & 0xfff));
    int64_t AdjustedVal = Val - Imm12;
    RISCXMatInt::InstSeq TmpSeq;
    generateInstSeqImpl(AdjustedVal, STI, TmpSeq);

    if ((TmpSeq.size() + 1) < Res.size()) {
      TmpSeq.emplace_back(RISCX::ADDI, Imm12);
      Res = TmpSeq;
    }
  }

  if (Val > 0 && Res.size() > 2) {
    generateInstSeqLeadingZeros(Val, STI, Res);
  }

  if (Val < 0 && Res.size() > 3) {
    uint64_t InvertedVal = ~(uint64_t)Val;
    RISCXMatInt::InstSeq TmpSeq;
    generateInstSeqLeadingZeros(InvertedVal, STI, TmpSeq);

    if (!TmpSeq.empty() && (TmpSeq.size() + 1) < Res.size()) {
      TmpSeq.emplace_back(RISCX::XORI, -1);
      Res = TmpSeq;
    }
  }
  return Res;
}

InstSeq generateTwoRegInstSeq(int64_t Val, const MCSubtargetInfo &STI, unsigned &ShiftAmt, unsigned &AddOpc) {
  int64_t LoVal = SignExtend64<32>(Val);
  if (LoVal == 0)
    return RISCXMatInt::InstSeq();

  uint64_t Tmp = (uint64_t)Val - (uint64_t)LoVal;

  unsigned TzLo = llvm::countr_zero((uint64_t)LoVal);
  unsigned TzHi = llvm::countr_zero(Tmp);
  ShiftAmt = TzHi - TzLo;
  AddOpc = RISCX::ADD;

  if (Tmp == ((uint64_t)LoVal << ShiftAmt))
    return RISCXMatInt::generateInstSeq(LoVal, STI);

  return RISCXMatInt::InstSeq();
}

OpndKind Inst::getOpndKind() const {
  switch (Opc) {
  default:
    llvm_unreachable("Unexpected opcode!");
  case RISCX::LUI:
    return RISCXMatInt::Imm;
  case RISCX::ADDI:
  case RISCX::ADDIW:
  case RISCX::XORI:
  case RISCX::SLLI:
  case RISCX::SRLI:
    return RISCXMatInt::RegImm;
  }
}

} // namespace llvm::RISCXMatInt