#ifndef LLVM_LIB_TARGET_RISCX_RISCXREGISTERINFO_H
#define LLVM_LIB_TARGET_RISCX_RISCXREGISTERINFO_H

#include "llvm/CodeGen/TargetRegisterInfo.h"

#define GET_REGINFO_HEADER
#include "RISCXGenRegisterInfo.inc"

namespace llvm {

struct RISCXRegisterInfo : public RISCXGenRegisterInfo {

  RISCXRegisterInfo(unsigned HwMode);

  const MCPhysReg *getCalleeSavedRegs(const MachineFunction *MF) const override;

  const uint32_t *getCallPreservedMask(const MachineFunction &MF, CallingConv::ID) const override;

  BitVector getReservedRegs(const MachineFunction &MF) const override;

  void adjustReg(MachineBasicBlock &MBB, MachineBasicBlock::iterator II,
                 const DebugLoc &DL, Register DestReg, Register SrcReg,
                 StackOffset Offset, MachineInstr::MIFlag Flag,
                 MaybeAlign RequiredAlign) const;

  bool eliminateFrameIndex(MachineBasicBlock::iterator MI, int SPAdj, unsigned FIOperandNum, RegScavenger *RS = nullptr) const override;

  Register getFrameRegister(const MachineFunction &MF) const override;
};
}

#endif  // LLVM_LIB_TARGET_RISCX_RISCXREGISTERINFO_H