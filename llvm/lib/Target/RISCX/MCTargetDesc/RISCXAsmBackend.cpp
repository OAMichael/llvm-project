#include "RISCXAsmBackend.h"
#include "RISCXMCExpr.h"
#include "llvm/ADT/APInt.h"
#include "llvm/MC/MCAsmInfo.h"
#include "llvm/MC/MCAsmLayout.h"
#include "llvm/MC/MCAssembler.h"
#include "llvm/MC/MCContext.h"
#include "llvm/MC/MCELFObjectWriter.h"
#include "llvm/MC/MCExpr.h"
#include "llvm/MC/MCObjectWriter.h"
#include "llvm/MC/MCSymbol.h"
#include "llvm/MC/MCValue.h"
#include "llvm/Support/Endian.h"
#include "llvm/Support/EndianStream.h"
#include "llvm/Support/raw_ostream.h"

using namespace llvm;

std::optional<MCFixupKind> RISCXAsmBackend::getFixupKind(StringRef Name) const {
  return static_cast<MCFixupKind>(FirstLiteralRelocationKind + ELF::R_RISCV_64);
}

const MCFixupKindInfo &RISCXAsmBackend::getFixupKindInfo(MCFixupKind Kind) const {
  const static MCFixupKindInfo Infos[] = {
      {"fixup_riscx_hi20", 12, 20, 0},
      {"fixup_riscx_lo12_i", 20, 12, 0},
      {"fixup_riscx_12_i", 20, 12, 0},
      {"fixup_riscx_lo12_s", 0, 32, 0},
      {"fixup_riscx_pcrel_hi20", 12, 20, MCFixupKindInfo::FKF_IsPCRel | MCFixupKindInfo::FKF_IsTarget},
      {"fixup_riscx_pcrel_lo12_i", 20, 12, MCFixupKindInfo::FKF_IsPCRel | MCFixupKindInfo::FKF_IsTarget},
      {"fixup_riscx_pcrel_lo12_s", 0, 32, MCFixupKindInfo::FKF_IsPCRel | MCFixupKindInfo::FKF_IsTarget},
      {"fixup_riscx_got_hi20", 12, 20, MCFixupKindInfo::FKF_IsPCRel},
      {"fixup_riscx_jal", 12, 20, MCFixupKindInfo::FKF_IsPCRel},
      {"fixup_riscx_branch", 0, 32, MCFixupKindInfo::FKF_IsPCRel},
      {"fixup_riscx_call", 0, 64, MCFixupKindInfo::FKF_IsPCRel},
      {"fixup_riscx_relax", 0, 0, 0},
      {"fixup_riscx_align", 0, 0, 0},
  };

  if (Kind >= FirstLiteralRelocationKind)
    return MCAsmBackend::getFixupKindInfo(FK_NONE);

  if (Kind < FirstTargetFixupKind)
    return MCAsmBackend::getFixupKindInfo(Kind);
  return Infos[Kind - FirstTargetFixupKind];
}

bool RISCXAsmBackend::writeNopData(raw_ostream &OS, uint64_t Count, const MCSubtargetInfo *STI) const {
  if (Count % 2) {
    OS.write("\0", 1);
    Count -= 1;
  }

  if (Count % 4 == 2) {
    OS.write("\0\0", 2);
    Count -= 2;
  }

  for (; Count >= 4; Count -= 4)
    OS.write("\x13\0\0\0", 4);

  return true;
}

static uint64_t adjustFixupValue(const MCFixup &Fixup, uint64_t Value, MCContext &Ctx) {
  switch (Fixup.getTargetKind()) {
  default:
    llvm_unreachable("Unknown fixup kind!");
  case RISCX::fixup_riscx_got_hi20:
    llvm_unreachable("Relocation should be unconditionally forced\n");
  case FK_Data_1:
  case FK_Data_2:
  case FK_Data_4:
  case FK_Data_8:
  case FK_Data_leb128:
    return Value;
  case RISCX::fixup_riscx_lo12_i:
  case RISCX::fixup_riscx_pcrel_lo12_i:
    return Value & 0xfff;
  case RISCX::fixup_riscx_12_i:
    if (!isInt<12>(Value)) {
      Ctx.reportError(Fixup.getLoc(), "operand must be a constant 12-bit integer");
    }
    return Value & 0xfff;
  case RISCX::fixup_riscx_lo12_s:
  case RISCX::fixup_riscx_pcrel_lo12_s:
    return (((Value >> 5) & 0x7f) << 25) | ((Value & 0x1f) << 7);
  case RISCX::fixup_riscx_hi20:
  case RISCX::fixup_riscx_pcrel_hi20:
    return ((Value + 0x800) >> 12) & 0xfffff;
  case RISCX::fixup_riscx_jal: {
    if (!isInt<21>(Value))
      Ctx.reportError(Fixup.getLoc(), "fixup value out of range");
    if (Value & 0x1)
      Ctx.reportError(Fixup.getLoc(), "fixup value must be 2-byte aligned");
    unsigned Sbit = (Value >> 20) & 0x1;
    unsigned Hi8 = (Value >> 12) & 0xff;
    unsigned Mid1 = (Value >> 11) & 0x1;
    unsigned Lo10 = (Value >> 1) & 0x3ff;
    Value = (Sbit << 19) | (Lo10 << 9) | (Mid1 << 8) | Hi8;
    return Value;
  }
  case RISCX::fixup_riscx_branch: {
    if (!isInt<13>(Value))
      Ctx.reportError(Fixup.getLoc(), "fixup value out of range");
    if (Value & 0x1)
      Ctx.reportError(Fixup.getLoc(), "fixup value must be 2-byte aligned");
    unsigned Sbit = (Value >> 12) & 0x1;
    unsigned Hi1 = (Value >> 11) & 0x1;
    unsigned Mid6 = (Value >> 5) & 0x3f;
    unsigned Lo4 = (Value >> 1) & 0xf;
    Value = (Sbit << 31) | (Mid6 << 25) | (Lo4 << 8) | (Hi1 << 7);
    return Value;
  }
  case RISCX::fixup_riscx_call: {
    uint64_t UpperImm = (Value + 0x800ULL) & 0xfffff000ULL;
    uint64_t LowerImm = Value & 0xfffULL;
    return UpperImm | ((LowerImm << 20) << 32);
  }
  }
}

