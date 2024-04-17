#include "RISCXISelLowering.h"
#include "MCTargetDesc/RISCXMatInt.h"
#include "RISCX.h"
#include "RISCXRegisterInfo.h"
#include "RISCXSubtarget.h"
#include "RISCXTargetMachine.h"
#include "llvm/Analysis/MemoryLocation.h"
#include "llvm/Analysis/VectorUtils.h"
#include "llvm/CodeGen/MachineFrameInfo.h"
#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"
#include "llvm/CodeGen/MachineJumpTableInfo.h"
#include "llvm/CodeGen/MachineRegisterInfo.h"
#include "llvm/CodeGen/SelectionDAGAddressAnalysis.h"
#include "llvm/CodeGen/TargetLoweringObjectFileImpl.h"
#include "llvm/CodeGen/ValueTypes.h"
#include "llvm/IR/DiagnosticInfo.h"
#include "llvm/IR/DiagnosticPrinter.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/PatternMatch.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/KnownBits.h"
#include "llvm/Support/MathExtras.h"
#include <optional>

using namespace llvm;

RISCXTargetLowering::RISCXTargetLowering(const TargetMachine &TM, const RISCXSubtarget &STI)
    : TargetLowering(TM), Subtarget(STI) {

  MVT XLenVT = Subtarget.getXLenVT();

  addRegisterClass(XLenVT, &RISCX::GPRRegClass);
  addRegisterClass(MVT::i32, &RISCX::GPRRegClass);

  computeRegisterProperties(STI.getRegisterInfo());

  setStackPointerRegisterToSaveRestore(RISCX::X2);

  setLoadExtAction({ISD::EXTLOAD, ISD::SEXTLOAD, ISD::ZEXTLOAD}, XLenVT, MVT::i1, Promote);
  setLoadExtAction({ISD::EXTLOAD, ISD::SEXTLOAD, ISD::ZEXTLOAD}, MVT::i32, MVT::i1, Promote);

  setOperationAction(ISD::DYNAMIC_STACKALLOC, XLenVT, Expand);

  setOperationAction(ISD::BR_JT, MVT::Other, Expand);
  setOperationAction(ISD::BR_CC, XLenVT, Expand);
  setOperationAction(ISD::BR_CC, MVT::i32, Expand);
  setOperationAction(ISD::BRCOND, MVT::Other, Custom);
  setOperationAction(ISD::SELECT_CC, XLenVT, Expand);
  setOperationAction(ISD::SELECT_CC, MVT::i32, Expand);

  setCondCodeAction(ISD::SETLE, XLenVT, Expand);
  setCondCodeAction(ISD::SETGT, XLenVT, Custom);
  setCondCodeAction(ISD::SETGE, XLenVT, Expand);
  setCondCodeAction(ISD::SETULE, XLenVT, Expand);
  setCondCodeAction(ISD::SETUGT, XLenVT, Custom);
  setCondCodeAction(ISD::SETUGE, XLenVT, Expand);

  setOperationAction(ISD::SETCC, MVT::i32, Promote);

  setOperationAction({ISD::STACKSAVE, ISD::STACKRESTORE}, MVT::Other, Expand);

  setOperationAction(ISD::SIGN_EXTEND_INREG, MVT::i1, Expand);

  setOperationAction(ISD::SIGN_EXTEND_INREG, {MVT::i8, MVT::i16}, Expand);

  setOperationAction(ISD::MUL, MVT::i128, Custom);

  setOperationAction({ISD::MULHS, ISD::MULHU}, MVT::i32, Expand);
  setOperationAction({ISD::SDIVREM, ISD::UDIVREM, ISD::SMUL_LOHI, ISD::UMUL_LOHI}, MVT::i32, Expand);

  setOperationAction({ISD::SDIVREM, ISD::UDIVREM, ISD::SMUL_LOHI, ISD::UMUL_LOHI}, XLenVT, Expand);

  setOperationAction({ISD::SHL_PARTS, ISD::SRL_PARTS, ISD::SRA_PARTS}, XLenVT, Custom);

  setOperationAction(ISD::SELECT, XLenVT, Custom);

  setOperationAction(ISD::SELECT, MVT::i32, Promote);

  setOperationAction({ISD::GlobalAddress, ISD::BlockAddress, ISD::ConstantPool, ISD::JumpTable}, XLenVT, Custom);

  setOperationAction(ISD::Constant, MVT::i64, Custom);

  setBooleanContents(ZeroOrOneBooleanContent);
}

bool RISCXTargetLowering::isLegalAddressingMode(const DataLayout &DL,
                                                const AddrMode &AM, Type *Ty,
                                                unsigned AS,
                                                Instruction *I) const {
  if (AM.BaseGV)
    return false;

  if (!isInt<12>(AM.BaseOffs))
    return false;

  switch (AM.Scale) {
  case 0:
    break;
  case 1:
    if (!AM.HasBaseReg)
      break;
    return false;
  default:
    return false;
  }

  return true;
}

bool RISCXTargetLowering::isLegalICmpImmediate(int64_t Imm) const {
  return isInt<12>(Imm);
}

bool RISCXTargetLowering::isLegalAddImmediate(int64_t Imm) const {
  return isInt<12>(Imm);
}

static void translateSetCCForBranch(const SDLoc &DL, SDValue &LHS, SDValue &RHS,
                                    ISD::CondCode &CC, SelectionDAG &DAG) {
  if (isIntEqualitySetCC(CC) && isNullConstant(RHS) &&
      LHS.getOpcode() == ISD::AND && LHS.hasOneUse() &&
      isa<ConstantSDNode>(LHS.getOperand(1))) {
    uint64_t Mask = LHS.getConstantOperandVal(1);
    if ((isPowerOf2_64(Mask) || isMask_64(Mask)) && !isInt<12>(Mask)) {
      unsigned ShAmt = 0;
      if (isPowerOf2_64(Mask)) {
        CC = CC == ISD::SETEQ ? ISD::SETGE : ISD::SETLT;
        ShAmt = LHS.getValueSizeInBits() - 1 - Log2_64(Mask);
      } else {
        ShAmt = LHS.getValueSizeInBits() - llvm::bit_width(Mask);
      }

      LHS = LHS.getOperand(0);
      if (ShAmt != 0)
        LHS = DAG.getNode(ISD::SHL, DL, LHS.getValueType(), LHS,
                          DAG.getConstant(ShAmt, DL, LHS.getValueType()));
      return;
    }
  }

  if (auto *RHSC = dyn_cast<ConstantSDNode>(RHS)) {
    int64_t C = RHSC->getSExtValue();
    switch (CC) {
    default: break;
    case ISD::SETGT:
      if (C == -1) {
        RHS = DAG.getConstant(0, DL, RHS.getValueType());
        CC = ISD::SETGE;
        return;
      }
      break;
    case ISD::SETLT:
      if (C == 1) {
        RHS = LHS;
        LHS = DAG.getConstant(0, DL, RHS.getValueType());
        CC = ISD::SETGE;
        return;
      }
      break;
    }
  }

  switch (CC) {
  default:
    break;
  case ISD::SETGT:
  case ISD::SETLE:
  case ISD::SETUGT:
  case ISD::SETULE:
    CC = ISD::getSetCCSwappedOperands(CC);
    std::swap(LHS, RHS);
    break;
  }
}

static SDValue lowerConstant(SDValue Op, SelectionDAG &DAG, const RISCXSubtarget &Subtarget) {
  int64_t Imm = cast<ConstantSDNode>(Op)->getSExtValue();

  if (isInt<32>(Imm))
    return Op;

  RISCXMatInt::InstSeq Seq = RISCXMatInt::generateInstSeq(Imm, Subtarget);
  if (Seq.size() <= Subtarget.getMaxBuildIntsCost())
    return Op;

  if (DAG.shouldOptForSize())
    return SDValue();

  unsigned ShiftAmt, AddOpc;
  RISCXMatInt::InstSeq SeqLo =
      RISCXMatInt::generateTwoRegInstSeq(Imm, Subtarget, ShiftAmt, AddOpc);
  if (!SeqLo.empty() && (SeqLo.size() + 2) <= Subtarget.getMaxBuildIntsCost())
    return Op;

  return SDValue();
}

