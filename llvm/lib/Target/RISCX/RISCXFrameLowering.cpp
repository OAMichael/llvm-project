#include "RISCXFrameLowering.h"
#include "RISCXSubtarget.h"
#include "llvm/CodeGen/MachineFrameInfo.h"
#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"
#include "llvm/CodeGen/MachineRegisterInfo.h"
#include "llvm/CodeGen/RegisterScavenging.h"
#include "llvm/IR/DiagnosticInfo.h"
#include <algorithm>

using namespace llvm;


RISCXFrameLowering::RISCXFrameLowering(const RISCXSubtarget &STI)
    : TargetFrameLowering(StackGrowsDown, /*StackAl=*/Align(16), /*LocalAreaOffset=*/0, /*TransientStackAlignment=*/Align(16)), STI(STI) {}

bool RISCXFrameLowering::hasFP(const MachineFunction &MF) const {
  const TargetRegisterInfo *RegInfo = MF.getSubtarget().getRegisterInfo();

  const MachineFrameInfo &MFI = MF.getFrameInfo();
  return MF.getTarget().Options.DisableFramePointerElim(MF) ||
         RegInfo->hasStackRealignment(MF) || MFI.hasVarSizedObjects() ||
         MFI.isFrameAddressTaken();
}

bool RISCXFrameLowering::hasBP(const MachineFunction &MF) const {
  const MachineFrameInfo &MFI = MF.getFrameInfo();
  const TargetRegisterInfo *TRI = STI.getRegisterInfo();

  return (MFI.hasVarSizedObjects() || (!hasReservedCallFrame(MF) && (!MFI.isMaxCallFrameSizeComputed() || MFI.getMaxCallFrameSize() != 0))) && TRI->hasStackRealignment(MF);
}

void RISCXFrameLowering::determineFrameLayout(MachineFunction &MF) const {
  MachineFrameInfo &MFI = MF.getFrameInfo();
  uint64_t FrameSize = MFI.getStackSize();
  Align StackAlign = getStackAlign();
  FrameSize = alignTo(FrameSize, StackAlign);
  MFI.setStackSize(FrameSize);
}

static Register getFPReg() { return RISCX::X8; }
static Register getSPReg() { return RISCX::X2; }
static Register getBPReg() { return RISCX::X9; }

