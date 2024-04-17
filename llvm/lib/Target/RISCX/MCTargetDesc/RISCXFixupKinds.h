#ifndef LLVM_LIB_TARGET_RISCX_MCTARGETDESC_RISCXFIXUPKINDS_H
#define LLVM_LIB_TARGET_RISCX_MCTARGETDESC_RISCXFIXUPKINDS_H

#include "llvm/BinaryFormat/ELF.h"
#include "llvm/MC/MCFixup.h"
#include <utility>

namespace llvm::RISCX {
enum Fixups {
  fixup_riscx_hi20 = FirstTargetFixupKind,
  fixup_riscx_lo12_i,
  fixup_riscx_12_i,
  fixup_riscx_lo12_s,
  fixup_riscx_pcrel_hi20,
  fixup_riscx_pcrel_lo12_i,
  fixup_riscx_pcrel_lo12_s,
  fixup_riscx_got_hi20,
  fixup_riscx_jal,
  fixup_riscx_branch,
  fixup_riscx_call,
  fixup_riscx_relax,
  fixup_riscx_align,

  fixup_riscx_invalid,
  NumTargetFixupKinds = fixup_riscx_invalid - FirstTargetFixupKind
};

} // end namespace llvm::RISCX

#endif  // LLVM_LIB_TARGET_RISCX_MCTARGETDESC_RISCXFIXUPKINDS_H