SDValue RISCXTargetLowering::LowerOperation(SDValue Op, SelectionDAG &DAG) const {
  switch (Op.getOpcode()) {
  default:
    report_fatal_error("unimplemented operand");
  case ISD::GlobalAddress:
    return lowerGlobalAddress(Op, DAG);
  case ISD::BlockAddress:
    return lowerBlockAddress(Op, DAG);
  case ISD::ConstantPool:
    return lowerConstantPool(Op, DAG);
  case ISD::JumpTable:
    return lowerJumpTable(Op, DAG);
  case ISD::Constant:
    return lowerConstant(Op, DAG, Subtarget);
  case ISD::SELECT:
    return lowerSELECT(Op, DAG);
  case ISD::BRCOND:
    return lowerBRCOND(Op, DAG);
  case ISD::FRAMEADDR:
    return lowerFRAMEADDR(Op, DAG);
  case ISD::RETURNADDR:
    return lowerRETURNADDR(Op, DAG);
  case ISD::SHL_PARTS:
    return lowerShiftLeftParts(Op, DAG);
  case ISD::SRA_PARTS:
    return lowerShiftRightParts(Op, DAG, true);
  case ISD::SRL_PARTS:
    return lowerShiftRightParts(Op, DAG, false);
  case ISD::TRUNCATE:
    return Op;
  case ISD::ANY_EXTEND:
  case ISD::ZERO_EXTEND:
    return Op;
  case ISD::SIGN_EXTEND:
    return Op;
  case ISD::LOAD:
    return Op;
  case ISD::STORE:
    return Op;
  case ISD::SELECT_CC: {
    SDValue Tmp1 = Op.getOperand(0);
    SDValue Tmp2 = Op.getOperand(1);
    SDValue True = Op.getOperand(2);
    SDValue False = Op.getOperand(3);
    EVT VT = Op.getValueType();
    SDValue CC = Op.getOperand(4);
    EVT CmpVT = Tmp1.getValueType();
    EVT CCVT = getSetCCResultType(DAG.getDataLayout(), *DAG.getContext(), CmpVT);
    SDLoc DL(Op);
    SDValue Cond = DAG.getNode(ISD::SETCC, DL, CCVT, Tmp1, Tmp2, CC, Op->getFlags());
    return DAG.getSelect(DL, VT, Cond, True, False);
  }
  case ISD::SETCC: {
    MVT OpVT = Op.getOperand(0).getSimpleValueType();
    if (OpVT.isScalarInteger()) {
      MVT VT = Op.getSimpleValueType();
      SDValue LHS = Op.getOperand(0);
      SDValue RHS = Op.getOperand(1);
      ISD::CondCode CCVal = cast<CondCodeSDNode>(Op.getOperand(2))->get();
      SDLoc DL(Op);

      if (isa<ConstantSDNode>(RHS)) {
        int64_t Imm = cast<ConstantSDNode>(RHS)->getSExtValue();
        if (Imm != 0 && isInt<12>((uint64_t)Imm + 1)) {
          if (CCVal == ISD::SETUGT && Imm == -1)
            return DAG.getConstant(0, DL, VT);

          CCVal = ISD::getSetCCSwappedOperands(CCVal);
          SDValue SetCC = DAG.getSetCC(
              DL, VT, LHS, DAG.getConstant(Imm + 1, DL, OpVT), CCVal);
          return DAG.getLogicalNOT(DL, SetCC, VT);
        }
      }

      CCVal = ISD::getSetCCSwappedOperands(CCVal);
      return DAG.getSetCC(DL, VT, RHS, LHS, CCVal);
    }
    return Op;
  }
  case ISD::ADD:
  case ISD::SUB:
  case ISD::MUL:
  case ISD::MULHS:
  case ISD::MULHU:
  case ISD::AND:
  case ISD::OR:
  case ISD::XOR:
  case ISD::SDIV:
  case ISD::SREM:
  case ISD::UDIV:
  case ISD::UREM:
    return Op;
  case ISD::SHL:
  case ISD::SRA:
  case ISD::SRL:
    return SDValue();
  }
}

static SDValue getTargetNode(GlobalAddressSDNode *N, const SDLoc &DL, EVT Ty, SelectionDAG &DAG, unsigned Flags) {
  return DAG.getTargetGlobalAddress(N->getGlobal(), DL, Ty, 0, Flags);
}

static SDValue getTargetNode(BlockAddressSDNode *N, const SDLoc &DL, EVT Ty, SelectionDAG &DAG, unsigned Flags) {
  return DAG.getTargetBlockAddress(N->getBlockAddress(), Ty, N->getOffset(), Flags);
}

static SDValue getTargetNode(ConstantPoolSDNode *N, const SDLoc &DL, EVT Ty, SelectionDAG &DAG, unsigned Flags) {
  return DAG.getTargetConstantPool(N->getConstVal(), Ty, N->getAlign(), N->getOffset(), Flags);
}

static SDValue getTargetNode(JumpTableSDNode *N, const SDLoc &DL, EVT Ty, SelectionDAG &DAG, unsigned Flags) {
  return DAG.getTargetJumpTable(N->getIndex(), Ty, Flags);
}

template <class NodeTy>
SDValue RISCXTargetLowering::getAddr(NodeTy *N, SelectionDAG &DAG,  bool IsLocal, bool IsExternWeak) const {
  SDLoc DL(N);
  EVT Ty = getPointerTy(DAG.getDataLayout());

  if (isPositionIndependent()) {
    SDValue Addr = getTargetNode(N, DL, Ty, DAG, 0);

    SDValue Load = SDValue(DAG.getMachineNode(RISCX::PseudoLGA, DL, Ty, Addr), 0);
    MachineFunction &MF = DAG.getMachineFunction();
    MachineMemOperand *MemOp = MF.getMachineMemOperand(
        MachinePointerInfo::getGOT(MF),
        MachineMemOperand::MOLoad | MachineMemOperand::MODereferenceable |
            MachineMemOperand::MOInvariant,
        LLT(Ty.getSimpleVT()), Align(Ty.getFixedSizeInBits() / 8));
    DAG.setNodeMemRefs(cast<MachineSDNode>(Load.getNode()), {MemOp});
    return Load;
  }

  switch (getTargetMachine().getCodeModel()) {
  default:
    report_fatal_error("Unsupported code model for lowering");
  case CodeModel::Small: {
    SDValue AddrHi = getTargetNode(N, DL, Ty, DAG, RISCXII::MO_HI);
    SDValue AddrLo = getTargetNode(N, DL, Ty, DAG, RISCXII::MO_LO);
    SDValue MNHi = DAG.getNode(RISCXISD::HI, DL, Ty, AddrHi);
    return DAG.getNode(RISCXISD::ADD_LO, DL, Ty, MNHi, AddrLo);
  }
  case CodeModel::Medium: {
    SDValue Addr = getTargetNode(N, DL, Ty, DAG, 0);
    if (IsExternWeak) {
      SDValue Load = SDValue(DAG.getMachineNode(RISCX::PseudoLGA, DL, Ty, Addr), 0);
      MachineFunction &MF = DAG.getMachineFunction();
      MachineMemOperand *MemOp = MF.getMachineMemOperand(
          MachinePointerInfo::getGOT(MF),
          MachineMemOperand::MOLoad | MachineMemOperand::MODereferenceable |
              MachineMemOperand::MOInvariant,
          LLT(Ty.getSimpleVT()), Align(Ty.getFixedSizeInBits() / 8));
      DAG.setNodeMemRefs(cast<MachineSDNode>(Load.getNode()), {MemOp});
      return Load;
    }
  }
  }
  return SDValue();
}

SDValue RISCXTargetLowering::lowerGlobalAddress(SDValue Op, SelectionDAG &DAG) const {
  GlobalAddressSDNode *N = cast<GlobalAddressSDNode>(Op);
  const GlobalValue *GV = N->getGlobal();
  return getAddr(N, DAG, GV->isDSOLocal(), GV->hasExternalWeakLinkage());
}

SDValue RISCXTargetLowering::lowerBlockAddress(SDValue Op, SelectionDAG &DAG) const {
  BlockAddressSDNode *N = cast<BlockAddressSDNode>(Op);
  return getAddr(N, DAG);
}

SDValue RISCXTargetLowering::lowerConstantPool(SDValue Op, SelectionDAG &DAG) const {
  ConstantPoolSDNode *N = cast<ConstantPoolSDNode>(Op);
  return getAddr(N, DAG);
}

SDValue RISCXTargetLowering::lowerJumpTable(SDValue Op, SelectionDAG &DAG) const {
  JumpTableSDNode *N = cast<JumpTableSDNode>(Op);
  return getAddr(N, DAG);
}

static std::optional<bool> matchSetCC(SDValue LHS, SDValue RHS, ISD::CondCode CC, SDValue Val) {
  SDValue LHS2 = Val.getOperand(0);
  SDValue RHS2 = Val.getOperand(1);
  ISD::CondCode CC2 = cast<CondCodeSDNode>(Val.getOperand(2))->get();

  if (LHS == LHS2 && RHS == RHS2) {
    if (CC == CC2)
      return true;
    if (CC == ISD::getSetCCInverse(CC2, LHS2.getValueType()))
      return false;
  } else if (LHS == RHS2 && RHS == LHS2) {
    CC2 = ISD::getSetCCSwappedOperands(CC2);
    if (CC == CC2)
      return true;
    if (CC == ISD::getSetCCInverse(CC2, LHS2.getValueType()))
      return false;
  }

  return std::nullopt;
}