void RISCXFrameLowering::emitPrologue(MachineFunction &MF, MachineBasicBlock &MBB) const {
  MachineFrameInfo &MFI = MF.getFrameInfo();
  const RISCXRegisterInfo *RI = STI.getRegisterInfo();
  MachineBasicBlock::iterator MBBI = MBB.begin();

  Register FPReg = getFPReg();
  Register SPReg = getSPReg();
  Register BPReg = getBPReg();

  DebugLoc DL;

  while (MBBI != MBB.end() && MBBI->getFlag(MachineInstr::FrameSetup))
    ++MBBI;

  determineFrameLayout(MF);

  uint64_t StackSize = MFI.getStackSize();
  uint64_t RealStackSize = StackSize;

  if (RealStackSize == 0 && !MFI.adjustsStack())
    return;

  uint64_t FirstSPAdjustAmount = getFirstSPAdjustAmount(MF);
  if (FirstSPAdjustAmount) {
    StackSize = FirstSPAdjustAmount;
    RealStackSize = FirstSPAdjustAmount;
  }

  if (StackSize != 0) {
    RI->adjustReg(MBB, MBBI, DL, SPReg, SPReg, StackOffset::getFixed(-StackSize), MachineInstr::FrameSetup, getStackAlign());
  }

  const auto &CSI = MFI.getCalleeSavedInfo();
  std::advance(MBBI, CSI.size());

  if (hasFP(MF)) {
    RI->adjustReg(MBB, MBBI, DL, FPReg, SPReg, StackOffset::getFixed(RealStackSize), MachineInstr::FrameSetup, getStackAlign());
  }

  if (FirstSPAdjustAmount) {
    uint64_t SecondSPAdjustAmount = MFI.getStackSize() - FirstSPAdjustAmount;
    RI->adjustReg(MBB, MBBI, DL, SPReg, SPReg, StackOffset::getFixed(-SecondSPAdjustAmount), MachineInstr::FrameSetup, getStackAlign());
  }

  if (hasFP(MF)) {
    const RISCXRegisterInfo *RI = STI.getRegisterInfo();
    if (RI->hasStackRealignment(MF)) {
      Align MaxAlignment = MFI.getMaxAlign();

      const RISCXInstrInfo *TII = STI.getInstrInfo();
      if (isInt<12>(-(int)MaxAlignment.value())) {
        BuildMI(MBB, MBBI, DL, TII->get(RISCX::ANDI), SPReg)
            .addReg(SPReg)
            .addImm(-(int)MaxAlignment.value())
            .setMIFlag(MachineInstr::FrameSetup);
      } else {
        unsigned ShiftAmount = Log2(MaxAlignment);
        Register VR = MF.getRegInfo().createVirtualRegister(&RISCX::GPRRegClass);
        BuildMI(MBB, MBBI, DL, TII->get(RISCX::SRLI), VR)
            .addReg(SPReg)
            .addImm(ShiftAmount)
            .setMIFlag(MachineInstr::FrameSetup);
        BuildMI(MBB, MBBI, DL, TII->get(RISCX::SLLI), SPReg)
            .addReg(VR)
            .addImm(ShiftAmount)
            .setMIFlag(MachineInstr::FrameSetup);
      }

      if (hasBP(MF)) {
        BuildMI(MBB, MBBI, DL, TII->get(RISCX::ADDI), BPReg)
            .addReg(SPReg)
            .addImm(0)
            .setMIFlag(MachineInstr::FrameSetup);
      }
    }
  }
}

void RISCXFrameLowering::emitEpilogue(MachineFunction &MF, MachineBasicBlock &MBB) const {
  const RISCXRegisterInfo *RI = STI.getRegisterInfo();
  MachineFrameInfo &MFI = MF.getFrameInfo();
  Register FPReg = getFPReg();
  Register SPReg = getSPReg();

  MachineBasicBlock::iterator MBBI = MBB.end();
  DebugLoc DL;
  if (!MBB.empty()) {
    MBBI = MBB.getLastNonDebugInstr();
    if (MBBI != MBB.end())
      DL = MBBI->getDebugLoc();

    MBBI = MBB.getFirstTerminator();

    while (MBBI != MBB.begin() && std::prev(MBBI)->getFlag(MachineInstr::FrameDestroy))
      --MBBI;
  }

  auto LastFrameDestroy = MBBI;
  const auto &CSI = MFI.getCalleeSavedInfo();
  if (!CSI.empty())
    LastFrameDestroy = std::prev(MBBI, CSI.size());

  uint64_t StackSize = MFI.getStackSize();
  uint64_t RealStackSize = StackSize;
  uint64_t FPOffset = RealStackSize;

  if (RI->hasStackRealignment(MF) || MFI.hasVarSizedObjects() || !hasReservedCallFrame(MF)) {
    RI->adjustReg(MBB, LastFrameDestroy, DL, SPReg, FPReg, StackOffset::getFixed(-FPOffset), MachineInstr::FrameDestroy, getStackAlign());
  }

  uint64_t FirstSPAdjustAmount = getFirstSPAdjustAmount(MF);
  if (FirstSPAdjustAmount) {
    uint64_t SecondSPAdjustAmount = MFI.getStackSize() - FirstSPAdjustAmount;
    RI->adjustReg(MBB, LastFrameDestroy, DL, SPReg, SPReg, StackOffset::getFixed(SecondSPAdjustAmount), MachineInstr::FrameDestroy, getStackAlign());
  }

  if (FirstSPAdjustAmount)
    StackSize = FirstSPAdjustAmount;

  if (StackSize != 0) {
    RI->adjustReg(MBB, MBBI, DL, SPReg, SPReg, StackOffset::getFixed(StackSize), MachineInstr::FrameDestroy, getStackAlign());
  }
}

