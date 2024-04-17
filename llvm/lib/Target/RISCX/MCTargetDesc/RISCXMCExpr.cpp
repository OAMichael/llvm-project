#include "RISCXMCExpr.h"
#include "MCTargetDesc/RISCXAsmBackend.h"
#include "RISCXFixupKinds.h"
#include "llvm/BinaryFormat/ELF.h"
#include "llvm/MC/MCAsmLayout.h"
#include "llvm/MC/MCAssembler.h"
#include "llvm/MC/MCContext.h"
#include "llvm/MC/MCStreamer.h"
#include "llvm/MC/MCSymbolELF.h"
#include "llvm/MC/MCValue.h"
#include "llvm/Support/Casting.h"

using namespace llvm;

const RISCXMCExpr *RISCXMCExpr::create(const MCExpr *Expr, VariantKind Kind, MCContext &Ctx) {
  return new (Ctx) RISCXMCExpr(Expr, Kind);
}

void RISCXMCExpr::printImpl(raw_ostream &OS, const MCAsmInfo *MAI) const {
  VariantKind Kind = getKind();
  bool HasVariant = ((Kind != VK_RISCX_None) && (Kind != VK_RISCX_CALL));

  if (HasVariant)
    OS << '%' << getVariantKindName(getKind()) << '(';
  Expr->print(OS, MAI);
  if (HasVariant)
    OS << ')';
}

const MCFixup *RISCXMCExpr::getPCRelHiFixup(const MCFragment **DFOut) const {
  MCValue AUIPCLoc;
  if (!getSubExpr()->evaluateAsRelocatable(AUIPCLoc, nullptr, nullptr))
    return nullptr;

  const MCSymbolRefExpr *AUIPCSRE = AUIPCLoc.getSymA();
  if (!AUIPCSRE)
    return nullptr;

  const MCSymbol *AUIPCSymbol = &AUIPCSRE->getSymbol();
  const auto *DF = dyn_cast_or_null<MCDataFragment>(AUIPCSymbol->getFragment());

  if (!DF)
    return nullptr;

  uint64_t Offset = AUIPCSymbol->getOffset();
  if (DF->getContents().size() == Offset) {
    DF = dyn_cast_or_null<MCDataFragment>(DF->getNextNode());
    if (!DF)
      return nullptr;
    Offset = 0;
  }

  for (const MCFixup &F : DF->getFixups()) {
    if (F.getOffset() != Offset)
      continue;

    switch ((unsigned)F.getKind()) {
    default:
      continue;
    case RISCX::fixup_riscx_got_hi20:
    case RISCX::fixup_riscx_pcrel_hi20:
      if (DFOut)
        *DFOut = DF;
      return &F;
    }
  }

  return nullptr;
}

bool RISCXMCExpr::evaluateAsRelocatableImpl(MCValue &Res, const MCAsmLayout *Layout, const MCFixup *Fixup) const {
  if (!getSubExpr()->evaluateAsRelocatable(Res, nullptr, nullptr))
    return false;

  Res = MCValue::get(Res.getSymA(), Res.getSymB(), Res.getConstant(), getKind());
  return Res.getSymB() ? getKind() == VK_RISCX_None : true;
}

void RISCXMCExpr::visitUsedExpr(MCStreamer &Streamer) const {
  Streamer.visitUsedExpr(*getSubExpr());
}

StringRef RISCXMCExpr::getVariantKindName(VariantKind Kind) {
  switch (Kind) {
  case VK_RISCX_Invalid:
  case VK_RISCX_None:
    llvm_unreachable("Invalid ELF symbol kind");
  case VK_RISCX_LO:
    return "lo";
  case VK_RISCX_HI:
    return "hi";
  case VK_RISCX_PCREL_LO:
    return "pcrel_lo";
  case VK_RISCX_PCREL_HI:
    return "pcrel_hi";
  case VK_RISCX_GOT_HI:
    return "got_pcrel_hi";
  case VK_RISCX_CALL:
    return "call";
  case VK_RISCX_32_PCREL:
    return "32_pcrel";
  }
  llvm_unreachable("Invalid ELF symbol kind");
}