static SDValue combineSelectToBinOp(SDNode *N, SelectionDAG &DAG, const RISCXSubtarget &Subtarget) {
  SDValue CondV = N->getOperand(0);
  SDValue TrueV = N->getOperand(1);
  SDValue FalseV = N->getOperand(2);
  MVT VT = N->getSimpleValueType(0);
  SDLoc DL(N);

  if (isAllOnesConstant(TrueV)) {
    SDValue Neg = DAG.getNegative(CondV, DL, VT);
    return DAG.getNode(ISD::OR, DL, VT, Neg, FalseV);
  }
  if (isAllOnesConstant(FalseV)) {
    SDValue Neg = DAG.getNode(ISD::ADD, DL, VT, CondV, DAG.getAllOnesConstant(DL, VT));
    return DAG.getNode(ISD::OR, DL, VT, Neg, TrueV);
  }

  if (isNullConstant(TrueV)) {
    SDValue Neg = DAG.getNode(ISD::ADD, DL, VT, CondV, DAG.getAllOnesConstant(DL, VT));
    return DAG.getNode(ISD::AND, DL, VT, Neg, FalseV);
  }
  if (isNullConstant(FalseV)) {
    SDValue Neg = DAG.getNegative(CondV, DL, VT);
    return DAG.getNode(ISD::AND, DL, VT, Neg, TrueV);
  }

  if (CondV.getOpcode() == ISD::SETCC && TrueV.getOpcode() == ISD::SETCC && FalseV.getOpcode() == ISD::SETCC) {
    SDValue LHS = CondV.getOperand(0);
    SDValue RHS = CondV.getOperand(1);
    ISD::CondCode CC = cast<CondCodeSDNode>(CondV.getOperand(2))->get();

    if (std::optional<bool> MatchResult = matchSetCC(LHS, RHS, CC, TrueV)) {
      return DAG.getNode(*MatchResult ? ISD::OR : ISD::AND, DL, VT, TrueV, FalseV);
    }

    if (std::optional<bool> MatchResult = matchSetCC(LHS, RHS, CC, FalseV)) {
      return DAG.getNode(*MatchResult ? ISD::AND : ISD::OR, DL, VT, TrueV, FalseV);
    }
  }

  return SDValue();
}

static SDValue foldBinOpIntoSelectIfProfitable(SDNode *BO, SelectionDAG &DAG, const RISCXSubtarget &Subtarget) {
  unsigned SelOpNo = 0;
  SDValue Sel = BO->getOperand(0);
  if (Sel.getOpcode() != ISD::SELECT || !Sel.hasOneUse()) {
    SelOpNo = 1;
    Sel = BO->getOperand(1);
  }

  if (Sel.getOpcode() != ISD::SELECT || !Sel.hasOneUse())
    return SDValue();

  unsigned ConstSelOpNo = 1;
  unsigned OtherSelOpNo = 2;
  if (!dyn_cast<ConstantSDNode>(Sel->getOperand(ConstSelOpNo))) {
    ConstSelOpNo = 2;
    OtherSelOpNo = 1;
  }
  SDValue ConstSelOp = Sel->getOperand(ConstSelOpNo);
  ConstantSDNode *ConstSelOpNode = dyn_cast<ConstantSDNode>(ConstSelOp);
  if (!ConstSelOpNode || ConstSelOpNode->isOpaque())
    return SDValue();

  SDValue ConstBinOp = BO->getOperand(SelOpNo ^ 1);
  ConstantSDNode *ConstBinOpNode = dyn_cast<ConstantSDNode>(ConstBinOp);
  if (!ConstBinOpNode || ConstBinOpNode->isOpaque())
    return SDValue();

  SDLoc DL(Sel);
  EVT VT = BO->getValueType(0);

  SDValue NewConstOps[2] = {ConstSelOp, ConstBinOp};
  if (SelOpNo == 1)
    std::swap(NewConstOps[0], NewConstOps[1]);

  SDValue NewConstOp = DAG.FoldConstantArithmetic(BO->getOpcode(), DL, VT, NewConstOps);
  if (!NewConstOp)
    return SDValue();

  const APInt &NewConstAPInt = NewConstOp->getAsAPIntVal();
  if (!NewConstAPInt.isZero() && !NewConstAPInt.isAllOnes())
    return SDValue();

  SDValue OtherSelOp = Sel->getOperand(OtherSelOpNo);
  SDValue NewNonConstOps[2] = {OtherSelOp, ConstBinOp};
  if (SelOpNo == 1)
    std::swap(NewNonConstOps[0], NewNonConstOps[1]);
  SDValue NewNonConstOp = DAG.getNode(BO->getOpcode(), DL, VT, NewNonConstOps);

  SDValue NewT = (ConstSelOpNo == 1) ? NewConstOp : NewNonConstOp;
  SDValue NewF = (ConstSelOpNo == 1) ? NewNonConstOp : NewConstOp;
  return DAG.getSelect(DL, VT, Sel.getOperand(0), NewT, NewF);
}

SDValue RISCXTargetLowering::lowerSELECT(SDValue Op, SelectionDAG &DAG) const {
  SDValue CondV = Op.getOperand(0);
  SDValue TrueV = Op.getOperand(1);
  SDValue FalseV = Op.getOperand(2);
  SDLoc DL(Op);
  MVT VT = Op.getSimpleValueType();
  MVT XLenVT = Subtarget.getXLenVT();

  if (SDValue V = combineSelectToBinOp(Op.getNode(), DAG, Subtarget))
    return V;

  if (Op.hasOneUse()) {
    unsigned UseOpc = Op->use_begin()->getOpcode();
    if (isBinOp(UseOpc) && DAG.isSafeToSpeculativelyExecute(UseOpc)) {
      SDNode *BinOp = *Op->use_begin();
      if (SDValue NewSel = foldBinOpIntoSelectIfProfitable(*Op->use_begin(),
                                                           DAG, Subtarget)) {
        DAG.ReplaceAllUsesWith(BinOp, &NewSel);
        return lowerSELECT(NewSel, DAG);
      }
    }
  }

  if (CondV.getOpcode() != ISD::SETCC || CondV.getOperand(0).getSimpleValueType() != XLenVT) {
    SDValue Zero = DAG.getConstant(0, DL, XLenVT);
    SDValue SetNE = DAG.getCondCode(ISD::SETNE);

    SDValue Ops[] = {CondV, Zero, SetNE, TrueV, FalseV};

    return DAG.getNode(RISCXISD::SELECT_CC, DL, VT, Ops);
  }

  SDValue LHS = CondV.getOperand(0);
  SDValue RHS = CondV.getOperand(1);
  ISD::CondCode CCVal = cast<CondCodeSDNode>(CondV.getOperand(2))->get();

  if (isa<ConstantSDNode>(TrueV) && isa<ConstantSDNode>(FalseV) && CCVal == ISD::SETLT) {
    const APInt &TrueVal = TrueV->getAsAPIntVal();
    const APInt &FalseVal = FalseV->getAsAPIntVal();
    if (TrueVal - 1 == FalseVal)
      return DAG.getNode(ISD::ADD, DL, VT, CondV, FalseV);
    if (TrueVal + 1 == FalseVal)
      return DAG.getNode(ISD::SUB, DL, VT, FalseV, CondV);
  }

  translateSetCCForBranch(DL, LHS, RHS, CCVal, DAG);
  if (isOneConstant(LHS) && (CCVal == ISD::SETLT || CCVal == ISD::SETULT) && RHS == TrueV && LHS == FalseV) {
    LHS = DAG.getConstant(0, DL, VT);
    if (CCVal == ISD::SETULT) {
      std::swap(LHS, RHS);
      CCVal = ISD::SETNE;
    }
  }

  if (isAllOnesConstant(RHS) && CCVal == ISD::SETLT && LHS == TrueV && RHS == FalseV) {
    RHS = DAG.getConstant(0, DL, VT);
  }

  SDValue TargetCC = DAG.getCondCode(CCVal);

  if (isa<ConstantSDNode>(TrueV) && !isa<ConstantSDNode>(FalseV)) {
    std::swap(TrueV, FalseV);
    TargetCC = DAG.getCondCode(ISD::getSetCCInverse(CCVal, LHS.getValueType()));
  }

  SDValue Ops[] = {LHS, RHS, TargetCC, TrueV, FalseV};
  return DAG.getNode(RISCXISD::SELECT_CC, DL, VT, Ops);
}

