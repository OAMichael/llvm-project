#include "RISCXInstrInfo.h"
#include "MCTargetDesc/RISCXMatInt.h"
#include "RISCX.h"
#include "RISCXSubtarget.h"
#include "RISCXTargetMachine.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/Analysis/MemoryLocation.h"
#include "llvm/CodeGen/LiveIntervals.h"
#include "llvm/CodeGen/LiveVariables.h"
#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/CodeGen/MachineFrameInfo.h"
#include "llvm/CodeGen/MachineCombinerPattern.h"
#include "llvm/CodeGen/MachineFunctionPass.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"
#include "llvm/CodeGen/MachineRegisterInfo.h"
#include "llvm/CodeGen/MachineTraceMetrics.h"
#include "llvm/MC/MCInstBuilder.h"
#include "llvm/MC/TargetRegistry.h"
#include "llvm/Support/ErrorHandling.h"

using namespace llvm;

#define GET_INSTRINFO_CTOR_DTOR
#define GET_INSTRINFO_NAMED_OPS
#include "RISCXGenInstrInfo.inc"

RISCXInstrInfo::RISCXInstrInfo(RISCXSubtarget &STI)
    : RISCXGenInstrInfo(RISCX::ADJCALLSTACKDOWN, RISCX::ADJCALLSTACKUP),
      STI(STI) {}

MCInst RISCXInstrInfo::getNop() const {
  return MCInstBuilder(RISCX::ADDI)
      .addReg(RISCX::X0)
      .addReg(RISCX::X0)
      .addImm(0);
}

unsigned RISCXInstrInfo::isLoadFromStackSlot(const MachineInstr &MI, int &FrameIndex) const {
  unsigned Dummy;
  return isLoadFromStackSlot(MI, FrameIndex, Dummy);
}

unsigned RISCXInstrInfo::isLoadFromStackSlot(const MachineInstr &MI, int &FrameIndex, unsigned &MemBytes) const {
  switch (MI.getOpcode()) {
  default:
    return 0;
  case RISCX::LB:
  case RISCX::LBU:
    MemBytes = 1;
    break;
  case RISCX::LH:
  case RISCX::LHU:
    MemBytes = 2;
    break;
  case RISCX::LW:
  case RISCX::LWU:
    MemBytes = 4;
    break;
  case RISCX::LD:
    MemBytes = 8;
    break;
  }

  if (MI.getOperand(1).isFI() && MI.getOperand(2).isImm() &&
      MI.getOperand(2).getImm() == 0) {
    FrameIndex = MI.getOperand(1).getIndex();
    return MI.getOperand(0).getReg();
  }
  return 0;
}

unsigned RISCXInstrInfo::isStoreToStackSlot(const MachineInstr &MI, int &FrameIndex) const {
  unsigned Dummy;
  return isStoreToStackSlot(MI, FrameIndex, Dummy);
}

unsigned RISCXInstrInfo::isStoreToStackSlot(const MachineInstr &MI, int &FrameIndex, unsigned &MemBytes) const {
  switch (MI.getOpcode()) {
  default:
    return 0;
  case RISCX::SB:
    MemBytes = 1;
    break;
  case RISCX::SH:
    MemBytes = 2;
    break;
  case RISCX::SW:
    MemBytes = 4;
    break;
  case RISCX::SD:
    MemBytes = 8;
    break;
  }

  if (MI.getOperand(1).isFI() && MI.getOperand(2).isImm() &&
      MI.getOperand(2).getImm() == 0) {
    FrameIndex = MI.getOperand(1).getIndex();
    return MI.getOperand(0).getReg();
  }
  return 0;
}

void RISCXInstrInfo::copyPhysReg(MachineBasicBlock &MBB,
                                 MachineBasicBlock::iterator MBBI,
                                 const DebugLoc &DL, MCRegister DstReg,
                                 MCRegister SrcReg, bool KillSrc) const {
  if (RISCX::GPRRegClass.contains(DstReg, SrcReg)) {
    BuildMI(MBB, MBBI, DL, get(RISCX::ADDI), DstReg)
        .addReg(SrcReg, getKillRegState(KillSrc))
        .addImm(0);
    return;
  }
}

