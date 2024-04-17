#ifndef LLVM_LIB_TARGET_RISCX_RISCXINSTRINFO_H
#define LLVM_LIB_TARGET_RISCX_RISCXINSTRINFO_H

#include "RISCXRegisterInfo.h"
#include "llvm/CodeGen/TargetInstrInfo.h"
#include "llvm/IR/DiagnosticInfo.h"

#define GET_INSTRINFO_HEADER
#define GET_INSTRINFO_OPERAND_ENUM
#include "RISCXGenInstrInfo.inc"

namespace llvm {

class RISCXSubtarget;

namespace RISCXCC {

enum CondCode {
  COND_EQ,
  COND_NE,
  COND_LT,
  COND_GE,
  COND_LTU,
  COND_GEU,
  COND_INVALID
};

CondCode getOppositeBranchCondition(CondCode);
unsigned getBrCond(CondCode CC);

} // end of namespace RISCXCC

class RISCXInstrInfo : public RISCXGenInstrInfo {

public:
  explicit RISCXInstrInfo(RISCXSubtarget &STI);

  MCInst getNop() const override;
  const MCInstrDesc &getBrCond(RISCXCC::CondCode CC) const;

  unsigned isLoadFromStackSlot(const MachineInstr &MI, int &FrameIndex) const override;
  unsigned isLoadFromStackSlot(const MachineInstr &MI, int &FrameIndex, unsigned &MemBytes) const override;
  unsigned isStoreToStackSlot(const MachineInstr &MI, int &FrameIndex) const override;
  unsigned isStoreToStackSlot(const MachineInstr &MI, int &FrameIndex, unsigned &MemBytes) const override;

  void copyPhysReg(MachineBasicBlock &MBB, MachineBasicBlock::iterator MBBI,
                   const DebugLoc &DL, MCRegister DstReg, MCRegister SrcReg,
                   bool KillSrc) const override;

  void storeRegToStackSlot(MachineBasicBlock &MBB,
                           MachineBasicBlock::iterator MBBI, Register SrcReg,
                           bool IsKill, int FrameIndex,
                           const TargetRegisterClass *RC,
                           const TargetRegisterInfo *TRI,
                           Register VReg) const override;

  void loadRegFromStackSlot(MachineBasicBlock &MBB,
                            MachineBasicBlock::iterator MBBI, Register DstReg,
                            int FrameIndex, const TargetRegisterClass *RC,
                            const TargetRegisterInfo *TRI,
                            Register VReg) const override;

  void movImm(MachineBasicBlock &MBB, MachineBasicBlock::iterator MBBI,
              const DebugLoc &DL, Register DstReg, uint64_t Val,
              MachineInstr::MIFlag Flag = MachineInstr::NoFlags,
              bool DstRenamable = false, bool DstIsDead = false) const;

  unsigned getInstSizeInBytes(const MachineInstr &MI) const override;

  bool analyzeBranch(MachineBasicBlock &MBB, MachineBasicBlock *&TBB,
                     MachineBasicBlock *&FBB,
                     SmallVectorImpl<MachineOperand> &Cond,
                     bool AllowModify) const override;

  unsigned insertBranch(MachineBasicBlock &MBB, MachineBasicBlock *TBB,
                        MachineBasicBlock *FBB, ArrayRef<MachineOperand> Cond,
                        const DebugLoc &dl,
                        int *BytesAdded = nullptr) const override;

  unsigned removeBranch(MachineBasicBlock &MBB, int *BytesRemoved = nullptr) const override;

  bool reverseBranchCondition(SmallVectorImpl<MachineOperand> &Cond) const override;

  MachineBasicBlock *getBranchDestBlock(const MachineInstr &MI) const override;

  std::optional<DestSourcePair> isCopyInstrImpl(const MachineInstr &MI) const override;

  MachineInstr *emitLdStWithAddr(MachineInstr &MemI, const ExtAddrMode &AM) const override;

  bool isAssociativeAndCommutative(const MachineInstr &Inst, bool Invert) const override;

  std::optional<unsigned> getInverseOpcode(unsigned Opcode) const override;

protected:
  const RISCXSubtarget &STI;
};
} // end namespace llvm
#endif  // LLVM_LIB_TARGET_RISCX_RISCXINSTRINFO_H