SDValue RISCXTargetLowering::lowerBRCOND(SDValue Op, SelectionDAG &DAG) const {
  SDValue CondV = Op.getOperand(1);
  SDLoc DL(Op);
  MVT XLenVT = Subtarget.getXLenVT();

  if (CondV.getOpcode() == ISD::SETCC && CondV.getOperand(0).getValueType() == XLenVT) {
    SDValue LHS = CondV.getOperand(0);
    SDValue RHS = CondV.getOperand(1);
    ISD::CondCode CCVal = cast<CondCodeSDNode>(CondV.getOperand(2))->get();

    translateSetCCForBranch(DL, LHS, RHS, CCVal, DAG);

    SDValue TargetCC = DAG.getCondCode(CCVal);
    return DAG.getNode(RISCXISD::BR_CC, DL, Op.getValueType(), Op.getOperand(0), LHS, RHS, TargetCC, Op.getOperand(2));
  }

  return DAG.getNode(RISCXISD::BR_CC, DL, Op.getValueType(), Op.getOperand(0),
                     CondV, DAG.getConstant(0, DL, XLenVT),
                     DAG.getCondCode(ISD::SETNE), Op.getOperand(2));
}

SDValue RISCXTargetLowering::lowerFRAMEADDR(SDValue Op, SelectionDAG &DAG) const {
  const RISCXRegisterInfo &RI = *Subtarget.getRegisterInfo();
  MachineFunction &MF = DAG.getMachineFunction();
  MachineFrameInfo &MFI = MF.getFrameInfo();
  MFI.setFrameAddressIsTaken(true);
  Register FrameReg = RI.getFrameRegister(MF);
  int XLenInBytes = Subtarget.getXLen() / 8;

  EVT VT = Op.getValueType();
  SDLoc DL(Op);
  SDValue FrameAddr = DAG.getCopyFromReg(DAG.getEntryNode(), DL, FrameReg, VT);
  unsigned Depth = Op.getConstantOperandVal(0);
  while (Depth--) {
    int Offset = -(XLenInBytes * 2);
    SDValue Ptr = DAG.getNode(ISD::ADD, DL, VT, FrameAddr, DAG.getIntPtrConstant(Offset, DL));
    FrameAddr = DAG.getLoad(VT, DL, DAG.getEntryNode(), Ptr, MachinePointerInfo());
  }
  return FrameAddr;
}

SDValue RISCXTargetLowering::lowerRETURNADDR(SDValue Op, SelectionDAG &DAG) const {
  const RISCXRegisterInfo &RI = *Subtarget.getRegisterInfo();
  MachineFunction &MF = DAG.getMachineFunction();
  MachineFrameInfo &MFI = MF.getFrameInfo();
  MFI.setReturnAddressIsTaken(true);
  MVT XLenVT = Subtarget.getXLenVT();
  int XLenInBytes = Subtarget.getXLen() / 8;

  if (verifyReturnAddressArgumentIsConstant(Op, DAG))
    return SDValue();

  EVT VT = Op.getValueType();
  SDLoc DL(Op);
  unsigned Depth = Op.getConstantOperandVal(0);
  if (Depth) {
    int Off = -XLenInBytes;
    SDValue FrameAddr = lowerFRAMEADDR(Op, DAG);
    SDValue Offset = DAG.getConstant(Off, DL, VT);
    return DAG.getLoad(VT, DL, DAG.getEntryNode(),
                       DAG.getNode(ISD::ADD, DL, VT, FrameAddr, Offset),
                       MachinePointerInfo());
  }

  Register Reg = MF.addLiveIn(RI.getRARegister(), getRegClassFor(XLenVT));
  return DAG.getCopyFromReg(DAG.getEntryNode(), DL, Reg, XLenVT);
}

SDValue RISCXTargetLowering::lowerShiftLeftParts(SDValue Op, SelectionDAG &DAG) const {
  SDLoc DL(Op);
  SDValue Lo = Op.getOperand(0);
  SDValue Hi = Op.getOperand(1);
  SDValue Shamt = Op.getOperand(2);
  EVT VT = Lo.getValueType();

  SDValue Zero = DAG.getConstant(0, DL, VT);
  SDValue One = DAG.getConstant(1, DL, VT);
  SDValue MinusXLen = DAG.getConstant(-(int)Subtarget.getXLen(), DL, VT);
  SDValue XLenMinus1 = DAG.getConstant(Subtarget.getXLen() - 1, DL, VT);
  SDValue ShamtMinusXLen = DAG.getNode(ISD::ADD, DL, VT, Shamt, MinusXLen);
  SDValue XLenMinus1Shamt = DAG.getNode(ISD::SUB, DL, VT, XLenMinus1, Shamt);

  SDValue LoTrue = DAG.getNode(ISD::SHL, DL, VT, Lo, Shamt);
  SDValue ShiftRight1Lo = DAG.getNode(ISD::SRL, DL, VT, Lo, One);
  SDValue ShiftRightLo = DAG.getNode(ISD::SRL, DL, VT, ShiftRight1Lo, XLenMinus1Shamt);
  SDValue ShiftLeftHi = DAG.getNode(ISD::SHL, DL, VT, Hi, Shamt);
  SDValue HiTrue = DAG.getNode(ISD::OR, DL, VT, ShiftLeftHi, ShiftRightLo);
  SDValue HiFalse = DAG.getNode(ISD::SHL, DL, VT, Lo, ShamtMinusXLen);

  SDValue CC = DAG.getSetCC(DL, VT, ShamtMinusXLen, Zero, ISD::SETLT);

  Lo = DAG.getNode(ISD::SELECT, DL, VT, CC, LoTrue, Zero);
  Hi = DAG.getNode(ISD::SELECT, DL, VT, CC, HiTrue, HiFalse);

  SDValue Parts[2] = {Lo, Hi};
  return DAG.getMergeValues(Parts, DL);
}

SDValue RISCXTargetLowering::lowerShiftRightParts(SDValue Op, SelectionDAG &DAG, bool IsSRA) const {
  SDLoc DL(Op);
  SDValue Lo = Op.getOperand(0);
  SDValue Hi = Op.getOperand(1);
  SDValue Shamt = Op.getOperand(2);
  EVT VT = Lo.getValueType();

  unsigned ShiftRightOp = IsSRA ? ISD::SRA : ISD::SRL;

  SDValue Zero = DAG.getConstant(0, DL, VT);
  SDValue One = DAG.getConstant(1, DL, VT);
  SDValue MinusXLen = DAG.getConstant(-(int)Subtarget.getXLen(), DL, VT);
  SDValue XLenMinus1 = DAG.getConstant(Subtarget.getXLen() - 1, DL, VT);
  SDValue ShamtMinusXLen = DAG.getNode(ISD::ADD, DL, VT, Shamt, MinusXLen);
  SDValue XLenMinus1Shamt = DAG.getNode(ISD::SUB, DL, VT, XLenMinus1, Shamt);

  SDValue ShiftRightLo = DAG.getNode(ISD::SRL, DL, VT, Lo, Shamt);
  SDValue ShiftLeftHi1 = DAG.getNode(ISD::SHL, DL, VT, Hi, One);
  SDValue ShiftLeftHi = DAG.getNode(ISD::SHL, DL, VT, ShiftLeftHi1, XLenMinus1Shamt);
  SDValue LoTrue = DAG.getNode(ISD::OR, DL, VT, ShiftRightLo, ShiftLeftHi);
  SDValue HiTrue = DAG.getNode(ShiftRightOp, DL, VT, Hi, Shamt);
  SDValue LoFalse = DAG.getNode(ShiftRightOp, DL, VT, Hi, ShamtMinusXLen);
  SDValue HiFalse = IsSRA ? DAG.getNode(ISD::SRA, DL, VT, Hi, XLenMinus1) : Zero;

  SDValue CC = DAG.getSetCC(DL, VT, ShamtMinusXLen, Zero, ISD::SETLT);

  Lo = DAG.getNode(ISD::SELECT, DL, VT, CC, LoTrue, LoFalse);
  Hi = DAG.getNode(ISD::SELECT, DL, VT, CC, HiTrue, HiFalse);

  SDValue Parts[2] = {Lo, Hi};
  return DAG.getMergeValues(Parts, DL);
}


static RISCXISD::NodeType getRISCXWOpcode(unsigned Opcode) {
  switch (Opcode) {
  default:
    llvm_unreachable("Unexpected opcode");
  case ISD::SHL:
    return RISCXISD::SLLW;
  case ISD::SRA:
    return RISCXISD::SRAW;
  case ISD::SRL:
    return RISCXISD::SRLW;
  case ISD::SDIV:
    return RISCXISD::DIVW;
  case ISD::UDIV:
    return RISCXISD::DIVUW;
  case ISD::UREM:
    return RISCXISD::REMUW;
  }
}

static SDValue customLegalizeToWOp(SDNode *N, SelectionDAG &DAG, unsigned ExtOpc = ISD::ANY_EXTEND) {
  SDLoc DL(N);
  RISCXISD::NodeType WOpcode = getRISCXWOpcode(N->getOpcode());
  SDValue NewOp0 = DAG.getNode(ExtOpc, DL, MVT::i64, N->getOperand(0));
  SDValue NewOp1 = DAG.getNode(ExtOpc, DL, MVT::i64, N->getOperand(1));
  SDValue NewRes = DAG.getNode(WOpcode, DL, MVT::i64, NewOp0, NewOp1);
  return DAG.getNode(ISD::TRUNCATE, DL, N->getValueType(0), NewRes);
}

