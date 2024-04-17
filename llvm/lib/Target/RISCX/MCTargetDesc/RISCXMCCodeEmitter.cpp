#include "MCTargetDesc/RISCXBaseInfo.h"
#include "MCTargetDesc/RISCXFixupKinds.h"
#include "MCTargetDesc/RISCXMCExpr.h"
#include "MCTargetDesc/RISCXMCTargetDesc.h"
#include "llvm/MC/MCAsmInfo.h"
#include "llvm/MC/MCCodeEmitter.h"
#include "llvm/MC/MCContext.h"
#include "llvm/MC/MCExpr.h"
#include "llvm/MC/MCInst.h"
#include "llvm/MC/MCInstBuilder.h"
#include "llvm/MC/MCInstrInfo.h"
#include "llvm/MC/MCRegisterInfo.h"
#include "llvm/MC/MCSubtargetInfo.h"
#include "llvm/MC/MCSymbol.h"
#include "llvm/Support/Casting.h"
#include "llvm/Support/EndianStream.h"

using namespace llvm;

namespace {
class RISCXMCCodeEmitter : public MCCodeEmitter {
  RISCXMCCodeEmitter(const RISCXMCCodeEmitter &) = delete;
  void operator=(const RISCXMCCodeEmitter &) = delete;
  MCContext &Ctx;
  MCInstrInfo const &MCII;

public:
  RISCXMCCodeEmitter(MCContext &ctx, MCInstrInfo const &MCII)
      : Ctx(ctx), MCII(MCII) {}

  ~RISCXMCCodeEmitter() override = default;

  void encodeInstruction(const MCInst &MI, SmallVectorImpl<char> &CB,
                         SmallVectorImpl<MCFixup> &Fixups,
                         const MCSubtargetInfo &STI) const override;

  void expandFunctionCall(const MCInst &MI, SmallVectorImpl<char> &CB,
                          SmallVectorImpl<MCFixup> &Fixups,
                          const MCSubtargetInfo &STI) const;

  uint64_t getBinaryCodeForInstr(const MCInst &MI,
                                 SmallVectorImpl<MCFixup> &Fixups,
                                 const MCSubtargetInfo &STI) const;

  unsigned getMachineOpValue(const MCInst &MI, const MCOperand &MO,
                             SmallVectorImpl<MCFixup> &Fixups,
                             const MCSubtargetInfo &STI) const;

  unsigned getImmOpValueAsr1(const MCInst &MI, unsigned OpNo,
                             SmallVectorImpl<MCFixup> &Fixups,
                             const MCSubtargetInfo &STI) const;

  unsigned getImmOpValue(const MCInst &MI, unsigned OpNo,
                         SmallVectorImpl<MCFixup> &Fixups,
                         const MCSubtargetInfo &STI) const;
};
} // end namespace

MCCodeEmitter *llvm::createRISCXMCCodeEmitter(const MCInstrInfo &MCII, MCContext &Ctx) {
  return new RISCXMCCodeEmitter(Ctx, MCII);
}

void RISCXMCCodeEmitter::expandFunctionCall(const MCInst &MI,
                                            SmallVectorImpl<char> &CB,
                                            SmallVectorImpl<MCFixup> &Fixups,
                                            const MCSubtargetInfo &STI) const {
  MCInst TmpInst{};
  uint32_t Binary{};
  unsigned Opcode = MI.getOpcode();

  // Handle SIM_...
  switch (Opcode) {
  default:
    break;
  case RISCX::PseudoSimPutPixel:
    Opcode = RISCX::SIM_PUT_PIXEL;
    break;
  case RISCX::PseudoSimFlush:
    Opcode = RISCX::SIM_FLUSH;
    break;
  case RISCX::PseudoSimRand:
    Opcode = RISCX::SIM_RAND;
    break;
  }

  if (Opcode == RISCX::SIM_PUT_PIXEL || Opcode == RISCX::SIM_FLUSH || Opcode == RISCX::SIM_RAND) {
    TmpInst = MCInstBuilder(Opcode);
    Binary = getBinaryCodeForInstr(TmpInst, Fixups, STI);
    support::endian::write(CB, Binary, llvm::endianness::little);
    return;
  }

  MCOperand Func;
  MCRegister Ra;
  if (Opcode == RISCX::PseudoCALL) {
    Func = MI.getOperand(0);
    Ra = RISCX::X1;
  } else if (Opcode == RISCX::PseudoJump) {
    Func = MI.getOperand(1);
    Ra = MI.getOperand(0).getReg();
  }

  const MCExpr *CallExpr = Func.getExpr();

  TmpInst = MCInstBuilder(RISCX::AUIPC).addReg(Ra).addExpr(CallExpr);
  Binary = getBinaryCodeForInstr(TmpInst, Fixups, STI);
  support::endian::write(CB, Binary, llvm::endianness::little);

  if (Opcode == RISCX::PseudoJump)
    TmpInst = MCInstBuilder(RISCX::JALR).addReg(RISCX::X0).addReg(Ra).addImm(0);
  else
    TmpInst = MCInstBuilder(RISCX::JALR).addReg(Ra).addReg(Ra).addImm(0);
  Binary = getBinaryCodeForInstr(TmpInst, Fixups, STI);
  support::endian::write(CB, Binary, llvm::endianness::little);
}

