#include "MCTargetDesc/RISCXFixupKinds.h"
#include "MCTargetDesc/RISCXMCExpr.h"
#include "MCTargetDesc/RISCXMCTargetDesc.h"
#include "llvm/MC/MCContext.h"
#include "llvm/MC/MCELFObjectWriter.h"
#include "llvm/MC/MCFixup.h"
#include "llvm/MC/MCObjectWriter.h"
#include "llvm/MC/MCValue.h"

using namespace llvm;

namespace {
class RISCXELFObjectWriter : public MCELFObjectTargetWriter {
public:
  RISCXELFObjectWriter(uint8_t OSABI);

  ~RISCXELFObjectWriter() override;

protected:
  unsigned getRelocType(MCContext &Ctx, const MCValue &Target, const MCFixup &Fixup, bool IsPCRel) const override;
};
}

RISCXELFObjectWriter::RISCXELFObjectWriter(uint8_t OSABI)
    : MCELFObjectTargetWriter(/*Is64Bit*/true, OSABI, ELF::EM_RISCV, /*HasRelocationAddend*/ true) {}

RISCXELFObjectWriter::~RISCXELFObjectWriter() = default;

unsigned RISCXELFObjectWriter::getRelocType(MCContext &Ctx,
                                            const MCValue &Target,
                                            const MCFixup &Fixup,
                                            bool IsPCRel) const {
  const MCExpr *Expr = Fixup.getValue();
  unsigned Kind = Fixup.getTargetKind();
  if (Kind >= FirstLiteralRelocationKind)
    return Kind - FirstLiteralRelocationKind;
  if (IsPCRel) {
    switch (Kind) {
    default:
      Ctx.reportError(Fixup.getLoc(), "unsupported relocation type");
      return ELF::R_RISCV_NONE;
    case FK_Data_4:
    case FK_PCRel_4:
      return ELF::R_RISCV_32_PCREL;
    case RISCX::fixup_riscx_pcrel_hi20:
      return ELF::R_RISCV_PCREL_HI20;
    case RISCX::fixup_riscx_pcrel_lo12_i:
      return ELF::R_RISCV_PCREL_LO12_I;
    case RISCX::fixup_riscx_pcrel_lo12_s:
      return ELF::R_RISCV_PCREL_LO12_S;
    case RISCX::fixup_riscx_got_hi20:
      return ELF::R_RISCV_GOT_HI20;
    case RISCX::fixup_riscx_jal:
      return ELF::R_RISCV_JAL;
    case RISCX::fixup_riscx_branch:
      return ELF::R_RISCV_BRANCH;
    case RISCX::fixup_riscx_call:
      return ELF::R_RISCV_CALL;
    }
  }

  switch (Kind) {
  default:
    Ctx.reportError(Fixup.getLoc(), "unsupported relocation type");
    return ELF::R_RISCV_NONE;
  case FK_Data_1:
    Ctx.reportError(Fixup.getLoc(), "1-byte data relocations not supported");
    return ELF::R_RISCV_NONE;
  case FK_Data_2:
    Ctx.reportError(Fixup.getLoc(), "2-byte data relocations not supported");
    return ELF::R_RISCV_NONE;
  case FK_Data_4:
    if (Expr->getKind() == MCExpr::Target && cast<RISCXMCExpr>(Expr)->getKind() == RISCXMCExpr::VK_RISCX_32_PCREL)
      return ELF::R_RISCV_32_PCREL;
    if (Target.getSymA()->getKind() == MCSymbolRefExpr::VK_GOTPCREL)
      return ELF::R_RISCV_GOT32_PCREL;
    return ELF::R_RISCV_32;
  case FK_Data_8:
    return ELF::R_RISCV_64;
  case RISCX::fixup_riscx_hi20:
    return ELF::R_RISCV_HI20;
  case RISCX::fixup_riscx_lo12_i:
    return ELF::R_RISCV_LO12_I;
  case RISCX::fixup_riscx_lo12_s:
    return ELF::R_RISCV_LO12_S;
  case RISCX::fixup_riscx_relax:
    return ELF::R_RISCV_RELAX;
  case RISCX::fixup_riscx_align:
    return ELF::R_RISCV_ALIGN;
  }
}

std::unique_ptr<MCObjectTargetWriter> llvm::createRISCXELFObjectWriter(uint8_t OSABI) {
  return std::make_unique<RISCXELFObjectWriter>(OSABI);
}