static SDValue customLegalizeToWOpWithSExt(SDNode *N, SelectionDAG &DAG) {
  SDLoc DL(N);
  SDValue NewOp0 = DAG.getNode(ISD::ANY_EXTEND, DL, MVT::i64, N->getOperand(0));
  SDValue NewOp1 = DAG.getNode(ISD::ANY_EXTEND, DL, MVT::i64, N->getOperand(1));
  SDValue NewWOp = DAG.getNode(N->getOpcode(), DL, MVT::i64, NewOp0, NewOp1);
  SDValue NewRes = DAG.getNode(ISD::SIGN_EXTEND_INREG, DL, MVT::i64, NewWOp, DAG.getValueType(MVT::i32));
  return DAG.getNode(ISD::TRUNCATE, DL, MVT::i32, NewRes);
}

void RISCXTargetLowering::ReplaceNodeResults(SDNode *N, SmallVectorImpl<SDValue> &Results, SelectionDAG &DAG) const {
  SDLoc DL(N);
  switch (N->getOpcode()) {
  default:
    llvm_unreachable("Don't know how to custom type legalize this operation!");
  case ISD::LOAD: {
    if (!ISD::isNON_EXTLoad(N))
      return;

    LoadSDNode *Ld = cast<LoadSDNode>(N);

    SDLoc dl(N);
    SDValue Res = DAG.getExtLoad(ISD::SEXTLOAD, dl, MVT::i64, Ld->getChain(),
                                 Ld->getBasePtr(), Ld->getMemoryVT(), Ld->getMemOperand());
    Results.push_back(DAG.getNode(ISD::TRUNCATE, dl, MVT::i32, Res));
    Results.push_back(Res.getValue(1));
    return;
  }
  case ISD::MUL: {
    unsigned Size = N->getSimpleValueType(0).getSizeInBits();
    unsigned XLen = Subtarget.getXLen();
    if (Size > XLen) {
      SDValue LHS = N->getOperand(0);
      SDValue RHS = N->getOperand(1);
      APInt HighMask = APInt::getHighBitsSet(Size, XLen);

      bool LHSIsU = DAG.MaskedValueIsZero(LHS, HighMask);
      bool RHSIsU = DAG.MaskedValueIsZero(RHS, HighMask);
      if (LHSIsU == RHSIsU)
        return;

      auto MakeMULPair = [&](SDValue S, SDValue U) {
        MVT XLenVT = Subtarget.getXLenVT();
        S = DAG.getNode(ISD::TRUNCATE, DL, XLenVT, S);
        U = DAG.getNode(ISD::TRUNCATE, DL, XLenVT, U);
        SDValue Lo = DAG.getNode(ISD::MUL, DL, XLenVT, S, U);
        SDValue Hi = DAG.getNode(RISCXISD::MULHSU, DL, XLenVT, S, U);
        return DAG.getNode(ISD::BUILD_PAIR, DL, N->getValueType(0), Lo, Hi);
      };

      bool LHSIsS = DAG.ComputeNumSignBits(LHS) > XLen;
      bool RHSIsS = DAG.ComputeNumSignBits(RHS) > XLen;

      if (RHSIsU && LHSIsS && !RHSIsS)
        Results.push_back(MakeMULPair(LHS, RHS));
      else if (LHSIsU && RHSIsS && !LHSIsS)
        Results.push_back(MakeMULPair(RHS, LHS));

      return;
    }
    [[fallthrough]];
  }
  case ISD::ADD:
  case ISD::SUB:
    Results.push_back(customLegalizeToWOpWithSExt(N, DAG));
    break;
  case ISD::SHL:
  case ISD::SRA:
  case ISD::SRL:
    if (N->getOperand(1).getOpcode() != ISD::Constant) {
      Results.push_back(customLegalizeToWOp(N, DAG));
      break;
    }
    if (N->getOpcode() == ISD::SHL) {
      SDLoc DL(N);
      SDValue NewOp0 = DAG.getNode(ISD::ANY_EXTEND, DL, MVT::i64, N->getOperand(0));
      SDValue NewOp1 = DAG.getNode(ISD::ZERO_EXTEND, DL, MVT::i64, N->getOperand(1));
      SDValue NewWOp = DAG.getNode(ISD::SHL, DL, MVT::i64, NewOp0, NewOp1);
      SDValue NewRes = DAG.getNode(ISD::SIGN_EXTEND_INREG, DL, MVT::i64, NewWOp, DAG.getValueType(MVT::i32));
      Results.push_back(DAG.getNode(ISD::TRUNCATE, DL, MVT::i32, NewRes));
    }

    break;
  case ISD::SDIV:
  case ISD::UDIV:
  case ISD::UREM: {
    MVT VT = N->getSimpleValueType(0);
    if (N->getOperand(1).getOpcode() == ISD::Constant)
      return;

    unsigned ExtOpc = ISD::ANY_EXTEND;
    if (VT != MVT::i32)
      ExtOpc = N->getOpcode() == ISD::SDIV ? ISD::SIGN_EXTEND : ISD::ZERO_EXTEND;

    Results.push_back(customLegalizeToWOp(N, DAG, ExtOpc));
    break;
  }
  }
}

void RISCXTargetLowering::computeKnownBitsForTargetNode(const SDValue Op,
                                                        KnownBits &Known,
                                                        const APInt &DemandedElts,
                                                        const SelectionDAG &DAG,
                                                        unsigned Depth) const {
  unsigned BitWidth = Known.getBitWidth();
  unsigned Opc = Op.getOpcode();

  Known.resetAll();
  switch (Opc) {
  default: break;
  case RISCXISD::SELECT_CC: {
    Known = DAG.computeKnownBits(Op.getOperand(4), Depth + 1);
    if (Known.isUnknown())
      break;
    KnownBits Known2 = DAG.computeKnownBits(Op.getOperand(3), Depth + 1);

    Known = Known.intersectWith(Known2);
    break;
  }
  case RISCXISD::REMUW: {
    KnownBits Known2;
    Known = DAG.computeKnownBits(Op.getOperand(0), DemandedElts, Depth + 1);
    Known2 = DAG.computeKnownBits(Op.getOperand(1), DemandedElts, Depth + 1);
    Known = KnownBits::urem(Known.trunc(32), Known2.trunc(32));
    Known = Known.sext(BitWidth);
    break;
  }
  case RISCXISD::DIVUW: {
    KnownBits Known2;
    Known = DAG.computeKnownBits(Op.getOperand(0), DemandedElts, Depth + 1);
    Known2 = DAG.computeKnownBits(Op.getOperand(1), DemandedElts, Depth + 1);
    Known = KnownBits::udiv(Known.trunc(32), Known2.trunc(32));
    Known = Known.sext(BitWidth);
    break;
  }
  case RISCXISD::SLLW: {
    KnownBits Known2;
    Known = DAG.computeKnownBits(Op.getOperand(0), DemandedElts, Depth + 1);
    Known2 = DAG.computeKnownBits(Op.getOperand(1), DemandedElts, Depth + 1);
    Known = KnownBits::shl(Known.trunc(32), Known2.trunc(5).zext(32));
    Known = Known.sext(BitWidth);
    break;
  }
  }
}

unsigned RISCXTargetLowering::ComputeNumSignBitsForTargetNode(SDValue Op, const APInt &DemandedElts, const SelectionDAG &DAG, unsigned Depth) const {
  switch (Op.getOpcode()) {
  default:
    break;
  case RISCXISD::SELECT_CC: {
    unsigned Tmp = DAG.ComputeNumSignBits(Op.getOperand(3), DemandedElts, Depth + 1);
    if (Tmp == 1)
      return 1;
    unsigned Tmp2 = DAG.ComputeNumSignBits(Op.getOperand(4), DemandedElts, Depth + 1);
    return std::min(Tmp, Tmp2);
  }
  case RISCXISD::SLLW:
  case RISCXISD::SRAW:
  case RISCXISD::SRLW:
  case RISCXISD::DIVW:
  case RISCXISD::DIVUW:
  case RISCXISD::REMUW:
    return 33;
  }

  return 1;
}

const Constant *RISCXTargetLowering::getTargetConstantFromLoad(LoadSDNode *Ld) const {
  if (!ISD::isNormalLoad(Ld))
    return nullptr;

  SDValue Ptr = Ld->getBasePtr();

  auto GetSupportedConstantPool = [](SDValue Ptr) -> ConstantPoolSDNode * {
    auto *CNode = dyn_cast<ConstantPoolSDNode>(Ptr);
    if (!CNode || CNode->isMachineConstantPoolEntry() ||
        CNode->getOffset() != 0)
      return nullptr;

    return CNode;
  };

  if (Ptr.getOpcode() != RISCXISD::ADD_LO || Ptr.getOperand(0).getOpcode() != RISCXISD::HI)
    return nullptr;

  auto *CNodeLo = GetSupportedConstantPool(Ptr.getOperand(1));
  auto *CNodeHi = GetSupportedConstantPool(Ptr.getOperand(0).getOperand(0));

  if (!CNodeLo || CNodeLo->getTargetFlags() != RISCXII::MO_LO ||
      !CNodeHi || CNodeHi->getTargetFlags() != RISCXII::MO_HI)
    return nullptr;

  if (CNodeLo->getConstVal() != CNodeHi->getConstVal())
    return nullptr;

  return CNodeLo->getConstVal();
}

