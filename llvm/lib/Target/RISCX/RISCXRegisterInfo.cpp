#include "RISCXRegisterInfo.h"
#include "RISCX.h"
#include "RISCXSubtarget.h"
#include "llvm/ADT/SmallSet.h"
#include "llvm/CodeGen/MachineFrameInfo.h"
#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"
#include "llvm/CodeGen/RegisterScavenging.h"
#include "llvm/CodeGen/TargetFrameLowering.h"
#include "llvm/CodeGen/TargetInstrInfo.h"
#include "llvm/IR/DebugInfoMetadata.h"

#define GET_REGINFO_TARGET_DESC
#include "RISCXGenRegisterInfo.inc"

using namespace llvm;

RISCXRegisterInfo::RISCXRegisterInfo(unsigned HwMode)
    : RISCXGenRegisterInfo(RISCX::X1, /*DwarfFlavour*/0, /*EHFlavor*/0, /*PC*/0, HwMode) {}

const MCPhysReg *RISCXRegisterInfo::getCalleeSavedRegs(const MachineFunction *MF) const {
  return CSR_RISCX_SaveList;
}

BitVector RISCXRegisterInfo::getReservedRegs(const MachineFunction &MF) const {
  const RISCXFrameLowering *TFI = getFrameLowering(MF);
  BitVector Reserved(getNumRegs());

  markSuperRegs(Reserved, RISCX::X0);
  markSuperRegs(Reserved, RISCX::X2);
  markSuperRegs(Reserved, RISCX::X3);
  markSuperRegs(Reserved, RISCX::X4);
  if (TFI->hasFP(MF))
    markSuperRegs(Reserved, RISCX::X8);
  if (TFI->hasBP(MF))
    markSuperRegs(Reserved, RISCX::X9);
  return Reserved;
}

void RISCXRegisterInfo::adjustReg(MachineBasicBlock &MBB,
                                  MachineBasicBlock::iterator II,
                                  const DebugLoc &DL, Register DestReg,
                                  Register SrcReg, StackOffset Offset,
                                  MachineInstr::MIFlag Flag,
                                  MaybeAlign RequiredAlign) const {

  if (DestReg == SrcReg && !Offset.getFixed() && !Offset.getScalable())
    return;

  MachineFunction &MF = *MBB.getParent();
  MachineRegisterInfo &MRI = MF.getRegInfo();
  const RISCXSubtarget &ST = MF.getSubtarget<RISCXSubtarget>();
  const RISCXInstrInfo *TII = ST.getInstrInfo();

  bool KillSrcReg = false;

  if (Offset.getScalable()) {
    unsigned ScalableAdjOpc = RISCX::ADD;
    int64_t ScalableValue = Offset.getScalable();
    if (ScalableValue < 0) {
      ScalableValue = -ScalableValue;
      ScalableAdjOpc = RISCX::SUB;
    }
    Register ScratchReg = DestReg;
    if (DestReg == SrcReg)
      ScratchReg = MRI.createVirtualRegister(&RISCX::GPRRegClass);
    BuildMI(MBB, II, DL, TII->get(ScalableAdjOpc), DestReg)
      .addReg(SrcReg).addReg(ScratchReg, RegState::Kill)
      .setMIFlag(Flag);
    SrcReg = DestReg;
    KillSrcReg = true;
  }

  int64_t Val = Offset.getFixed();
  if (DestReg == SrcReg && Val == 0)
    return;

  const uint64_t Align = RequiredAlign.valueOrOne().value();

  if (isInt<12>(Val)) {
    BuildMI(MBB, II, DL, TII->get(RISCX::ADDI), DestReg)
        .addReg(SrcReg, getKillRegState(KillSrcReg))
        .addImm(Val)
        .setMIFlag(Flag);
    return;
  }

  int64_t MaxPosAdjStep = 2048 - Align;
  if (Val > -4096 && Val <= (2 * MaxPosAdjStep)) {
    int64_t FirstAdj = Val < 0 ? -2048 : MaxPosAdjStep;
    Val -= FirstAdj;
    BuildMI(MBB, II, DL, TII->get(RISCX::ADDI), DestReg)
        .addReg(SrcReg, getKillRegState(KillSrcReg))
        .addImm(FirstAdj)
        .setMIFlag(Flag);
    BuildMI(MBB, II, DL, TII->get(RISCX::ADDI), DestReg)
        .addReg(DestReg, RegState::Kill)
        .addImm(Val)
        .setMIFlag(Flag);
    return;
  }

  unsigned Opc = RISCX::ADD;
  if (Val < 0) {
    Val = -Val;
    Opc = RISCX::SUB;
  }

  Register ScratchReg = MRI.createVirtualRegister(&RISCX::GPRRegClass);
  TII->movImm(MBB, II, DL, ScratchReg, Val, Flag);
  BuildMI(MBB, II, DL, TII->get(Opc), DestReg)
      .addReg(SrcReg, getKillRegState(KillSrcReg))
      .addReg(ScratchReg, RegState::Kill)
      .setMIFlag(Flag);
}

