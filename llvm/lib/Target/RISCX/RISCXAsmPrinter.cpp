#include "MCTargetDesc/RISCXBaseInfo.h"
#include "MCTargetDesc/RISCXInstPrinter.h"
#include "MCTargetDesc/RISCXMCExpr.h"
#include "MCTargetDesc/RISCXTargetStreamer.h"
#include "RISCX.h"
#include "RISCXTargetMachine.h"
#include "TargetInfo/RISCXTargetInfo.h"
#include "llvm/ADT/APInt.h"
#include "llvm/BinaryFormat/ELF.h"
#include "llvm/CodeGen/AsmPrinter.h"
#include "llvm/CodeGen/MachineConstantPool.h"
#include "llvm/CodeGen/MachineFunctionPass.h"
#include "llvm/CodeGen/MachineInstr.h"
#include "llvm/CodeGen/MachineModuleInfo.h"
#include "llvm/MC/MCAsmInfo.h"
#include "llvm/MC/MCContext.h"
#include "llvm/MC/MCInst.h"
#include "llvm/MC/MCInstBuilder.h"
#include "llvm/MC/MCObjectFileInfo.h"
#include "llvm/MC/MCSectionELF.h"
#include "llvm/MC/MCStreamer.h"
#include "llvm/MC/MCSymbol.h"
#include "llvm/MC/TargetRegistry.h"
#include "llvm/Support/raw_ostream.h"

using namespace llvm;

namespace {
class RISCXAsmPrinter : public AsmPrinter {
  const RISCXSubtarget *STI;

public:
  explicit RISCXAsmPrinter(TargetMachine &TM, std::unique_ptr<MCStreamer> Streamer) : AsmPrinter(TM, std::move(Streamer)) {}

  StringRef getPassName() const override { return "RISC-X Assembly Printer"; }

  bool runOnMachineFunction(MachineFunction &MF) override;

  void emitInstruction(const MachineInstr *MI) override;

  bool PrintAsmOperand(const MachineInstr *MI, unsigned OpNo,
                       const char *ExtraCode, raw_ostream &OS) override;
  bool PrintAsmMemoryOperand(const MachineInstr *MI, unsigned OpNo,
                             const char *ExtraCode, raw_ostream &OS) override;

  bool emitPseudoExpansionLowering(MCStreamer &OutStreamer, const MachineInstr *MI);

  bool lowerOperand(const MachineOperand &MO, MCOperand &MCOp) const;

private:

  bool lowerToMCInst(const MachineInstr *MI, MCInst &OutMI);
};
}


#include "RISCXGenMCPseudoLowering.inc"

void RISCXAsmPrinter::emitInstruction(const MachineInstr *MI) {
  if (emitPseudoExpansionLowering(*OutStreamer, MI))
    return;

  MCInst OutInst;
  if (!lowerToMCInst(MI, OutInst))
    EmitToStreamer(*OutStreamer, OutInst);
}

bool RISCXAsmPrinter::PrintAsmOperand(const MachineInstr *MI, unsigned OpNo, const char *ExtraCode, raw_ostream &OS) {
  if (!AsmPrinter::PrintAsmOperand(MI, OpNo, ExtraCode, OS))
    return false;

  const MachineOperand &MO = MI->getOperand(OpNo);
  if (ExtraCode && ExtraCode[0]) {
    if (ExtraCode[1] != 0)
      return true;

    switch (ExtraCode[0]) {
    default:
      return true;
    case 'z':
      if (MO.isImm() && MO.getImm() == 0) {
        OS << RISCXInstPrinter::getRegisterName(RISCX::X0);
        return false;
      }
      break;
    case 'i':
      if (!MO.isReg())
        OS << 'i';
      return false;
    }
  }

  switch (MO.getType()) {
  case MachineOperand::MO_Immediate:
    OS << MO.getImm();
    return false;
  case MachineOperand::MO_Register:
    OS << RISCXInstPrinter::getRegisterName(MO.getReg());
    return false;
  case MachineOperand::MO_GlobalAddress:
    PrintSymbolOperand(MO, OS);
    return false;
  case MachineOperand::MO_BlockAddress: {
    MCSymbol *Sym = GetBlockAddressSymbol(MO.getBlockAddress());
    Sym->print(OS, MAI);
    return false;
  }
  default:
    break;
  }

  return true;
}

bool RISCXAsmPrinter::PrintAsmMemoryOperand(const MachineInstr *MI,
                                            unsigned OpNo,
                                            const char *ExtraCode,
                                            raw_ostream &OS) {
  if (ExtraCode)
    return AsmPrinter::PrintAsmMemoryOperand(MI, OpNo, ExtraCode, OS);

  const MachineOperand &AddrReg = MI->getOperand(OpNo);
  const MachineOperand &Offset = MI->getOperand(OpNo + 1);
  if (!AddrReg.isReg())
    return true;
  if (!Offset.isImm() && !Offset.isGlobal() && !Offset.isBlockAddress() &&
      !Offset.isMCSymbol())
    return true;

  MCOperand MCO;
  if (!lowerOperand(Offset, MCO))
    return true;

  if (Offset.isImm())
    OS << MCO.getImm();
  else if (Offset.isGlobal() || Offset.isBlockAddress() || Offset.isMCSymbol())
    OS << *MCO.getExpr();
  OS << "(" << RISCXInstPrinter::getRegisterName(AddrReg.getReg()) << ")";
  return false;
}