ArrayRef<MCPhysReg> RISCX::getArgGPRs(const RISCXABI::ABI ABI) {
  static const MCPhysReg ArgIGPRs[] = {
    RISCX::X10, RISCX::X11, RISCX::X12,
    RISCX::X13, RISCX::X14, RISCX::X15,
    RISCX::X16, RISCX::X17
  };

  return ArrayRef(ArgIGPRs);
}

static bool CC_RISCXAssign2XLen(unsigned XLen, CCState &State, CCValAssign VA1,
                                ISD::ArgFlagsTy ArgFlags1, unsigned ValNo2,
                                MVT ValVT2, MVT LocVT2,
                                ISD::ArgFlagsTy ArgFlags2, bool EABI) {
  unsigned XLenInBytes = XLen / 8;
  const RISCXSubtarget &STI = State.getMachineFunction().getSubtarget<RISCXSubtarget>();
  ArrayRef<MCPhysReg> ArgGPRs = RISCX::getArgGPRs(STI.getTargetABI());

  if (Register Reg = State.AllocateReg(ArgGPRs)) {
    State.addLoc(CCValAssign::getReg(VA1.getValNo(), VA1.getValVT(), Reg, VA1.getLocVT(), CCValAssign::Full));
  } else {
    Align StackAlign(XLenInBytes);
    if (!EABI || XLen != 32)
      StackAlign = std::max(StackAlign, ArgFlags1.getNonZeroOrigAlign());
    State.addLoc(CCValAssign::getMem(VA1.getValNo(), VA1.getValVT(),
                                     State.AllocateStack(XLenInBytes, StackAlign),
                                     VA1.getLocVT(), CCValAssign::Full));
    State.addLoc(CCValAssign::getMem(ValNo2, ValVT2,
                                     State.AllocateStack(XLenInBytes, Align(XLenInBytes)),
                                     LocVT2, CCValAssign::Full));
    return false;
  }

  if (Register Reg = State.AllocateReg(ArgGPRs)) {
    State.addLoc(CCValAssign::getReg(ValNo2, ValVT2, Reg, LocVT2, CCValAssign::Full));
  } else {
    State.addLoc(CCValAssign::getMem(ValNo2, ValVT2,
                                     State.AllocateStack(XLenInBytes, Align(XLenInBytes)),
                                     LocVT2, CCValAssign::Full));
  }

  return false;
}


bool RISCX::CC_RISCX(const DataLayout &DL, RISCXABI::ABI ABI, unsigned ValNo,
                     MVT ValVT, MVT LocVT, CCValAssign::LocInfo LocInfo,
                     ISD::ArgFlagsTy ArgFlags, CCState &State, bool IsFixed,
                     bool IsRet, Type *OrigTy, const RISCXTargetLowering &TLI,
                     std::optional<unsigned> FirstMaskArgument) {
  unsigned XLen = DL.getLargestLegalIntTypeSizeInBits();
  MVT XLenVT = MVT::i64;

  if (ArgFlags.isNest()) {
    if (unsigned Reg = State.AllocateReg(RISCX::X7)) {
      State.addLoc(CCValAssign::getReg(ValNo, ValVT, Reg, LocVT, LocInfo));
      return false;
    }
  }

  if (IsRet && ValNo > 1)
    return true;

  ArrayRef<MCPhysReg> ArgGPRs = RISCX::getArgGPRs(ABI);

  unsigned TwoXLenInBytes = (2 * XLen) / 8;
  if (!IsFixed && ArgFlags.getNonZeroOrigAlign() == TwoXLenInBytes && DL.getTypeAllocSize(OrigTy) == TwoXLenInBytes) {
    unsigned RegIdx = State.getFirstUnallocated(ArgGPRs);
    if (RegIdx != std::size(ArgGPRs) && RegIdx % 2 == 1)
      State.AllocateReg(ArgGPRs);
  }

  SmallVectorImpl<CCValAssign> &PendingLocs = State.getPendingLocs();
  SmallVectorImpl<ISD::ArgFlagsTy> &PendingArgFlags = State.getPendingArgFlags();

  if (ValVT.isScalarInteger() && (ArgFlags.isSplit() || !PendingLocs.empty())) {
    LocVT = XLenVT;
    LocInfo = CCValAssign::Indirect;
    PendingLocs.push_back(CCValAssign::getPending(ValNo, ValVT, LocVT, LocInfo));
    PendingArgFlags.push_back(ArgFlags);
    if (!ArgFlags.isSplitEnd()) {
      return false;
    }
  }

  if (ValVT.isScalarInteger() && ArgFlags.isSplitEnd() && PendingLocs.size() <= 2) {
    CCValAssign VA = PendingLocs[0];
    ISD::ArgFlagsTy AF = PendingArgFlags[0];
    PendingLocs.clear();
    PendingArgFlags.clear();
    return CC_RISCXAssign2XLen(XLen, State, VA, AF, ValNo, ValVT, LocVT, ArgFlags, false);
  }

  Register Reg;
  unsigned StoreSizeBytes = XLen / 8;
  Align StackAlign = Align(XLen / 8);
  Reg = State.AllocateReg(ArgGPRs);

  unsigned StackOffset = Reg ? 0 : State.AllocateStack(StoreSizeBytes, StackAlign);

  if (!PendingLocs.empty()) {
    for (auto &It : PendingLocs) {
      if (Reg)
        It.convertToReg(Reg);
      else
        It.convertToMem(StackOffset);
      State.addLoc(It);
    }
    PendingLocs.clear();
    PendingArgFlags.clear();
    return false;
  }

  if (Reg) {
    State.addLoc(CCValAssign::getReg(ValNo, ValVT, Reg, LocVT, LocInfo));
    return false;
  }

  State.addLoc(CCValAssign::getMem(ValNo, ValVT, StackOffset, LocVT, LocInfo));
  return false;
}

void RISCXTargetLowering::analyzeInputArgs(MachineFunction &MF, CCState &CCInfo,
                                           const SmallVectorImpl<ISD::InputArg> &Ins, bool IsRet,
                                           RISCXCCAssignFn Fn) const
{
  unsigned NumArgs = Ins.size();
  FunctionType *FType = MF.getFunction().getFunctionType();

  std::optional<unsigned> FirstMaskArgument;
  for (unsigned i = 0; i != NumArgs; ++i) {
    MVT ArgVT = Ins[i].VT;
    ISD::ArgFlagsTy ArgFlags = Ins[i].Flags;

    Type *ArgTy = nullptr;
    if (IsRet)
      ArgTy = FType->getReturnType();
    else if (Ins[i].isOrigArg())
      ArgTy = FType->getParamType(Ins[i].getOrigArgIndex());

    RISCXABI::ABI ABI = MF.getSubtarget<RISCXSubtarget>().getTargetABI();
    if (Fn(MF.getDataLayout(), ABI, i, ArgVT, ArgVT, CCValAssign::Full,
           ArgFlags, CCInfo, /*IsFixed=*/true, IsRet, ArgTy, *this, FirstMaskArgument)) {
      llvm_unreachable(nullptr);
    }
  }
}

void RISCXTargetLowering::analyzeOutputArgs(MachineFunction &MF, CCState &CCInfo,
                                            const SmallVectorImpl<ISD::OutputArg> &Outs, bool IsRet,
                                            CallLoweringInfo *CLI, RISCXCCAssignFn Fn) const
{
  unsigned NumArgs = Outs.size();

  std::optional<unsigned> FirstMaskArgument;
  for (unsigned i = 0; i != NumArgs; i++) {
    MVT ArgVT = Outs[i].VT;
    ISD::ArgFlagsTy ArgFlags = Outs[i].Flags;
    Type *OrigTy = CLI ? CLI->getArgs()[Outs[i].OrigArgIndex].Ty : nullptr;

    RISCXABI::ABI ABI = MF.getSubtarget<RISCXSubtarget>().getTargetABI();
    if (Fn(MF.getDataLayout(), ABI, i, ArgVT, ArgVT, CCValAssign::Full,
           ArgFlags, CCInfo, Outs[i].IsFixed, IsRet, OrigTy, *this, FirstMaskArgument)) {
      llvm_unreachable(nullptr);
    }
  }
}

