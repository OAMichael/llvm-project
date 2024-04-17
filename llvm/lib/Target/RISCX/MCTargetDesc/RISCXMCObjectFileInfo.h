#ifndef LLVM_LIB_TARGET_RISCX_MCTARGETDESC_RISCXMCOBJECTFILEINFO_H
#define LLVM_LIB_TARGET_RISCX_MCTARGETDESC_RISCXMCOBJECTFILEINFO_H

#include "llvm/MC/MCObjectFileInfo.h"
#include "llvm/MC/MCSubtargetInfo.h"

namespace llvm {

class RISCXMCObjectFileInfo : public MCObjectFileInfo {
public:
  static unsigned getTextSectionAlignmentStatic() { return 4; };
};

} // namespace llvm

#endif  // LLVM_LIB_TARGET_RISCX_MCTARGETDESC_RISCXMCOBJECTFILEINFO_H