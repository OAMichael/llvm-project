#ifndef LLVM_LIB_TARGET_RISCX_MCTARGETDESC_RISCXTARGETSTREAMER_H
#define LLVM_LIB_TARGET_RISCX_MCTARGETDESC_RISCXTARGETSTREAMER_H

#include "llvm/MC/MCStreamer.h"

namespace llvm {

class RISCXTargetStreamer : public MCTargetStreamer {
public:
  RISCXTargetStreamer(MCStreamer &S) : MCTargetStreamer(S) {};
  ~RISCXTargetStreamer() = default;
};

}

#endif  // LLVM_LIB_TARGET_RISCX_MCTARGETDESC_RISCXTARGETSTREAMER_H