bool RISCXRegisterInfo::eliminateFrameIndex(MachineBasicBlock::iterator II, int SPAdj, unsigned FIOperandNum, RegScavenger *RS) const {
  MachineInstr &MI = *II;
  MachineFunction &MF = *MI.getParent()->getParent();
  MachineRegisterInfo &MRI = MF.getRegInfo();
  DebugLoc DL = MI.getDebugLoc();

  int FrameIndex = MI.getOperand(FIOperandNum).getIndex();
  Register FrameReg;
  StackOffset Offset = getFrameLowering(MF)->getFrameIndexReference(MF, FrameIndex, FrameReg);
  Offset += StackOffset::getFixed(MI.getOperand(FIOperandNum + 1).getImm());

  if (!isInt<32>(Offset.getFixed())) {
    report_fatal_error("Frame offsets outside of the signed 32-bit range not supported");
  }

  if (MI.getOpcode() == RISCX::ADDI && !isInt<12>(Offset.getFixed())) {
    MI.getOperand(FIOperandNum + 1).ChangeToImmediate(0);
  } else {
    int64_t Val = Offset.getFixed();
    int64_t Lo12 = SignExtend64<12>(Val);
    MI.getOperand(FIOperandNum + 1).ChangeToImmediate(Lo12);
    Offset = StackOffset::get((uint64_t)Val - (uint64_t)Lo12, Offset.getScalable());
  }

  if (Offset.getScalable() || Offset.getFixed()) {
    Register DestReg;
    if (MI.getOpcode() == RISCX::ADDI)
      DestReg = MI.getOperand(0).getReg();
    else
      DestReg = MRI.createVirtualRegister(&RISCX::GPRRegClass);
    adjustReg(*II->getParent(), II, DL, DestReg, FrameReg, Offset, MachineInstr::NoFlags, std::nullopt);
    MI.getOperand(FIOperandNum).ChangeToRegister(DestReg, /*IsDef*/false, /*IsImp*/false, /*IsKill*/true);
  } else {
    MI.getOperand(FIOperandNum).ChangeToRegister(FrameReg, /*IsDef*/false, /*IsImp*/false, /*IsKill*/false);
  }

  if (MI.getOpcode() == RISCX::ADDI &&
      MI.getOperand(0).getReg() == MI.getOperand(1).getReg() &&
      MI.getOperand(2).getImm() == 0) {
    MI.eraseFromParent();
    return true;
  }
  return false;
}

Register RISCXRegisterInfo::getFrameRegister(const MachineFunction &MF) const {
  const TargetFrameLowering *TFI = getFrameLowering(MF);
  return TFI->hasFP(MF) ? RISCX::X8 : RISCX::X2;
}

const uint32_t *RISCXRegisterInfo::getCallPreservedMask(const MachineFunction & MF, CallingConv::ID CC) const {
  return CSR_RISCX_RegMask;
}