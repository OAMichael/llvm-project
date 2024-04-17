#ifndef LLVM_LIB_TARGET_RISCX_MCTARGETDESC_RISCXBASEINFO_H
#define LLVM_LIB_TARGET_RISCX_MCTARGETDESC_RISCXBASEINFO_H

#include "MCTargetDesc/RISCXMCTargetDesc.h"
#include "llvm/ADT/APInt.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/MC/MCInstrDesc.h"
#include "llvm/TargetParser/Triple.h"

namespace llvm {

namespace RISCXII {
enum {
  InstFormatPseudo = 0,
  InstFormatR = 1,
  InstFormatI = 2,
  InstFormatS = 3,
  InstFormatB = 4,
  InstFormatU = 5,
  InstFormatJ = 6,
  InstFormatOther = 7,

  InstFormatMask = 31,
  InstFormatShift = 0,
};

static inline unsigned getFormat(uint64_t TSFlags) {
  return (TSFlags & InstFormatMask) >> InstFormatShift;
}

enum {
  MO_None = 0,
  MO_CALL = 1,
  MO_LO = 3,
  MO_HI = 4,
  MO_PCREL_LO = 5,
  MO_PCREL_HI = 6,
  MO_GOT_HI = 7,

  MO_DIRECT_FLAG_MASK = 31
};
}

namespace RISCXOp {
enum OperandType : unsigned {
  OPERAND_FIRST_RISCX_IMM = MCOI::OPERAND_FIRST_TARGET,
  OPERAND_UIMM5 = OPERAND_FIRST_RISCX_IMM,
  OPERAND_SIMM12,
  OPERAND_SIMM12_LSB00000,
  OPERAND_UIMM20,
  OPERAND_UIMMLOG2XLEN,
  OPERAND_LAST_RISCX_IMM = OPERAND_UIMMLOG2XLEN,
};
} // namespace RISCXOp

namespace RISCXABI {
enum ABI {
  ABI_LP64,
  ABI_Unknown
};
} // namespace RISCXABI

} // namespace llvm
#endif  // LLVM_LIB_TARGET_RISCX_MCTARGETDESC_RISCXBASEINFO_H