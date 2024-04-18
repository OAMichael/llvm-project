#ifndef LLVM_LIB_TARGET_RISCX_RISCXISELLOWERING_H
#define LLVM_LIB_TARGET_RISCX_RISCXISELLOWERING_H

#include "RISCX.h"
#include "llvm/CodeGen/CallingConvLower.h"
#include "llvm/CodeGen/SelectionDAG.h"
#include "llvm/CodeGen/TargetLowering.h"
#include <optional>

namespace llvm {
class InstructionCost;
class RISCXSubtarget;
struct RISCXRegisterInfo;

namespace RISCXISD {
enum NodeType : unsigned {
  FIRST_NUMBER = ISD::BUILTIN_OP_END,
  RET_GLUE,
  CALL,

  SELECT_CC,
  BR_CC,
  ADD_LO,
  HI,

  MULHSU,
  SLLW,
  SRAW,
  SRLW,
  DIVW,
  DIVUW,
  REMUW,
};
} // namespace RISCXISD

class RISCXTargetLowering : public TargetLowering {
  const RISCXSubtarget &Subtarget;

public:
  explicit RISCXTargetLowering(const TargetMachine &TM, const RISCXSubtarget &STI);

  const RISCXSubtarget &getSubtarget() const { return Subtarget; }

  bool isLegalAddressingMode(const DataLayout &DL, const AddrMode &AM, Type *Ty, unsigned AS, Instruction *I = nullptr) const override;
  bool isLegalICmpImmediate(int64_t Imm) const override;
  bool isLegalAddImmediate(int64_t Imm) const override;

  SDValue LowerOperation(SDValue Op, SelectionDAG &DAG) const override;
  void ReplaceNodeResults(SDNode *N, SmallVectorImpl<SDValue> &Results, SelectionDAG &DAG) const override;

  void computeKnownBitsForTargetNode(const SDValue Op,
                                     KnownBits &Known,
                                     const APInt &DemandedElts,
                                     const SelectionDAG &DAG,
                                     unsigned Depth) const override;
  unsigned ComputeNumSignBitsForTargetNode(SDValue Op,
                                           const APInt &DemandedElts,
                                           const SelectionDAG &DAG,
                                           unsigned Depth) const override;

  const Constant *getTargetConstantFromLoad(LoadSDNode *LD) const override;

  const char *getTargetNodeName(unsigned Opcode) const override;

  SDValue LowerFormalArguments(SDValue Chain, CallingConv::ID CallConv,
                               bool IsVarArg,
                               const SmallVectorImpl<ISD::InputArg> &Ins,
                               const SDLoc &DL, SelectionDAG &DAG,
                               SmallVectorImpl<SDValue> &InVals) const override;
  bool CanLowerReturn(CallingConv::ID CallConv, MachineFunction &MF,
                      bool IsVarArg,
                      const SmallVectorImpl<ISD::OutputArg> &Outs,
                      LLVMContext &Context) const override;
  SDValue LowerReturn(SDValue Chain, CallingConv::ID CallConv, bool IsVarArg,
                      const SmallVectorImpl<ISD::OutputArg> &Outs,
                      const SmallVectorImpl<SDValue> &OutVals, const SDLoc &DL,
                      SelectionDAG &DAG) const override;
  SDValue LowerCall(TargetLowering::CallLoweringInfo &CLI, SmallVectorImpl<SDValue> &InVals) const override;

  typedef bool RISCXCCAssignFn(const DataLayout &DL, RISCXABI::ABI,
                               unsigned ValNo, MVT ValVT, MVT LocVT,
                               CCValAssign::LocInfo LocInfo,
                               ISD::ArgFlagsTy ArgFlags, CCState &State,
                               bool IsFixed, bool IsRet, Type *OrigTy,
                               const RISCXTargetLowering &TLI,
                               std::optional<unsigned> FirstMaskArgument);

private:
  void analyzeInputArgs(MachineFunction &MF, CCState &CCInfo,
                        const SmallVectorImpl<ISD::InputArg> &Ins, bool IsRet,
                        RISCXCCAssignFn Fn) const;
  void analyzeOutputArgs(MachineFunction &MF, CCState &CCInfo,
                         const SmallVectorImpl<ISD::OutputArg> &Outs,
                         bool IsRet, CallLoweringInfo *CLI,
                         RISCXCCAssignFn Fn) const;

  template <class NodeTy>
  SDValue getAddr(NodeTy *N, SelectionDAG &DAG, bool IsLocal = true, bool IsExternWeak = false) const;

  SDValue lowerGlobalAddress(SDValue Op, SelectionDAG &DAG) const;
  SDValue lowerBlockAddress(SDValue Op, SelectionDAG &DAG) const;
  SDValue lowerConstantPool(SDValue Op, SelectionDAG &DAG) const;
  SDValue lowerJumpTable(SDValue Op, SelectionDAG &DAG) const;
  SDValue lowerSELECT(SDValue Op, SelectionDAG &DAG) const;
  SDValue lowerBRCOND(SDValue Op, SelectionDAG &DAG) const;
  SDValue lowerFRAMEADDR(SDValue Op, SelectionDAG &DAG) const;
  SDValue lowerRETURNADDR(SDValue Op, SelectionDAG &DAG) const;
  SDValue lowerShiftLeftParts(SDValue Op, SelectionDAG &DAG) const;
  SDValue lowerShiftRightParts(SDValue Op, SelectionDAG &DAG, bool IsSRA) const;
};

namespace RISCX {

bool CC_RISCX(const DataLayout &DL, RISCXABI::ABI ABI, unsigned ValNo,
              MVT ValVT, MVT LocVT, CCValAssign::LocInfo LocInfo,
              ISD::ArgFlagsTy ArgFlags, CCState &State, bool IsFixed,
              bool IsRet, Type *OrigTy, const RISCXTargetLowering &TLI,
              std::optional<unsigned> FirstMaskArgument);

ArrayRef<MCPhysReg> getArgGPRs(const RISCXABI::ABI ABI);

} // end namespace RISCX
} // end namespace llvm

#endif  // LLVM_LIB_TARGET_RISCX_RISCXISELLOWERING_H