static SDValue convertLocVTToValVT(SelectionDAG &DAG, SDValue Val,
                                   const CCValAssign &VA, const SDLoc &DL,
                                   const RISCXSubtarget &Subtarget)
{
  switch (VA.getLocInfo()) {
  default:
    llvm_unreachable("Unexpected CCValAssign::LocInfo");
  case CCValAssign::Full:
    break;
  case CCValAssign::BCvt:
    Val = DAG.getNode(ISD::BITCAST, DL, VA.getValVT(), Val);
    break;
  }
  return Val;
}

static SDValue unpackFromRegLoc(SelectionDAG &DAG, SDValue Chain,
                                const CCValAssign &VA, const SDLoc &DL,
                                const ISD::InputArg &In,
                                const RISCXTargetLowering &TLI)
{
  MachineFunction &MF = DAG.getMachineFunction();
  MachineRegisterInfo &RegInfo = MF.getRegInfo();
  EVT LocVT = VA.getLocVT();
  SDValue Val;
  const TargetRegisterClass *RC = TLI.getRegClassFor(LocVT.getSimpleVT());
  Register VReg = RegInfo.createVirtualRegister(RC);
  RegInfo.addLiveIn(VA.getLocReg(), VReg);
  Val = DAG.getCopyFromReg(Chain, DL, VReg, LocVT);

  if (VA.getLocInfo() == CCValAssign::Indirect)
    return Val;

  return convertLocVTToValVT(DAG, Val, VA, DL, TLI.getSubtarget());
}

static SDValue convertValVTToLocVT(SelectionDAG &DAG, SDValue Val,
                                   const CCValAssign &VA, const SDLoc &DL,
                                   const RISCXSubtarget &Subtarget)
{
  EVT LocVT = VA.getLocVT();

  switch (VA.getLocInfo()) {
  default:
    llvm_unreachable("Unexpected CCValAssign::LocInfo");
  case CCValAssign::Full:
    break;
  case CCValAssign::BCvt:
    Val = DAG.getNode(ISD::BITCAST, DL, LocVT, Val);
    break;
  }
  return Val;
}

static SDValue unpackFromMemLoc(SelectionDAG &DAG, SDValue Chain, const CCValAssign &VA, const SDLoc &DL) {
  MachineFunction &MF = DAG.getMachineFunction();
  MachineFrameInfo &MFI = MF.getFrameInfo();
  EVT LocVT = VA.getLocVT();
  EVT ValVT = VA.getValVT();
  EVT PtrVT = MVT::getIntegerVT(DAG.getDataLayout().getPointerSizeInBits(0));
  int FI = MFI.CreateFixedObject(ValVT.getStoreSize(), VA.getLocMemOffset(), /*IsImmutable=*/true);
  SDValue FIN = DAG.getFrameIndex(FI, PtrVT);
  SDValue Val;

  ISD::LoadExtType ExtType;
  switch (VA.getLocInfo()) {
  default:
    llvm_unreachable("Unexpected CCValAssign::LocInfo");
  case CCValAssign::Full:
  case CCValAssign::Indirect:
  case CCValAssign::BCvt:
    ExtType = ISD::NON_EXTLOAD;
    break;
  }
  Val = DAG.getExtLoad(ExtType, DL, LocVT, Chain, FIN, MachinePointerInfo::getFixedStack(DAG.getMachineFunction(), FI), ValVT);
  return Val;
}

SDValue RISCXTargetLowering::LowerFormalArguments(SDValue Chain, CallingConv::ID CallConv, bool IsVarArg,
                                                  const SmallVectorImpl<ISD::InputArg> &Ins, const SDLoc &DL,
                                                  SelectionDAG &DAG, SmallVectorImpl<SDValue> &InVals) const
{
  MachineFunction &MF = DAG.getMachineFunction();
  std::vector<SDValue> OutChains;

  SmallVector<CCValAssign, 16> ArgLocs;
  CCState CCInfo(CallConv, IsVarArg, MF, ArgLocs, *DAG.getContext());

  analyzeInputArgs(MF, CCInfo, Ins, /*IsRet=*/false, RISCX::CC_RISCX);

  for (unsigned i = 0, e = ArgLocs.size(), InsIdx = 0; i != e; ++i, ++InsIdx) {
    CCValAssign &VA = ArgLocs[i];
    SDValue ArgValue;
    if (VA.isRegLoc())
      ArgValue = unpackFromRegLoc(DAG, Chain, VA, DL, Ins[InsIdx], *this);
    else
      ArgValue = unpackFromMemLoc(DAG, Chain, VA, DL);

    InVals.push_back(ArgValue);
  }

  if (!OutChains.empty()) {
    OutChains.push_back(Chain);
    Chain = DAG.getNode(ISD::TokenFactor, DL, MVT::Other, OutChains);
  }
  return Chain;
}

static Align getPrefTypeAlign(EVT VT, SelectionDAG &DAG) {
  return DAG.getDataLayout().getPrefTypeAlign(VT.getTypeForEVT(*DAG.getContext()));
}