bool RISCXAsmPrinter::runOnMachineFunction(MachineFunction &MF) {
  STI = &MF.getSubtarget<RISCXSubtarget>();

  SetupMachineFunction(MF);
  emitFunctionBody();

  return false;
}


extern "C" LLVM_EXTERNAL_VISIBILITY void LLVMInitializeRISCXAsmPrinter() {
  RegisterAsmPrinter<RISCXAsmPrinter> Y(getTheRISCXTarget());
}

static MCOperand lowerSymbolOperand(const MachineOperand &MO, MCSymbol *Sym, const AsmPrinter &AP) {
  MCContext &Ctx = AP.OutContext;
  RISCXMCExpr::VariantKind Kind;

  switch (MO.getTargetFlags()) {
  default:
    llvm_unreachable("Unknown target flag on GV operand");
  case RISCXII::MO_None:
    Kind = RISCXMCExpr::VK_RISCX_None;
    break;
  case RISCXII::MO_CALL:
    Kind = RISCXMCExpr::VK_RISCX_CALL;
    break;
  case RISCXII::MO_LO:
    Kind = RISCXMCExpr::VK_RISCX_LO;
    break;
  case RISCXII::MO_HI:
    Kind = RISCXMCExpr::VK_RISCX_HI;
    break;
  case RISCXII::MO_PCREL_LO:
    Kind = RISCXMCExpr::VK_RISCX_PCREL_LO;
    break;
  case RISCXII::MO_PCREL_HI:
    Kind = RISCXMCExpr::VK_RISCX_PCREL_HI;
    break;
  case RISCXII::MO_GOT_HI:
    Kind = RISCXMCExpr::VK_RISCX_GOT_HI;
    break;
  }

  const MCExpr *ME = MCSymbolRefExpr::create(Sym, MCSymbolRefExpr::VK_None, Ctx);

  if (!MO.isJTI() && !MO.isMBB() && MO.getOffset())
    ME = MCBinaryExpr::createAdd(ME, MCConstantExpr::create(MO.getOffset(), Ctx), Ctx);

  if (Kind != RISCXMCExpr::VK_RISCX_None)
    ME = RISCXMCExpr::create(ME, Kind, Ctx);
  return MCOperand::createExpr(ME);
}

bool RISCXAsmPrinter::lowerOperand(const MachineOperand &MO, MCOperand &MCOp) const {
  switch (MO.getType()) {
  default:
    report_fatal_error("lowerOperand: unknown operand type");
  case MachineOperand::MO_Register:
    if (MO.isImplicit())
      return false;
    MCOp = MCOperand::createReg(MO.getReg());
    break;
  case MachineOperand::MO_RegisterMask:
    return false;
  case MachineOperand::MO_Immediate:
    MCOp = MCOperand::createImm(MO.getImm());
    break;
  case MachineOperand::MO_MachineBasicBlock:
    MCOp = lowerSymbolOperand(MO, MO.getMBB()->getSymbol(), *this);
    break;
  case MachineOperand::MO_GlobalAddress:
    MCOp = lowerSymbolOperand(MO, getSymbolPreferLocal(*MO.getGlobal()), *this);
    break;
  case MachineOperand::MO_BlockAddress:
    MCOp = lowerSymbolOperand(MO, GetBlockAddressSymbol(MO.getBlockAddress()), *this);
    break;
  case MachineOperand::MO_ExternalSymbol:
    MCOp = lowerSymbolOperand(MO, GetExternalSymbolSymbol(MO.getSymbolName()), *this);
    break;
  case MachineOperand::MO_ConstantPoolIndex:
    MCOp = lowerSymbolOperand(MO, GetCPISymbol(MO.getIndex()), *this);
    break;
  case MachineOperand::MO_JumpTableIndex:
    MCOp = lowerSymbolOperand(MO, GetJTISymbol(MO.getIndex()), *this);
    break;
  case MachineOperand::MO_MCSymbol:
    MCOp = lowerSymbolOperand(MO, MO.getMCSymbol(), *this);
    break;
  }
  return true;
}

bool RISCXAsmPrinter::lowerToMCInst(const MachineInstr *MI, MCInst &OutMI) {
  OutMI.setOpcode(MI->getOpcode());

  for (const MachineOperand &MO : MI->operands()) {
    MCOperand MCOp;
    if (lowerOperand(MO, MCOp))
      OutMI.addOperand(MCOp);
  }
  return false;
}