StackOffset RISCXFrameLowering::getFrameIndexReference(const MachineFunction &MF, int FI, Register &FrameReg) const {
  const MachineFrameInfo &MFI = MF.getFrameInfo();
  const TargetRegisterInfo *RI = MF.getSubtarget().getRegisterInfo();

  const auto &CSI = MFI.getCalleeSavedInfo();
  int MinCSFI = 0;
  int MaxCSFI = -1;
  StackOffset Offset;

  Offset = StackOffset::getFixed(MFI.getObjectOffset(FI) - getOffsetOfLocalArea() + MFI.getOffsetAdjustment());
  uint64_t FirstSPAdjustAmount = getFirstSPAdjustAmount(MF);

  if (CSI.size()) {
    MinCSFI = CSI[0].getFrameIdx();
    MaxCSFI = CSI[CSI.size() - 1].getFrameIdx();
  }

  if (FI >= MinCSFI && FI <= MaxCSFI) {
    FrameReg = RISCX::X2;

    if (FirstSPAdjustAmount)
      Offset += StackOffset::getFixed(FirstSPAdjustAmount);
    else
      Offset += StackOffset::getFixed(MFI.getStackSize());
    return Offset;
  }

  if (RI->hasStackRealignment(MF) && !MFI.isFixedObjectIndex(FI)) {
    if (hasBP(MF)) {
      FrameReg = getBPReg();
    } else {
      FrameReg = RISCX::X2;
    }
  } else {
    FrameReg = RI->getFrameRegister(MF);
  }

  if (FrameReg == getFPReg()) {
    return Offset;
  }

  if (MFI.getStackID(FI) == TargetStackID::Default) {
    if (MFI.isFixedObjectIndex(FI)) {
      Offset += StackOffset::get(MFI.getStackSize(), 0);
    } else {
      Offset += StackOffset::getFixed(MFI.getStackSize());
    }
  }
  return Offset;
}

void RISCXFrameLowering::determineCalleeSaves(MachineFunction &MF, BitVector &SavedRegs, RegScavenger *RS) const {
  TargetFrameLowering::determineCalleeSaves(MF, SavedRegs, RS);
  if (hasFP(MF)) {
    SavedRegs.set(RISCX::X1);
    SavedRegs.set(RISCX::X8);
  }
  if (hasBP(MF))
    SavedRegs.set(getBPReg());
}

static unsigned estimateFunctionSizeInBytes(const MachineFunction &MF, const RISCXInstrInfo &TII) {
  unsigned FnSize = 0;
  for (auto &MBB : MF) {
    for (auto &MI : MBB) {
      if (MI.isConditionalBranch())
        FnSize += TII.getInstSizeInBytes(MI);
      if (MI.isConditionalBranch() || MI.isUnconditionalBranch()) {
        FnSize += 4 + 8 + 4 + 4;
        continue;
      }

      FnSize += TII.getInstSizeInBytes(MI);
    }
  }
  return FnSize;
}

void RISCXFrameLowering::processFunctionBeforeFrameFinalized(MachineFunction &MF, RegScavenger *RS) const {
  const RISCXRegisterInfo *RegInfo = MF.getSubtarget<RISCXSubtarget>().getRegisterInfo();
  const RISCXInstrInfo *TII = MF.getSubtarget<RISCXSubtarget>().getInstrInfo();
  MachineFrameInfo &MFI = MF.getFrameInfo();
  const TargetRegisterClass *RC = &RISCX::GPRRegClass;

  unsigned ScavSlotsNum = 0;
  if (!isInt<11>(MFI.estimateStackSize(MF)))
    ScavSlotsNum = 1;

  bool IsLargeFunction = !isInt<20>(estimateFunctionSizeInBytes(MF, *TII));
  if (IsLargeFunction)
    ScavSlotsNum = std::max(ScavSlotsNum, 1u);

  for (unsigned I = 0; I < ScavSlotsNum; I++) {
    int FI = MFI.CreateStackObject(RegInfo->getSpillSize(*RC), RegInfo->getSpillAlign(*RC), false);
    RS->addScavengingFrameIndex(FI);
  }
}