void RISCXInstrInfo::storeRegToStackSlot(MachineBasicBlock &MBB,
                                         MachineBasicBlock::iterator I,
                                         Register SrcReg, bool IsKill, int FI,
                                         const TargetRegisterClass *RC,
                                         const TargetRegisterInfo *TRI,
                                         Register VReg) const {
  MachineFunction *MF = MBB.getParent();
  MachineFrameInfo &MFI = MF->getFrameInfo();
  MachineMemOperand *MMO = MF->getMachineMemOperand(MachinePointerInfo::getFixedStack(*MF, FI), MachineMemOperand::MOStore,
                                                    MFI.getObjectSize(FI), MFI.getObjectAlign(FI));

  BuildMI(MBB, I, DebugLoc(), get(RISCX::SD))
      .addReg(SrcReg, getKillRegState(IsKill))
      .addFrameIndex(FI)
      .addImm(0)
      .addMemOperand(MMO);
}

void RISCXInstrInfo::loadRegFromStackSlot(MachineBasicBlock &MBB,
                                          MachineBasicBlock::iterator I,
                                          Register DstReg, int FI,
                                          const TargetRegisterClass *RC,
                                          const TargetRegisterInfo *TRI,
                                          Register VReg) const {
  MachineFunction *MF = MBB.getParent();
  MachineFrameInfo &MFI = MF->getFrameInfo();
  MachineMemOperand *MMO = MF->getMachineMemOperand(MachinePointerInfo::getFixedStack(*MF, FI), MachineMemOperand::MOLoad,
                                                    MFI.getObjectSize(FI), MFI.getObjectAlign(FI));

  BuildMI(MBB, I, DebugLoc(), get(RISCX::LD), DstReg)
      .addFrameIndex(FI)
      .addImm(0)
      .addMemOperand(MMO);
}

void RISCXInstrInfo::movImm(MachineBasicBlock &MBB,
                            MachineBasicBlock::iterator MBBI,
                            const DebugLoc &DL, Register DstReg, uint64_t Val,
                            MachineInstr::MIFlag Flag, bool DstRenamable,
                            bool DstIsDead) const {
  Register SrcReg = RISCX::X0;

  RISCXMatInt::InstSeq Seq = RISCXMatInt::generateInstSeq(Val, STI);

  bool SrcRenamable = false;
  unsigned Num = 0;

  for (const RISCXMatInt::Inst &Inst : Seq) {
    bool LastItem = ++Num == Seq.size();
    unsigned DstRegState = getDeadRegState(DstIsDead && LastItem) | getRenamableRegState(DstRenamable);
    unsigned SrcRegState = getKillRegState(SrcReg != RISCX::X0) | getRenamableRegState(SrcRenamable);
    switch (Inst.getOpndKind()) {
    case RISCXMatInt::Imm:
      BuildMI(MBB, MBBI, DL, get(Inst.getOpcode()))
          .addReg(DstReg, RegState::Define | DstRegState)
          .addImm(Inst.getImm())
          .setMIFlag(Flag);
      break;
    case RISCXMatInt::RegImm:
      BuildMI(MBB, MBBI, DL, get(Inst.getOpcode()))
          .addReg(DstReg, RegState::Define | DstRegState)
          .addReg(SrcReg, SrcRegState)
          .addImm(Inst.getImm())
          .setMIFlag(Flag);
      break;
    }

    SrcReg = DstReg;
    SrcRenamable = DstRenamable;
  }
}

static RISCXCC::CondCode getCondFromBranchOpc(unsigned Opc) {
  switch (Opc) {
  default:
    return RISCXCC::COND_INVALID;
  case RISCX::BEQ:
    return RISCXCC::COND_EQ;
  case RISCX::BNE:
    return RISCXCC::COND_NE;
  case RISCX::BLT:
    return RISCXCC::COND_LT;
  case RISCX::BGE:
    return RISCXCC::COND_GE;
  case RISCX::BLTU:
    return RISCXCC::COND_LTU;
  case RISCX::BGEU:
    return RISCXCC::COND_GEU;
  }
}

static void parseCondBranch(MachineInstr &LastInst, MachineBasicBlock *&Target, SmallVectorImpl<MachineOperand> &Cond) {
  Target = LastInst.getOperand(2).getMBB();
  unsigned CC = getCondFromBranchOpc(LastInst.getOpcode());
  Cond.push_back(MachineOperand::CreateImm(CC));
  Cond.push_back(LastInst.getOperand(0));
  Cond.push_back(LastInst.getOperand(1));
}

