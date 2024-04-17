#ifndef LLVM_LIB_TARGET_RISCX_MCTARGETDESC_RISCXMCEXPR_H
#define LLVM_LIB_TARGET_RISCX_MCTARGETDESC_RISCXMCEXPR_H

#include "llvm/MC/MCExpr.h"

namespace llvm {

class StringRef;

class RISCXMCExpr : public MCTargetExpr {
public:
  enum VariantKind {
    VK_RISCX_None,
    VK_RISCX_LO,
    VK_RISCX_HI,
    VK_RISCX_PCREL_LO,
    VK_RISCX_PCREL_HI,
    VK_RISCX_GOT_HI,
    VK_RISCX_CALL,
    VK_RISCX_32_PCREL,
    VK_RISCX_Invalid
  };

private:
  const MCExpr *Expr;
  const VariantKind Kind;

  explicit RISCXMCExpr(const MCExpr *Expr, VariantKind Kind) : Expr(Expr), Kind(Kind) {}

public:
  static const RISCXMCExpr *create(const MCExpr *Expr, VariantKind Kind, MCContext &Ctx);

  VariantKind getKind() const { return Kind; }

  const MCExpr *getSubExpr() const { return Expr; }

  const MCFixup *getPCRelHiFixup(const MCFragment **DFOut) const;

  void printImpl(raw_ostream &OS, const MCAsmInfo *MAI) const override;

  bool evaluateAsRelocatableImpl(MCValue &Res, const MCAsmLayout *Layout, const MCFixup *Fixup) const override;

  void visitUsedExpr(MCStreamer &Streamer) const override;

  MCFragment *findAssociatedFragment() const override { return getSubExpr()->findAssociatedFragment(); }

  void fixELFSymbolsInTLSFixups(MCAssembler &Asm) const override {}

  static StringRef getVariantKindName(VariantKind Kind);
};
} // end namespace llvm.
#endif  // LLVM_LIB_TARGET_RISCX_MCTARGETDESC_RISCXMCEXPR_H