SDValue RISCXTargetLowering::LowerCall(CallLoweringInfo &CLI, SmallVectorImpl<SDValue> &InVals) const {
  SelectionDAG &DAG = CLI.DAG;
  SDLoc &DL = CLI.DL;
  SmallVectorImpl<ISD::OutputArg> &Outs = CLI.Outs;
  SmallVectorImpl<SDValue> &OutVals = CLI.OutVals;
  SmallVectorImpl<ISD::InputArg> &Ins = CLI.Ins;
  SDValue Chain = CLI.Chain;
  SDValue Callee = CLI.Callee;
  bool &IsTailCall = CLI.IsTailCall;
  CallingConv::ID CallConv = CLI.CallConv;
  bool IsVarArg = CLI.IsVarArg;
  EVT PtrVT = getPointerTy(DAG.getDataLayout());
  MVT XLenVT = Subtarget.getXLenVT();

  std::string calleeName = "";
  MachineFunction &MF = DAG.getMachineFunction();

  SmallVector<CCValAssign, 16> ArgLocs;
  CCState ArgCCInfo(CallConv, IsVarArg, MF, ArgLocs, *DAG.getContext());

  analyzeOutputArgs(MF, ArgCCInfo, Outs, /*IsRet=*/false, &CLI, RISCX::CC_RISCX);

  unsigned NumBytes = ArgCCInfo.getStackSize();

  SmallVector<SDValue, 8> ByValArgs;
  for (unsigned i = 0, e = Outs.size(); i != e; ++i) {
    ISD::ArgFlagsTy Flags = Outs[i].Flags;
    if (!Flags.isByVal())
      continue;

    SDValue Arg = OutVals[i];
    unsigned Size = Flags.getByValSize();
    Align Alignment = Flags.getNonZeroByValAlign();

    int FI = MF.getFrameInfo().CreateStackObject(Size, Alignment, /*isSS=*/false);
    SDValue FIPtr = DAG.getFrameIndex(FI, getPointerTy(DAG.getDataLayout()));
    SDValue SizeNode = DAG.getConstant(Size, DL, XLenVT);

    Chain = DAG.getMemcpy(Chain, DL, FIPtr, Arg, SizeNode, Alignment, /*IsVolatile=*/false, /*AlwaysInline=*/false,
                          IsTailCall, MachinePointerInfo(), MachinePointerInfo());
    ByValArgs.push_back(FIPtr);
  }

  Chain = DAG.getCALLSEQ_START(Chain, NumBytes, 0, CLI.DL);

  SmallVector<std::pair<Register, SDValue>, 8> RegsToPass;
  SmallVector<SDValue, 8> MemOpChains;
  SDValue StackPtr;
  for (unsigned i = 0, j = 0, e = ArgLocs.size(), OutIdx = 0; i != e; ++i, ++OutIdx) {
    CCValAssign &VA = ArgLocs[i];
    SDValue ArgValue = OutVals[OutIdx];
    ISD::ArgFlagsTy Flags = Outs[OutIdx].Flags;

    if (VA.getLocInfo() == CCValAssign::Indirect) {
      Align StackAlign = std::max(getPrefTypeAlign(Outs[OutIdx].ArgVT, DAG), getPrefTypeAlign(ArgValue.getValueType(), DAG));
      TypeSize StoredSize = ArgValue.getValueType().getStoreSize();
      unsigned ArgIndex = Outs[OutIdx].OrigArgIndex;
      unsigned ArgPartOffset = Outs[OutIdx].PartOffset;

      SmallVector<std::pair<SDValue, SDValue>> Parts;
      while (i + 1 != e && Outs[OutIdx + 1].OrigArgIndex == ArgIndex) {
        SDValue PartValue = OutVals[OutIdx + 1];
        unsigned PartOffset = Outs[OutIdx + 1].PartOffset - ArgPartOffset;
        SDValue Offset = DAG.getIntPtrConstant(PartOffset, DL);
        EVT PartVT = PartValue.getValueType();
        StoredSize += PartVT.getStoreSize();
        StackAlign = std::max(StackAlign, getPrefTypeAlign(PartVT, DAG));
        Parts.push_back(std::make_pair(PartValue, Offset));
        ++i;
        ++OutIdx;
      }
      SDValue SpillSlot = DAG.CreateStackTemporary(StoredSize, StackAlign);
      int FI = cast<FrameIndexSDNode>(SpillSlot)->getIndex();
      MemOpChains.push_back(DAG.getStore(Chain, DL, ArgValue, SpillSlot, MachinePointerInfo::getFixedStack(MF, FI)));
      for (const auto &Part : Parts) {
        SDValue PartValue = Part.first;
        SDValue PartOffset = Part.second;
        SDValue Address = DAG.getNode(ISD::ADD, DL, PtrVT, SpillSlot, PartOffset);
        MemOpChains.push_back(DAG.getStore(Chain, DL, PartValue, Address, MachinePointerInfo::getFixedStack(MF, FI)));
      }
      ArgValue = SpillSlot;
    } else {
      ArgValue = convertValVTToLocVT(DAG, ArgValue, VA, DL, Subtarget);
    }

    if (Flags.isByVal())
      ArgValue = ByValArgs[j++];

    if (VA.isRegLoc()) {
      RegsToPass.push_back(std::make_pair(VA.getLocReg(), ArgValue));
    } else {
      if (!StackPtr.getNode())
        StackPtr = DAG.getCopyFromReg(Chain, DL, RISCX::X2, PtrVT);
      SDValue Address = DAG.getNode(ISD::ADD, DL, PtrVT, StackPtr, DAG.getIntPtrConstant(VA.getLocMemOffset(), DL));

      MemOpChains.push_back(DAG.getStore(Chain, DL, ArgValue, Address, MachinePointerInfo()));
    }
  }

  if (!MemOpChains.empty())
    Chain = DAG.getNode(ISD::TokenFactor, DL, MVT::Other, MemOpChains);

  SDValue Glue;

  for (auto &Reg : RegsToPass) {
    Chain = DAG.getCopyToReg(Chain, DL, Reg.first, Reg.second, Glue);
    Glue = Chain.getValue(1);
  }

  if (GlobalAddressSDNode *S = dyn_cast<GlobalAddressSDNode>(Callee)) {
    const GlobalValue *GV = S->getGlobal();
    calleeName = GV->getGlobalIdentifier();
    Callee = DAG.getTargetGlobalAddress(GV, DL, PtrVT, 0, RISCXII::MO_CALL);
  } else if (ExternalSymbolSDNode *S = dyn_cast<ExternalSymbolSDNode>(Callee)) {
    Callee = DAG.getTargetExternalSymbol(S->getSymbol(), PtrVT, RISCXII::MO_CALL);
  }

  SmallVector<SDValue, 8> Ops;
  Ops.push_back(Chain);
  Ops.push_back(Callee);

  for (auto &Reg : RegsToPass)
    Ops.push_back(DAG.getRegister(Reg.first, Reg.second.getValueType()));

  const TargetRegisterInfo *TRI = Subtarget.getRegisterInfo();
  const uint32_t *Mask = TRI->getCallPreservedMask(MF, CallConv);
  Ops.push_back(DAG.getRegisterMask(Mask));

  if (Glue.getNode())
    Ops.push_back(Glue);

  SDVTList NodeTys = DAG.getVTList(MVT::Other, MVT::Glue);

  if (calleeName == "simPutPixel") {
    Chain = DAG.getNode(RISCXISD::SIM_PUT_PIXEL, DL, NodeTys, Ops);
  } else if (calleeName == "simFlush") {
    Chain = DAG.getNode(RISCXISD::SIM_FLUSH, DL, NodeTys, Ops);
  } else if (calleeName == "simRand") {
    Chain = DAG.getNode(RISCXISD::SIM_RAND, DL, NodeTys, Ops);
  } else {
    Chain = DAG.getNode(RISCXISD::CALL, DL, NodeTys, Ops);
  }
  
  if (CLI.CFIType)
    Chain.getNode()->setCFIType(CLI.CFIType->getZExtValue());
  DAG.addNoMergeSiteInfo(Chain.getNode(), CLI.NoMerge);
  Glue = Chain.getValue(1);

  Chain = DAG.getCALLSEQ_END(Chain, NumBytes, 0, Glue, DL);
  Glue = Chain.getValue(1);

  SmallVector<CCValAssign, 16> RVLocs;
  CCState RetCCInfo(CallConv, IsVarArg, MF, RVLocs, *DAG.getContext());
  analyzeInputArgs(MF, RetCCInfo, Ins, /*IsRet=*/true, RISCX::CC_RISCX);

  for (unsigned i = 0, e = RVLocs.size(); i != e; ++i) {
    auto &VA = RVLocs[i];
    SDValue RetValue = DAG.getCopyFromReg(Chain, DL, VA.getLocReg(), VA.getLocVT(), Glue);
    Chain = RetValue.getValue(1);
    Glue = RetValue.getValue(2);

    RetValue = convertLocVTToValVT(DAG, RetValue, VA, DL, Subtarget);
    InVals.push_back(RetValue);
  }

  return Chain;
}

bool RISCXTargetLowering::CanLowerReturn(CallingConv::ID CallConv, MachineFunction &MF, bool IsVarArg,
                                         const SmallVectorImpl<ISD::OutputArg> &Outs, LLVMContext &Context) const
{
  SmallVector<CCValAssign, 16> RVLocs;
  CCState CCInfo(CallConv, IsVarArg, MF, RVLocs, Context);

  std::optional<unsigned> FirstMaskArgument;
  for (unsigned i = 0, e = Outs.size(); i != e; ++i) {
    MVT VT = Outs[i].VT;
    ISD::ArgFlagsTy ArgFlags = Outs[i].Flags;
    RISCXABI::ABI ABI = MF.getSubtarget<RISCXSubtarget>().getTargetABI();
    if (RISCX::CC_RISCX(MF.getDataLayout(), ABI, i, VT, VT, CCValAssign::Full, ArgFlags,
                        CCInfo, /*IsFixed=*/true, /*IsRet=*/true, nullptr, *this, FirstMaskArgument))
      return false;
  }
  return true;
}

SDValue RISCXTargetLowering::LowerReturn(SDValue Chain, CallingConv::ID CallConv,
                                         bool IsVarArg,
                                         const SmallVectorImpl<ISD::OutputArg> &Outs,
                                         const SmallVectorImpl<SDValue> &OutVals,
                                         const SDLoc &DL, SelectionDAG &DAG) const
{  SmallVector<CCValAssign, 16> RVLocs;

  CCState CCInfo(CallConv, IsVarArg, DAG.getMachineFunction(), RVLocs, *DAG.getContext());

  analyzeOutputArgs(DAG.getMachineFunction(), CCInfo, Outs, /*IsRet=*/true, nullptr, RISCX::CC_RISCX);

  SDValue Glue;
  SmallVector<SDValue, 4> RetOps(1, Chain);

  for (unsigned i = 0, e = RVLocs.size(), OutIdx = 0; i < e; ++i, ++OutIdx) {
    SDValue Val = OutVals[OutIdx];
    CCValAssign &VA = RVLocs[i];

    Val = convertValVTToLocVT(DAG, Val, VA, DL, Subtarget);
    Chain = DAG.getCopyToReg(Chain, DL, VA.getLocReg(), Val, Glue);
    Glue = Chain.getValue(1);
    RetOps.push_back(DAG.getRegister(VA.getLocReg(), VA.getLocVT()));
  }

  RetOps[0] = Chain;

  if (Glue.getNode()) {
    RetOps.push_back(Glue);
  }

  unsigned RetOpc = RISCXISD::RET_GLUE;
  return DAG.getNode(RetOpc, DL, MVT::Other, RetOps);
}

const char *RISCXTargetLowering::getTargetNodeName(unsigned Opcode) const {
#define NODE_NAME_CASE(NODE)                                                   \
  case RISCXISD::NODE:                                                         \
    return "RISCXISD::" #NODE;
  switch ((RISCXISD::NodeType)Opcode) {
  case RISCXISD::FIRST_NUMBER:
    break;
  NODE_NAME_CASE(RET_GLUE)
  NODE_NAME_CASE(CALL)
  NODE_NAME_CASE(SIM_PUT_PIXEL)
  NODE_NAME_CASE(SIM_FLUSH)
  NODE_NAME_CASE(SIM_RAND)
  NODE_NAME_CASE(SELECT_CC)
  NODE_NAME_CASE(BR_CC)
  NODE_NAME_CASE(ADD_LO)
  NODE_NAME_CASE(HI)
  NODE_NAME_CASE(MULHSU)
  NODE_NAME_CASE(SLLW)
  NODE_NAME_CASE(SRAW)
  NODE_NAME_CASE(SRLW)
  NODE_NAME_CASE(DIVW)
  NODE_NAME_CASE(DIVUW)
  NODE_NAME_CASE(REMUW)
  }
  return nullptr;
#undef NODE_NAME_CASE
}