unsigned RISCXCC::getBrCond(RISCXCC::CondCode CC) {
  switch (CC) {
  default:
    llvm_unreachable("Unknown condition code!");
  case RISCXCC::COND_EQ:
    return RISCX::BEQ;
  case RISCXCC::COND_NE:
    return RISCX::BNE;
  case RISCXCC::COND_LT:
    return RISCX::BLT;
  case RISCXCC::COND_GE:
    return RISCX::BGE;
  case RISCXCC::COND_LTU:
    return RISCX::BLTU;
  case RISCXCC::COND_GEU:
    return RISCX::BGEU;
  }
}

const MCInstrDesc &RISCXInstrInfo::getBrCond(RISCXCC::CondCode CC) const {
  return get(RISCXCC::getBrCond(CC));
}

RISCXCC::CondCode RISCXCC::getOppositeBranchCondition(RISCXCC::CondCode CC) {
  switch (CC) {
  default:
    llvm_unreachable("Unrecognized conditional branch");
  case RISCXCC::COND_EQ:
    return RISCXCC::COND_NE;
  case RISCXCC::COND_NE:
    return RISCXCC::COND_EQ;
  case RISCXCC::COND_LT:
    return RISCXCC::COND_GE;
  case RISCXCC::COND_GE:
    return RISCXCC::COND_LT;
  case RISCXCC::COND_LTU:
    return RISCXCC::COND_GEU;
  case RISCXCC::COND_GEU:
    return RISCXCC::COND_LTU;
  }
}

bool RISCXInstrInfo::analyzeBranch(MachineBasicBlock &MBB,
                                   MachineBasicBlock *&TBB,
                                   MachineBasicBlock *&FBB,
                                   SmallVectorImpl<MachineOperand> &Cond,
                                   bool AllowModify) const {
  TBB = FBB = nullptr;
  Cond.clear();

  MachineBasicBlock::iterator I = MBB.getLastNonDebugInstr();
  if (I == MBB.end() || !isUnpredicatedTerminator(*I))
    return false;

  MachineBasicBlock::iterator FirstUncondOrIndirectBr = MBB.end();
  int NumTerminators = 0;
  for (auto J = I.getReverse(); J != MBB.rend() && isUnpredicatedTerminator(*J); J++) {
    NumTerminators++;
    if (J->getDesc().isUnconditionalBranch() || J->getDesc().isIndirectBranch()) {
      FirstUncondOrIndirectBr = J.getReverse();
    }
  }

  if (AllowModify && FirstUncondOrIndirectBr != MBB.end()) {
    while (std::next(FirstUncondOrIndirectBr) != MBB.end()) {
      std::next(FirstUncondOrIndirectBr)->eraseFromParent();
      NumTerminators--;
    }
    I = FirstUncondOrIndirectBr;
  }

  if (I->getDesc().isIndirectBranch())
    return true;

  if (I->isPreISelOpcode())
    return true;

  if (NumTerminators > 2)
    return true;

  if (NumTerminators == 1 && I->getDesc().isUnconditionalBranch()) {
    TBB = getBranchDestBlock(*I);
    return false;
  }

  if (NumTerminators == 1 && I->getDesc().isConditionalBranch()) {
    parseCondBranch(*I, TBB, Cond);
    return false;
  }

  if (NumTerminators == 2 && std::prev(I)->getDesc().isConditionalBranch() && I->getDesc().isUnconditionalBranch()) {
    parseCondBranch(*std::prev(I), TBB, Cond);
    FBB = getBranchDestBlock(*I);
    return false;
  }

  return true;
}

unsigned RISCXInstrInfo::removeBranch(MachineBasicBlock &MBB, int *BytesRemoved) const {
  if (BytesRemoved)
    *BytesRemoved = 0;
  MachineBasicBlock::iterator I = MBB.getLastNonDebugInstr();
  if (I == MBB.end())
    return 0;

  if (!I->getDesc().isUnconditionalBranch() && !I->getDesc().isConditionalBranch())
    return 0;

  if (BytesRemoved)
    *BytesRemoved += getInstSizeInBytes(*I);
  I->eraseFromParent();

  I = MBB.end();

  if (I == MBB.begin())
    return 1;
  --I;
  if (!I->getDesc().isConditionalBranch())
    return 1;

  if (BytesRemoved)
    *BytesRemoved += getInstSizeInBytes(*I);
  I->eraseFromParent();
  return 2;
}