void RISCXMCCodeEmitter::encodeInstruction(const MCInst &MI,
                                           SmallVectorImpl<char> &CB,
                                           SmallVectorImpl<MCFixup> &Fixups,
                                           const MCSubtargetInfo &STI) const {
  switch (MI.getOpcode()) {
  default:
    break;
  case RISCX::PseudoCALL:
  case RISCX::PseudoJump:
  case RISCX::PseudoSimPutPixel:
  case RISCX::PseudoSimFlush:
  case RISCX::PseudoSimRand:
    expandFunctionCall(MI, CB, Fixups, STI);
    return;
  }

  uint32_t Bits = getBinaryCodeForInstr(MI, Fixups, STI);
  support::endian::write(CB, Bits, llvm::endianness::little);
}

unsigned RISCXMCCodeEmitter::getMachineOpValue(const MCInst &MI, const MCOperand &MO, SmallVectorImpl<MCFixup> &Fixups, const MCSubtargetInfo &STI) const {
  if (MO.isReg())
    return Ctx.getRegisterInfo()->getEncodingValue(MO.getReg());

  if (MO.isImm())
    return static_cast<unsigned>(MO.getImm());

  llvm_unreachable("Unhandled expression!");
  return 0;
}

unsigned RISCXMCCodeEmitter::getImmOpValueAsr1(const MCInst &MI, unsigned OpNo, SmallVectorImpl<MCFixup> &Fixups, const MCSubtargetInfo &STI) const {
  const MCOperand &MO = MI.getOperand(OpNo);

  if (MO.isImm()) {
    unsigned Res = MO.getImm();
    return Res >> 1;
  }

  return getImmOpValue(MI, OpNo, Fixups, STI);
}

unsigned RISCXMCCodeEmitter::getImmOpValue(const MCInst &MI, unsigned OpNo, SmallVectorImpl<MCFixup> &Fixups, const MCSubtargetInfo &STI) const {
  const MCOperand &MO = MI.getOperand(OpNo);

  MCInstrDesc const &Desc = MCII.get(MI.getOpcode());
  unsigned MIFrm = RISCXII::getFormat(Desc.TSFlags);

  if (MO.isImm())
    return MO.getImm();

  const MCExpr *Expr = MO.getExpr();
  MCExpr::ExprKind Kind = Expr->getKind();
  RISCX::Fixups FixupKind = RISCX::fixup_riscx_invalid;
  if (Kind == MCExpr::Target) {
    const RISCXMCExpr *RXExpr = cast<RISCXMCExpr>(Expr);

    switch (RXExpr->getKind()) {
    case RISCXMCExpr::VK_RISCX_None:
    case RISCXMCExpr::VK_RISCX_Invalid:
    case RISCXMCExpr::VK_RISCX_32_PCREL:
      llvm_unreachable("Unhandled fixup kind!");
    case RISCXMCExpr::VK_RISCX_LO:
      if (MIFrm == RISCXII::InstFormatI)
        FixupKind = RISCX::fixup_riscx_lo12_i;
      else if (MIFrm == RISCXII::InstFormatS)
        FixupKind = RISCX::fixup_riscx_lo12_s;
      else
        llvm_unreachable("Unhandled fixup kind!");
      break;
    case RISCXMCExpr::VK_RISCX_HI:
      FixupKind = RISCX::fixup_riscx_hi20;
      break;
    case RISCXMCExpr::VK_RISCX_PCREL_LO:
      if (MIFrm == RISCXII::InstFormatI)
        FixupKind = RISCX::fixup_riscx_pcrel_lo12_i;
      else if (MIFrm == RISCXII::InstFormatS)
        FixupKind = RISCX::fixup_riscx_pcrel_lo12_s;
      else
        llvm_unreachable("Unhandled fixup kind!");
      break;
    case RISCXMCExpr::VK_RISCX_PCREL_HI:
      FixupKind = RISCX::fixup_riscx_pcrel_hi20;
      break;
    case RISCXMCExpr::VK_RISCX_GOT_HI:
      FixupKind = RISCX::fixup_riscx_got_hi20;
      break;
    case RISCXMCExpr::VK_RISCX_CALL:
      FixupKind = RISCX::fixup_riscx_call;
      break;
    }
  } else if ((Kind == MCExpr::SymbolRef && cast<MCSymbolRefExpr>(Expr)->getKind() == MCSymbolRefExpr::VK_None) || Kind == MCExpr::Binary) {
    if (MIFrm == RISCXII::InstFormatJ) {
      FixupKind = RISCX::fixup_riscx_jal;
    } else if (MIFrm == RISCXII::InstFormatB) {
      FixupKind = RISCX::fixup_riscx_branch;
    } else if (MIFrm == RISCXII::InstFormatI) {
      FixupKind = RISCX::fixup_riscx_12_i;
    }
  }

  Fixups.push_back(MCFixup::create(0, Expr, MCFixupKind(FixupKind), MI.getLoc()));
  return 0;
}

#include "RISCXGenMCCodeEmitter.inc"