bool RISCXFrameLowering::hasReservedCallFrame(const MachineFunction &MF) const {
  return !MF.getFrameInfo().hasVarSizedObjects();
}

MachineBasicBlock::iterator RISCXFrameLowering::eliminateCallFramePseudoInstr(MachineFunction &MF, MachineBasicBlock &MBB, MachineBasicBlock::iterator MI) const {
  Register SPReg = RISCX::X2;
  DebugLoc DL = MI->getDebugLoc();

  if (!hasReservedCallFrame(MF)) {
    int64_t Amount = MI->getOperand(0).getImm();

    if (Amount != 0) {
      Amount = alignSPAdjust(Amount);

      if (MI->getOpcode() == RISCX::ADJCALLSTACKDOWN)
        Amount = -Amount;

      const RISCXRegisterInfo &RI = *STI.getRegisterInfo();
      RI.adjustReg(MBB, MI, DL, SPReg, SPReg, StackOffset::getFixed(Amount), MachineInstr::NoFlags, getStackAlign());
    }
  }

  return MBB.erase(MI);
}

uint64_t RISCXFrameLowering::getFirstSPAdjustAmount(const MachineFunction &MF) const {
  const MachineFrameInfo &MFI = MF.getFrameInfo();
  const std::vector<CalleeSavedInfo> &CSI = MFI.getCalleeSavedInfo();
  uint64_t StackSize = MFI.getStackSize();

  if (!isInt<12>(StackSize) && (CSI.size() > 0)) {
    const uint64_t StackAlign = getStackAlign().value();
    return 2048 - StackAlign;
  }
  return 0;
}

bool RISCXFrameLowering::spillCalleeSavedRegisters(MachineBasicBlock &MBB, MachineBasicBlock::iterator MI,
                                                   ArrayRef<CalleeSavedInfo> CSI, const TargetRegisterInfo *TRI) const {
  if (CSI.empty())
    return true;
  
  MachineFunction *MF = MBB.getParent();
  MachineFrameInfo &MFI = MF->getFrameInfo();
  const TargetInstrInfo &TII = *MF->getSubtarget().getInstrInfo();
  const RISCXRegisterInfo *RegInfo = MF->getSubtarget<RISCXSubtarget>().getRegisterInfo();
  const TargetRegisterClass *RC = &RISCX::GPRRegClass;

  // Need non-const
  auto &CSIs = MFI.getCalleeSavedInfo();
  for (auto &CS : CSIs) {
    int FI = MFI.CreateStackObject(RegInfo->getSpillSize(*RC), RegInfo->getSpillAlign(*RC), true);
    CS.setFrameIdx(FI);

    Register Reg = CS.getReg();
    const TargetRegisterClass *RC = TRI->getMinimalPhysRegClass(Reg);
    TII.storeRegToStackSlot(MBB, MI, Reg, !MBB.isLiveIn(Reg), CS.getFrameIdx(), RC, TRI, Register());
  }
  return true;
}

bool RISCXFrameLowering::restoreCalleeSavedRegisters(MachineBasicBlock &MBB, MachineBasicBlock::iterator MI,
                                                     MutableArrayRef<CalleeSavedInfo> CSI, const TargetRegisterInfo *TRI) const {
  if (CSI.empty())
    return true;

  MachineFunction *MF = MBB.getParent();
  const TargetInstrInfo &TII = *MF->getSubtarget().getInstrInfo();
  for (auto &CS : CSI) {
    Register Reg = CS.getReg();
    const TargetRegisterClass *RC = TRI->getMinimalPhysRegClass(Reg);
    TII.loadRegFromStackSlot(MBB, MI, Reg, CS.getFrameIdx(), RC, TRI, Register());
  }
  return true;
}