unsigned RISCXInstrInfo::insertBranch(MachineBasicBlock &MBB, MachineBasicBlock *TBB, MachineBasicBlock *FBB, 
                                      ArrayRef<MachineOperand> Cond, const DebugLoc &DL, int *BytesAdded) const {
  if (BytesAdded)
    *BytesAdded = 0;

  if (Cond.empty()) {
    MachineInstr &MI = *BuildMI(&MBB, DL, get(RISCX::PseudoBR)).addMBB(TBB);
    if (BytesAdded)
      *BytesAdded += getInstSizeInBytes(MI);
    return 1;
  }

  auto CC = static_cast<RISCXCC::CondCode>(Cond[0].getImm());
  MachineInstr &CondMI = *BuildMI(&MBB, DL, getBrCond(CC)).add(Cond[1]).add(Cond[2]).addMBB(TBB);
  if (BytesAdded)
    *BytesAdded += getInstSizeInBytes(CondMI);

  if (!FBB)
    return 1;

  MachineInstr &MI = *BuildMI(&MBB, DL, get(RISCX::PseudoBR)).addMBB(FBB);
  if (BytesAdded)
    *BytesAdded += getInstSizeInBytes(MI);
  return 2;
}

bool RISCXInstrInfo::reverseBranchCondition(SmallVectorImpl<MachineOperand> &Cond) const {
  auto CC = static_cast<RISCXCC::CondCode>(Cond[0].getImm());
  Cond[0].setImm(getOppositeBranchCondition(CC));
  return false;
}

MachineBasicBlock *RISCXInstrInfo::getBranchDestBlock(const MachineInstr &MI) const {
  int NumOp = MI.getNumExplicitOperands();
  return MI.getOperand(NumOp - 1).getMBB();
}

unsigned RISCXInstrInfo::getInstSizeInBytes(const MachineInstr &MI) const {
  if (MI.isMetaInstruction())
    return 0;

  unsigned Opcode = MI.getOpcode();
  if (Opcode == TargetOpcode::INLINEASM || Opcode == TargetOpcode::INLINEASM_BR) {
    const MachineFunction &MF = *MI.getParent()->getParent();
    const auto &TM = static_cast<const RISCXTargetMachine &>(MF.getTarget());
    return getInlineAsmLength(MI.getOperand(0).getSymbolName(), *TM.getMCAsmInfo());
  }
  return get(Opcode).getSize();
}

std::optional<DestSourcePair> RISCXInstrInfo::isCopyInstrImpl(const MachineInstr &MI) const {
  if (MI.isMoveReg())
    return DestSourcePair{MI.getOperand(0), MI.getOperand(1)};
  switch (MI.getOpcode()) {
  default:
    break;
  case RISCX::ADDI:
    if (MI.getOperand(1).isReg() && MI.getOperand(2).isImm() && MI.getOperand(2).getImm() == 0)
      return DestSourcePair{MI.getOperand(0), MI.getOperand(1)};
    break;
  }
  return std::nullopt;
}

bool RISCXInstrInfo::isAssociativeAndCommutative(const MachineInstr &Inst, bool Invert) const {
  unsigned Opc = Inst.getOpcode();
  if (Invert) {
    auto InverseOpcode = getInverseOpcode(Opc);
    if (!InverseOpcode)
      return false;
    Opc = *InverseOpcode;
  }

  switch (Opc) {
  default:
    return false;
  case RISCX::ADD:
  case RISCX::ADDW:
  case RISCX::AND:
  case RISCX::OR:
  case RISCX::XOR:
  case RISCX::MUL:
  case RISCX::MULW:
    return true;
  }

  return false;
}

std::optional<unsigned> RISCXInstrInfo::getInverseOpcode(unsigned Opcode) const {
  switch (Opcode) {
  default:
    return std::nullopt;
  case RISCX::ADD:
    return RISCX::SUB;
  case RISCX::SUB:
    return RISCX::ADD;
  case RISCX::ADDW:
    return RISCX::SUBW;
  case RISCX::SUBW:
    return RISCX::ADDW;
  }
}

MachineInstr *RISCXInstrInfo::emitLdStWithAddr(MachineInstr &MemI, const ExtAddrMode &AM) const {
  const DebugLoc &DL = MemI.getDebugLoc();
  MachineBasicBlock &MBB = *MemI.getParent();

  return BuildMI(MBB, MemI, DL, get(MemI.getOpcode()))
      .addReg(MemI.getOperand(0).getReg(), MemI.mayLoad() ? RegState::Define : 0)
      .addReg(AM.BaseReg)
      .addImm(AM.Displacement)
      .setMemRefs(MemI.memoperands())
      .setMIFlags(MemI.getFlags());
}
