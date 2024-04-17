#ifndef LLVM_LIB_TARGET_RISCX_MCTARGETDESC_RISCXMCASMINFO_H
#define LLVM_LIB_TARGET_RISCX_MCTARGETDESC_RISCXMCASMINFO_H

#include "llvm/MC/MCAsmInfoELF.h"

namespace llvm {
class Triple;

class RISCXMCAsmInfo : public MCAsmInfoELF {

public:
  explicit RISCXMCAsmInfo(const Triple &TheTriple);
};

} // namespace llvm

#endif  // LLVM_LIB_TARGET_RISCX_MCTARGETDESC_RISCXMCASMINFO_H