bool RISCXAsmBackend::evaluateTargetFixup(const MCAssembler &Asm, const MCAsmLayout &Layout, const MCFixup &Fixup,
                                          const MCFragment *DF, const MCValue &Target, const MCSubtargetInfo *STI,
                                          uint64_t &Value, bool &WasForced) {
  const MCFixup *AUIPCFixup;
  const MCFragment *AUIPCDF;
  MCValue AUIPCTarget;
  switch (Fixup.getTargetKind()) {
  default:
    llvm_unreachable("Unexpected fixup kind!");
  case RISCX::fixup_riscx_pcrel_hi20:
    AUIPCFixup = &Fixup;
    AUIPCDF = DF;
    AUIPCTarget = Target;
    break;
  case RISCX::fixup_riscx_pcrel_lo12_i:
  case RISCX::fixup_riscx_pcrel_lo12_s: {
    AUIPCFixup = cast<RISCXMCExpr>(Fixup.getValue())->getPCRelHiFixup(&AUIPCDF);
    if (!AUIPCFixup) {
      Asm.getContext().reportError(Fixup.getLoc(), "could not find corresponding %pcrel_hi");
      return true;
    }

    const MCExpr *AUIPCExpr = AUIPCFixup->getValue();
    if (!AUIPCExpr->evaluateAsRelocatable(AUIPCTarget, &Layout, AUIPCFixup))
      return true;
    break;
  }
  }

  if (!AUIPCTarget.getSymA() || AUIPCTarget.getSymB())
    return false;

  const MCSymbolRefExpr *A = AUIPCTarget.getSymA();
  const MCSymbol &SA = A->getSymbol();
  if (A->getKind() != MCSymbolRefExpr::VK_None || SA.isUndefined())
    return false;

  auto *Writer = Asm.getWriterPtr();
  if (!Writer)
    return false;

  bool IsResolved = Writer->isSymbolRefDifferenceFullyResolvedImpl(Asm, SA, *AUIPCDF, false, true);
  if (!IsResolved)
    return false;

  Value = Layout.getSymbolOffset(SA) + AUIPCTarget.getConstant();
  Value -= Layout.getFragmentOffset(AUIPCDF) + AUIPCFixup->getOffset();
  return true;
}

void RISCXAsmBackend::applyFixup(const MCAssembler &Asm, const MCFixup &Fixup,
                                 const MCValue &Target,
                                 MutableArrayRef<char> Data, uint64_t Value,
                                 bool IsResolved,
                                 const MCSubtargetInfo *STI) const {
  MCFixupKind Kind = Fixup.getKind();
  if (Kind >= FirstLiteralRelocationKind)
    return;
  MCContext &Ctx = Asm.getContext();
  MCFixupKindInfo Info = getFixupKindInfo(Kind);
  if (!Value)
    return;

  Value = adjustFixupValue(Fixup, Value, Ctx);

  Value <<= Info.TargetOffset;

  unsigned Offset = Fixup.getOffset();
  unsigned NumBytes = alignTo(Info.TargetSize + Info.TargetOffset, 8) / 8;

  for (unsigned i = 0; i != NumBytes; ++i) {
    Data[Offset + i] |= uint8_t((Value >> (i * 8)) & 0xff);
  }
}

std::unique_ptr<MCObjectTargetWriter> RISCXAsmBackend::createObjectTargetWriter() const {
  return createRISCXELFObjectWriter(OSABI);
}

MCAsmBackend *llvm::createRISCXAsmBackend(const Target &T,
                                          const MCSubtargetInfo &STI,
                                          const MCRegisterInfo &MRI,
                                          const MCTargetOptions &Options) {
  const Triple &TT = STI.getTargetTriple();
  uint8_t OSABI = MCELFObjectTargetWriter::getOSABI(TT.getOS());
  return new RISCXAsmBackend(STI, OSABI, Options);
}