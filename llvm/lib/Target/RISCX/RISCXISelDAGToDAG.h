#ifndef LLVM_LIB_TARGET_RISCX_RISCXISELDAGTODAG_H
#define LLVM_LIB_TARGET_RISCX_RISCXISELDAGTODAG_H

#include "RISCX.h"
#include "RISCXTargetMachine.h"
#include "llvm/CodeGen/SelectionDAGISel.h"
#include "llvm/Support/KnownBits.h"

namespace llvm {
class RISCXDAGToDAGISel : public SelectionDAGISel {
  const RISCXSubtarget *Subtarget = nullptr;

public:
  static char ID;

  RISCXDAGToDAGISel() = delete;

  explicit RISCXDAGToDAGISel(RISCXTargetMachine &TargetMachine, CodeGenOptLevel OptLevel) : SelectionDAGISel(ID, TargetMachine, OptLevel) {}

  bool runOnMachineFunction(MachineFunction &MF) override {
    Subtarget = &MF.getSubtarget<RISCXSubtarget>();
    return SelectionDAGISel::runOnMachineFunction(MF);
  }

  void Select(SDNode *Node) override;

  bool SelectAddrFrameIndex(SDValue Addr, SDValue &Base, SDValue &Offset);
  bool SelectAddrRegImm(SDValue Addr, SDValue &Base, SDValue &Offset, bool IsINX = false);

  bool tryShrinkShlLogicImm(SDNode *Node);

  bool selectShiftMask(SDValue N, unsigned ShiftWidth, SDValue &ShAmt);
  bool selectShiftMaskXLen(SDValue N, SDValue &ShAmt) { return selectShiftMask(N, Subtarget->getXLen(), ShAmt); }
  bool selectShiftMask32(SDValue N, SDValue &ShAmt) { return selectShiftMask(N, 32, ShAmt); }

  bool selectSETCC(SDValue N, ISD::CondCode ExpectedCCVal, SDValue &Val);
  bool selectSETNE(SDValue N, SDValue &Val) { return selectSETCC(N, ISD::SETNE, Val); }
  bool selectSETEQ(SDValue N, SDValue &Val) { return selectSETCC(N, ISD::SETEQ, Val); }

  bool selectSExtBits(SDValue N, unsigned Bits, SDValue &Val);
  template <unsigned Bits> bool selectSExtBits(SDValue N, SDValue &Val) {
    return selectSExtBits(N, Bits, Val);
  }
  bool selectZExtBits(SDValue N, unsigned Bits, SDValue &Val);
  template <unsigned Bits> bool selectZExtBits(SDValue N, SDValue &Val) {
    return selectZExtBits(N, Bits, Val);
  }

  bool hasAllNBitUsers(SDNode *Node, unsigned Bits, const unsigned Depth = 0) const;
  bool hasAllHUsers(SDNode *Node) const { return hasAllNBitUsers(Node, 16); }
  bool hasAllWUsers(SDNode *Node) const { return hasAllNBitUsers(Node, 32); }

#include "RISCXGenDAGISel.inc"
};
} // namespace llvm

#endif  // LLVM_LIB_TARGET_RISCX_RISCXISELDAGTODAG_H