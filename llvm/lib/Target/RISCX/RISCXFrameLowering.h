#ifndef LLVM_LIB_TARGET_RISCX_RISCXFRAMELOWERING_H
#define LLVM_LIB_TARGET_RISCX_RISCXFRAMELOWERING_H

#include "llvm/CodeGen/TargetFrameLowering.h"

namespace llvm {
class RISCXSubtarget;

class RISCXFrameLowering : public TargetFrameLowering {
public:
  explicit RISCXFrameLowering(const RISCXSubtarget &STI);

  void emitPrologue(MachineFunction &MF, MachineBasicBlock &MBB) const override;
  void emitEpilogue(MachineFunction &MF, MachineBasicBlock &MBB) const override;

  StackOffset getFrameIndexReference(const MachineFunction &MF, int FI, Register &FrameReg) const override;

  void determineCalleeSaves(MachineFunction &MF, BitVector &SavedRegs, RegScavenger *RS) const override;

  void processFunctionBeforeFrameFinalized(MachineFunction &MF, RegScavenger *RS) const override;

  bool hasFP(const MachineFunction &MF) const override;

  bool hasBP(const MachineFunction &MF) const;

  bool hasReservedCallFrame(const MachineFunction &MF) const override;
  MachineBasicBlock::iterator eliminateCallFramePseudoInstr(MachineFunction &MF, MachineBasicBlock &MBB, MachineBasicBlock::iterator MI) const override;

  bool spillCalleeSavedRegisters(MachineBasicBlock &MBB,
                                 MachineBasicBlock::iterator MI,
                                 ArrayRef<CalleeSavedInfo> CSI,
                                 const TargetRegisterInfo *TRI) const override;

  bool restoreCalleeSavedRegisters(MachineBasicBlock &MBB,
                                   MachineBasicBlock::iterator MI,
                                   MutableArrayRef<CalleeSavedInfo> CSI,
                                   const TargetRegisterInfo *TRI) const override;

  uint64_t getFirstSPAdjustAmount(const MachineFunction &MF) const;

protected:
  const RISCXSubtarget &STI;

private:
  void determineFrameLayout(MachineFunction &MF) const;
};
} // namespace llvm
#endif  // LLVM_LIB_TARGET_RISCX_RISCXFRAMELOWERING_H