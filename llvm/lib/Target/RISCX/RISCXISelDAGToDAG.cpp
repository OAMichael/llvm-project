#include "RISCXISelDAGToDAG.h"
#include "MCTargetDesc/RISCXBaseInfo.h"
#include "MCTargetDesc/RISCXMCTargetDesc.h"
#include "MCTargetDesc/RISCXMatInt.h"
#include "RISCXISelLowering.h"
#include "llvm/CodeGen/MachineFrameInfo.h"
#include "llvm/Support/Alignment.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/MathExtras.h"

using namespace llvm;

#define DEBUG_TYPE "riscx-isel"
#define PASS_NAME "RISC-X DAG->DAG Pattern Instruction Selection"

static SDValue selectImmSeq(SelectionDAG *CurDAG, const SDLoc &DL, const MVT VT, RISCXMatInt::InstSeq &Seq) {
  SDValue SrcReg = CurDAG->getRegister(RISCX::X0, VT);
  for (const RISCXMatInt::Inst &Inst : Seq) {
    SDValue SDImm = CurDAG->getTargetConstant(Inst.getImm(), DL, VT);
    SDNode *Result = nullptr;
    switch (Inst.getOpndKind()) {
    case RISCXMatInt::Imm:
      Result = CurDAG->getMachineNode(Inst.getOpcode(), DL, VT, SDImm);
      break;
    case RISCXMatInt::RegImm:
      Result = CurDAG->getMachineNode(Inst.getOpcode(), DL, VT, SrcReg, SDImm);
      break;
    }

    SrcReg = SDValue(Result, 0);
  }

  return SrcReg;
}

static SDValue selectImm(SelectionDAG *CurDAG, const SDLoc &DL, const MVT VT, int64_t Imm, const RISCXSubtarget &Subtarget) {
  RISCXMatInt::InstSeq Seq = RISCXMatInt::generateInstSeq(Imm, Subtarget);
  if (Seq.size() > 3) {
    unsigned ShiftAmt, AddOpc;
    RISCXMatInt::InstSeq SeqLo = RISCXMatInt::generateTwoRegInstSeq(Imm, Subtarget, ShiftAmt, AddOpc);
    if (!SeqLo.empty() && (SeqLo.size() + 2) < Seq.size()) {
      SDValue Lo = selectImmSeq(CurDAG, DL, VT, SeqLo);

      SDValue SLLI = SDValue(CurDAG->getMachineNode(RISCX::SLLI, DL, VT, Lo, CurDAG->getTargetConstant(ShiftAmt, DL, VT)), 0);
      return SDValue(CurDAG->getMachineNode(AddOpc, DL, VT, Lo, SLLI), 0);
    }
  }

  return selectImmSeq(CurDAG, DL, VT, Seq);
}

bool RISCXDAGToDAGISel::tryShrinkShlLogicImm(SDNode *Node) {
  MVT VT = Node->getSimpleValueType(0);
  unsigned Opcode = Node->getOpcode();
  SDLoc DL(Node);

  SDValue N0 = Node->getOperand(0);
  SDValue N1 = Node->getOperand(1);

  ConstantSDNode *Cst = dyn_cast<ConstantSDNode>(N1);
  if (!Cst)
    return false;

  int64_t Val = Cst->getSExtValue();

  if (isInt<12>(Val))
    return false;

  SDValue Shift = N0;

  bool SignExt = false;
  if (isInt<32>(Val) && N0.getOpcode() == ISD::SIGN_EXTEND_INREG && N0.hasOneUse() && cast<VTSDNode>(N0.getOperand(1))->getVT() == MVT::i32) {
    SignExt = true;
    Shift = N0.getOperand(0);
  }

  if (Shift.getOpcode() != ISD::SHL || !Shift.hasOneUse())
    return false;

  ConstantSDNode *ShlCst = dyn_cast<ConstantSDNode>(Shift.getOperand(1));
  if (!ShlCst)
    return false;

  uint64_t ShAmt = ShlCst->getZExtValue();

  uint64_t RemovedBitsMask = maskTrailingOnes<uint64_t>(ShAmt);
  if (Opcode != ISD::AND && (Val & RemovedBitsMask) != 0)
    return false;

  int64_t ShiftedVal = Val >> ShAmt;
  if (!isInt<12>(ShiftedVal))
    return false;

  if (SignExt && ShAmt >= 32)
    return false;

  unsigned BinOpc;
  switch (Opcode) {
  default: llvm_unreachable("Unexpected opcode");
  case ISD::AND: BinOpc = RISCX::ANDI; break;
  case ISD::OR:  BinOpc = RISCX::ORI;  break;
  case ISD::XOR: BinOpc = RISCX::XORI; break;
  }

  unsigned ShOpc = SignExt ? RISCX::SLLIW : RISCX::SLLI;

  SDNode *BinOp = CurDAG->getMachineNode(BinOpc, DL, VT, Shift.getOperand(0), CurDAG->getTargetConstant(ShiftedVal, DL, VT));
  SDNode *SLLI = CurDAG->getMachineNode(ShOpc, DL, VT, SDValue(BinOp, 0), CurDAG->getTargetConstant(ShAmt, DL, VT));
  ReplaceNode(Node, SLLI);
  return true;
}

void RISCXDAGToDAGISel::Select(SDNode *Node) {
  if (Node->isMachineOpcode()) {
    Node->setNodeId(-1);
    return;
  }

  unsigned Opcode = Node->getOpcode();
  SDLoc DL(Node);
  MVT VT = Node->getSimpleValueType(0);

  switch (Opcode) {
  case ISD::Constant: {
    auto *ConstNode = cast<ConstantSDNode>(Node);
    if (ConstNode->isZero()) {
      SDValue New = CurDAG->getCopyFromReg(CurDAG->getEntryNode(), DL, RISCX::X0, VT);
      ReplaceNode(Node, New.getNode());
      return;
    }
    int64_t Imm = ConstNode->getSExtValue();
    if (isUInt<16>(Imm) && isInt<12>(SignExtend64<16>(Imm)) && hasAllHUsers(Node))
      Imm = SignExtend64<16>(Imm);
    if (!isInt<32>(Imm) && isUInt<32>(Imm) && hasAllWUsers(Node))
      Imm = SignExtend64<32>(Imm);

    ReplaceNode(Node, selectImm(CurDAG, DL, VT, Imm, *Subtarget).getNode());
    return;
  }
  case ISD::SHL: {
    auto *N1C = dyn_cast<ConstantSDNode>(Node->getOperand(1));
    if (!N1C)
      break;
    SDValue N0 = Node->getOperand(0);
    if (N0.getOpcode() != ISD::AND || !N0.hasOneUse() || !isa<ConstantSDNode>(N0.getOperand(1)))
      break;
    unsigned ShAmt = N1C->getZExtValue();
    uint64_t Mask = N0.getConstantOperandVal(1);

    if (ShAmt <= 32 && isShiftedMask_64(Mask)) {
      unsigned XLen = Subtarget->getXLen();
      unsigned LeadingZeros = XLen - llvm::bit_width(Mask);
      unsigned TrailingZeros = llvm::countr_zero(Mask);
      if (TrailingZeros > 0 && LeadingZeros == 32) {
        SDNode *SRLIW = CurDAG->getMachineNode(RISCX::SRLIW, DL, VT, N0->getOperand(0), CurDAG->getTargetConstant(TrailingZeros, DL, VT));
        SDNode *SLLI = CurDAG->getMachineNode(RISCX::SLLI, DL, VT, SDValue(SRLIW, 0), CurDAG->getTargetConstant(TrailingZeros + ShAmt, DL, VT));
        ReplaceNode(Node, SLLI);
        return;
      }
    }
    break;
  }
  case ISD::SRL: {
    auto *N1C = dyn_cast<ConstantSDNode>(Node->getOperand(1));
    if (!N1C)
      break;
    SDValue N0 = Node->getOperand(0);
    if (N0.getOpcode() != ISD::AND || !isa<ConstantSDNode>(N0.getOperand(1)))
      break;
    unsigned ShAmt = N1C->getZExtValue();
    uint64_t Mask = N0.getConstantOperandVal(1);

    if (isShiftedMask_64(Mask) && N0.hasOneUse()) {
      unsigned XLen = Subtarget->getXLen();
      unsigned LeadingZeros = XLen - llvm::bit_width(Mask);
      unsigned TrailingZeros = llvm::countr_zero(Mask);
      if (LeadingZeros == 32 && TrailingZeros > ShAmt) {
        SDNode *SRLIW = CurDAG->getMachineNode(RISCX::SRLIW, DL, VT, N0->getOperand(0), CurDAG->getTargetConstant(TrailingZeros, DL, VT));
        SDNode *SLLI = CurDAG->getMachineNode(RISCX::SLLI, DL, VT, SDValue(SRLIW, 0), CurDAG->getTargetConstant(TrailingZeros - ShAmt, DL, VT));
        ReplaceNode(Node, SLLI);
        return;
      }
    }

    Mask |= maskTrailingOnes<uint64_t>(ShAmt);
    if (!isMask_64(Mask))
      break;
    unsigned TrailingOnes = llvm::countr_one(Mask);
    if (ShAmt >= TrailingOnes)
      break;
    if (TrailingOnes == 32) {
      SDNode *SRLI = CurDAG->getMachineNode(RISCX::SRLIW, DL, VT, N0->getOperand(0), CurDAG->getTargetConstant(ShAmt, DL, VT));
      ReplaceNode(Node, SRLI);
      return;
    }

    if (!N0.hasOneUse())
      break;

    unsigned LShAmt = Subtarget->getXLen() - TrailingOnes;
    SDNode *SLLI = CurDAG->getMachineNode(RISCX::SLLI, DL, VT, N0->getOperand(0), CurDAG->getTargetConstant(LShAmt, DL, VT));
    SDNode *SRLI = CurDAG->getMachineNode(RISCX::SRLI, DL, VT, SDValue(SLLI, 0), CurDAG->getTargetConstant(LShAmt + ShAmt, DL, VT));
    ReplaceNode(Node, SRLI);
    return;
  }
  case ISD::SRA: {
    auto *N1C = dyn_cast<ConstantSDNode>(Node->getOperand(1));
    if (!N1C)
      break;
    SDValue N0 = Node->getOperand(0);
    if (N0.getOpcode() != ISD::SIGN_EXTEND_INREG || !N0.hasOneUse())
      break;
    unsigned ShAmt = N1C->getZExtValue();
    unsigned ExtSize = cast<VTSDNode>(N0.getOperand(1))->getVT().getSizeInBits();

    if (ExtSize >= 32 || ShAmt >= ExtSize)
      break;
    unsigned LShAmt = Subtarget->getXLen() - ExtSize;
    SDNode *SLLI = CurDAG->getMachineNode(RISCX::SLLI, DL, VT, N0->getOperand(0), CurDAG->getTargetConstant(LShAmt, DL, VT));
    SDNode *SRAI = CurDAG->getMachineNode(RISCX::SRAI, DL, VT, SDValue(SLLI, 0), CurDAG->getTargetConstant(LShAmt + ShAmt, DL, VT));
    ReplaceNode(Node, SRAI);
    return;
  }
  case ISD::OR:
  case ISD::XOR:
    if (tryShrinkShlLogicImm(Node))
      return;

    break;
  case ISD::AND: {
    auto *N1C = dyn_cast<ConstantSDNode>(Node->getOperand(1));
    if (!N1C)
      break;
    uint64_t C1 = N1C->getZExtValue();
    const bool isC1Mask = isMask_64(C1);

    SDValue N0 = Node->getOperand(0);

    bool LeftShift = N0.getOpcode() == ISD::SHL;
    if (LeftShift || N0.getOpcode() == ISD::SRL) {
      auto *C = dyn_cast<ConstantSDNode>(N0.getOperand(1));
      if (!C)
        break;
      unsigned C2 = C->getZExtValue();
      unsigned XLen = Subtarget->getXLen();

      bool IsCANDI = isInt<6>(N1C->getSExtValue());

      if (LeftShift)
        C1 &= maskTrailingZeros<uint64_t>(C2);
      else
        C1 &= maskTrailingOnes<uint64_t>(XLen - C2);

      bool OneUseOrZExtW = N0.hasOneUse() || C1 == UINT64_C(0xFFFFFFFF);

      SDValue X = N0.getOperand(0);

      if (!LeftShift && isC1Mask) {
        unsigned Leading = XLen - llvm::bit_width(C1);
        if (C2 < Leading) {
          if (C2 + 32 == Leading) {
            SDNode *SRLIW = CurDAG->getMachineNode(RISCX::SRLIW, DL, VT, X, CurDAG->getTargetConstant(C2, DL, VT));
            ReplaceNode(Node, SRLIW);
            return;
          }

          if (C2 >= 32 && (Leading - C2) == 1 && N0.hasOneUse() && X.getOpcode() == ISD::SIGN_EXTEND_INREG && cast<VTSDNode>(X.getOperand(1))->getVT() == MVT::i32) {
            SDNode *SRAIW = CurDAG->getMachineNode(RISCX::SRAIW, DL, VT, X.getOperand(0), CurDAG->getTargetConstant(31, DL, VT));
            SDNode *SRLIW = CurDAG->getMachineNode(RISCX::SRLIW, DL, VT, SDValue(SRAIW, 0), CurDAG->getTargetConstant(Leading - 32, DL, VT));
            ReplaceNode(Node, SRLIW);
            return;
          }

          if (OneUseOrZExtW) {
            SDNode *SLLI = CurDAG->getMachineNode(RISCX::SLLI, DL, VT, X, CurDAG->getTargetConstant(Leading - C2, DL, VT));
            SDNode *SRLI = CurDAG->getMachineNode(RISCX::SRLI, DL, VT, SDValue(SLLI, 0), CurDAG->getTargetConstant(Leading, DL, VT));
            ReplaceNode(Node, SRLI);
            return;
          }
        }
      }

      if (LeftShift && isShiftedMask_64(C1)) {
        unsigned Leading = XLen - llvm::bit_width(C1);

        if (C2 + Leading < XLen && C1 == (maskTrailingOnes<uint64_t>(XLen - (C2 + Leading)) << C2)) {
          if (OneUseOrZExtW && !IsCANDI) {
            SDNode *SLLI = CurDAG->getMachineNode(RISCX::SLLI, DL, VT, X, CurDAG->getTargetConstant(C2 + Leading, DL, VT));
            SDNode *SRLI = CurDAG->getMachineNode(RISCX::SRLI, DL, VT, SDValue(SLLI, 0), CurDAG->getTargetConstant(Leading, DL, VT));
            ReplaceNode(Node, SRLI);
            return;
          }
        }
      }

      if (!LeftShift && isShiftedMask_64(C1)) {
        unsigned Leading = XLen - llvm::bit_width(C1);
        unsigned Trailing = llvm::countr_zero(C1);
        if (Leading == C2 && C2 + Trailing < XLen && OneUseOrZExtW && !IsCANDI) {
          unsigned SrliOpc = RISCX::SRLI;
          if (X.getOpcode() == ISD::AND &&
              isa<ConstantSDNode>(X.getOperand(1)) &&
              X.getConstantOperandVal(1) == UINT64_C(0xFFFFFFFF)) {
            SrliOpc = RISCX::SRLIW;
            X = X.getOperand(0);
          }
          SDNode *SRLI = CurDAG->getMachineNode(SrliOpc, DL, VT, X, CurDAG->getTargetConstant(C2 + Trailing, DL, VT));
          SDNode *SLLI = CurDAG->getMachineNode(RISCX::SLLI, DL, VT, SDValue(SRLI, 0), CurDAG->getTargetConstant(Trailing, DL, VT));
          ReplaceNode(Node, SLLI);
          return;
        }
        if (Leading > 32 && (Leading - 32) == C2 && C2 + Trailing < 32 && OneUseOrZExtW && !IsCANDI) {
          SDNode *SRLIW = CurDAG->getMachineNode(RISCX::SRLIW, DL, VT, X, CurDAG->getTargetConstant(C2 + Trailing, DL, VT));
          SDNode *SLLI = CurDAG->getMachineNode(RISCX::SLLI, DL, VT, SDValue(SRLIW, 0), CurDAG->getTargetConstant(Trailing, DL, VT));
          ReplaceNode(Node, SLLI);
          return;
        }
      }

      if (LeftShift && isShiftedMask_64(C1)) {
        unsigned Leading = XLen - llvm::bit_width(C1);
        unsigned Trailing = llvm::countr_zero(C1);
        if (Leading == 0 && C2 < Trailing && OneUseOrZExtW && !IsCANDI) {
          SDNode *SRLI = CurDAG->getMachineNode(RISCX::SRLI, DL, VT, X, CurDAG->getTargetConstant(Trailing - C2, DL, VT));
          SDNode *SLLI = CurDAG->getMachineNode(RISCX::SLLI, DL, VT, SDValue(SRLI, 0), CurDAG->getTargetConstant(Trailing, DL, VT));
          ReplaceNode(Node, SLLI);
          return;
        }

        if (C2 < Trailing && Leading + C2 == 32 && OneUseOrZExtW && !IsCANDI) {
          SDNode *SRLIW = CurDAG->getMachineNode(RISCX::SRLIW, DL, VT, X, CurDAG->getTargetConstant(Trailing - C2, DL, VT));
          SDNode *SLLI = CurDAG->getMachineNode(RISCX::SLLI, DL, VT, SDValue(SRLIW, 0), CurDAG->getTargetConstant(Trailing, DL, VT));
          ReplaceNode(Node, SLLI);
          return;
        }
      }
    }

    if (tryShrinkShlLogicImm(Node))
      return;

    break;
  }
  case ISD::MUL: {
    auto *N1C = dyn_cast<ConstantSDNode>(Node->getOperand(1));
    if (!N1C || !N1C->hasOneUse())
      break;

    SDValue N0 = Node->getOperand(0);
    if (N0.getOpcode() != ISD::AND || !isa<ConstantSDNode>(N0.getOperand(1)))
      break;

    uint64_t C2 = N0.getConstantOperandVal(1);

    if (!isMask_64(C2))
      break;

    unsigned XLen = Subtarget->getXLen();
    unsigned LeadingZeros = XLen - llvm::bit_width(C2);

    uint64_t C1 = N1C->getZExtValue();
    unsigned ConstantShift = XLen - LeadingZeros;
    if (ConstantShift > (XLen - llvm::bit_width(C1)))
      break;

    uint64_t ShiftedC1 = C1 << ConstantShift;
    if (XLen == 32)
      ShiftedC1 = SignExtend64<32>(ShiftedC1);

    SDNode *Imm = selectImm(CurDAG, DL, VT, ShiftedC1, *Subtarget).getNode();
    SDNode *SLLI = CurDAG->getMachineNode(RISCX::SLLI, DL, VT, N0.getOperand(0), CurDAG->getTargetConstant(LeadingZeros, DL, VT));
    SDNode *MULHU = CurDAG->getMachineNode(RISCX::MULHU, DL, VT, SDValue(SLLI, 0), SDValue(Imm, 0));
    ReplaceNode(Node, MULHU);
    return;
  }
  case ISD::LOAD: {
    break;
  }
  }
  SelectCode(Node);
}

bool RISCXDAGToDAGISel::SelectAddrFrameIndex(SDValue Addr, SDValue &Base, SDValue &Offset) {
  if (auto *FIN = dyn_cast<FrameIndexSDNode>(Addr)) {
    Base = CurDAG->getTargetFrameIndex(FIN->getIndex(), Subtarget->getXLenVT());
    Offset = CurDAG->getTargetConstant(0, SDLoc(Addr), Subtarget->getXLenVT());
    return true;
  }
  return false;
}

static bool selectConstantAddr(SelectionDAG *CurDAG, const SDLoc &DL,
                               const MVT VT, const RISCXSubtarget *Subtarget,
                               SDValue Addr, SDValue &Base, SDValue &Offset,
                               bool IsPrefetch = false) {
  if (!isa<ConstantSDNode>(Addr))
    return false;

  int64_t CVal = cast<ConstantSDNode>(Addr)->getSExtValue();

  int64_t Lo12 = SignExtend64<12>(CVal);
  int64_t Hi = (uint64_t)CVal - (uint64_t)Lo12;
  if (isInt<32>(Hi)) {
    if (IsPrefetch && (Lo12 & 0b11111) != 0)
      return false;

    if (Hi) {
      int64_t Hi20 = (Hi >> 12) & 0xfffff;
      Base = SDValue(CurDAG->getMachineNode(RISCX::LUI, DL, VT, CurDAG->getTargetConstant(Hi20, DL, VT)), 0);
    } else {
      Base = CurDAG->getRegister(RISCX::X0, VT);
    }
    Offset = CurDAG->getTargetConstant(Lo12, DL, VT);
    return true;
  }

  RISCXMatInt::InstSeq Seq = RISCXMatInt::generateInstSeq(CVal, *Subtarget);

  if (Seq.back().getOpcode() != RISCX::ADDI)
    return false;
  Lo12 = Seq.back().getImm();
  if (IsPrefetch && (Lo12 & 0b11111) != 0)
    return false;

  Seq.pop_back();

  Base = selectImmSeq(CurDAG, DL, VT, Seq);
  Offset = CurDAG->getTargetConstant(Lo12, DL, VT);
  return true;
}

static bool isWorthFoldingAdd(SDValue Add) {
  for (auto *Use : Add->uses()) {
    if (Use->getOpcode() != ISD::LOAD && Use->getOpcode() != ISD::STORE)
      return false;
    if (Use->getOpcode() == ISD::STORE && cast<StoreSDNode>(Use)->getValue() == Add)
      return false;
  }
  return true;
}

bool RISCXDAGToDAGISel::SelectAddrRegImm(SDValue Addr, SDValue &Base, SDValue &Offset, bool IsINX) {
  if (SelectAddrFrameIndex(Addr, Base, Offset))
    return true;

  SDLoc DL(Addr);
  MVT VT = Addr.getSimpleValueType();

  if (Addr.getOpcode() == RISCXISD::ADD_LO) {
    Base = Addr.getOperand(0);
    Offset = Addr.getOperand(1);
    return true;
  }

  int64_t RX32ZdinxRange = IsINX ? 4 : 0;
  if (CurDAG->isBaseWithConstantOffset(Addr)) {
    int64_t CVal = cast<ConstantSDNode>(Addr.getOperand(1))->getSExtValue();
    if (isInt<12>(CVal) && isInt<12>(CVal + RX32ZdinxRange)) {
      Base = Addr.getOperand(0);
      if (Base.getOpcode() == RISCXISD::ADD_LO) {
        SDValue LoOperand = Base.getOperand(1);
        if (auto *GA = dyn_cast<GlobalAddressSDNode>(LoOperand)) {
          const DataLayout &DL = CurDAG->getDataLayout();
          Align Alignment = commonAlignment(GA->getGlobal()->getPointerAlignment(DL), GA->getOffset());
          if (CVal == 0 || Alignment > CVal) {
            int64_t CombinedOffset = CVal + GA->getOffset();
            Base = Base.getOperand(0);
            Offset = CurDAG->getTargetGlobalAddress(GA->getGlobal(), SDLoc(LoOperand), LoOperand.getValueType(), CombinedOffset, GA->getTargetFlags());
            return true;
          }
        }
      }

      if (auto *FIN = dyn_cast<FrameIndexSDNode>(Base))
        Base = CurDAG->getTargetFrameIndex(FIN->getIndex(), VT);
      Offset = CurDAG->getTargetConstant(CVal, DL, VT);
      return true;
    }
  }

  if (Addr.getOpcode() == ISD::ADD && isa<ConstantSDNode>(Addr.getOperand(1))) {
    int64_t CVal = cast<ConstantSDNode>(Addr.getOperand(1))->getSExtValue();

    if (isInt<12>(CVal / 2) && isInt<12>(CVal - CVal / 2)) {
      int64_t Adj = CVal < 0 ? -2048 : 2047;
      Base = SDValue(CurDAG->getMachineNode(RISCX::ADDI, DL, VT, Addr.getOperand(0), CurDAG->getTargetConstant(Adj, DL, VT)), 0);
      Offset = CurDAG->getTargetConstant(CVal - Adj, DL, VT);
      return true;
    }

    if (isWorthFoldingAdd(Addr) && selectConstantAddr(CurDAG, DL, VT, Subtarget, Addr.getOperand(1), Base, Offset)) {
      Base = SDValue(CurDAG->getMachineNode(RISCX::ADD, DL, VT, Addr.getOperand(0), Base), 0);
      return true;
    }
  }

  if (selectConstantAddr(CurDAG, DL, VT, Subtarget, Addr, Base, Offset))
    return true;

  Base = Addr;
  Offset = CurDAG->getTargetConstant(0, DL, VT);
  return true;
}

bool RISCXDAGToDAGISel::selectSETCC(SDValue N, ISD::CondCode ExpectedCCVal, SDValue &Val) {
  if (N->getOpcode() != ISD::SETCC)
    return false;

  ISD::CondCode CCVal = cast<CondCodeSDNode>(N->getOperand(2))->get();
  if (CCVal != ExpectedCCVal)
    return false;

  SDValue LHS = N->getOperand(0);
  SDValue RHS = N->getOperand(1);

  if (!LHS.getValueType().isScalarInteger())
    return false;

  if (isNullConstant(RHS)) {
    Val = LHS;
    return true;
  }

  SDLoc DL(N);

  if (auto *C = dyn_cast<ConstantSDNode>(RHS)) {
    int64_t CVal = C->getSExtValue();
    if (CVal == -2048) {
      Val = SDValue(CurDAG->getMachineNode(RISCX::XORI, DL, N->getValueType(0), LHS, CurDAG->getTargetConstant(CVal, DL, N->getValueType(0))), 0);
      return true;
    }

    if (isInt<12>(CVal) || CVal == 2048) {
      Val = SDValue(CurDAG->getMachineNode(RISCX::ADDI, DL, N->getValueType(0), LHS, CurDAG->getTargetConstant(-CVal, DL, N->getValueType(0))), 0);
      return true;
    }
  }

  Val = SDValue(CurDAG->getMachineNode(RISCX::XOR, DL, N->getValueType(0), LHS, RHS), 0);
  return true;
}

bool RISCXDAGToDAGISel::selectSExtBits(SDValue N, unsigned Bits, SDValue &Val) {
  if (N.getOpcode() == ISD::SIGN_EXTEND_INREG && cast<VTSDNode>(N.getOperand(1))->getVT().getSizeInBits() == Bits) {
    Val = N.getOperand(0);
    return true;
  }

  auto UnwrapShlSra = [](SDValue N, unsigned ShiftAmt) {
    if (N.getOpcode() != ISD::SRA || !isa<ConstantSDNode>(N.getOperand(1)))
      return N;

    SDValue N0 = N.getOperand(0);
    if (N0.getOpcode() == ISD::SHL && isa<ConstantSDNode>(N0.getOperand(1)) &&
        N.getConstantOperandVal(1) == ShiftAmt &&
        N0.getConstantOperandVal(1) == ShiftAmt)
      return N0.getOperand(0);

    return N;
  };

  MVT VT = N.getSimpleValueType();
  if (CurDAG->ComputeNumSignBits(N) > (VT.getSizeInBits() - Bits)) {
    Val = UnwrapShlSra(N, VT.getSizeInBits() - Bits);
    return true;
  }
  return false;
}

bool RISCXDAGToDAGISel::selectZExtBits(SDValue N, unsigned Bits, SDValue &Val) {
  if (N.getOpcode() == ISD::AND) {
    auto *C = dyn_cast<ConstantSDNode>(N.getOperand(1));
    if (C && C->getZExtValue() == maskTrailingOnes<uint64_t>(Bits)) {
      Val = N.getOperand(0);
      return true;
    }
  }
  MVT VT = N.getSimpleValueType();
  APInt Mask = APInt::getBitsSetFrom(VT.getSizeInBits(), Bits);
  if (CurDAG->MaskedValueIsZero(N, Mask)) {
    Val = N;
    return true;
  }
  return false;
}

bool RISCXDAGToDAGISel::hasAllNBitUsers(SDNode *Node, unsigned Bits, const unsigned Depth) const {
  if (Depth >= SelectionDAG::MaxRecursionDepth)
    return false;

  if (Depth == 0 && !Node->getValueType(0).isScalarInteger())
    return false;

  for (auto UI = Node->use_begin(), UE = Node->use_end(); UI != UE; ++UI) {
    SDNode *User = *UI;
    if (!User->isMachineOpcode())
      return false;

    switch (User->getMachineOpcode()) {
    default:
      return false;
    case RISCX::ADDW:
    case RISCX::ADDIW:
    case RISCX::SUBW:
    case RISCX::MULW:
    case RISCX::SLLW:
    case RISCX::SLLIW:
    case RISCX::SRAW:
    case RISCX::SRAIW:
    case RISCX::SRLW:
    case RISCX::SRLIW:
    case RISCX::DIVW:
    case RISCX::DIVUW:
    case RISCX::REMW:
    case RISCX::REMUW:
      if (Bits < 32)
        return false;
      break;
    case RISCX::SLL:
    case RISCX::SRA:
    case RISCX::SRL:
      if (UI.getOperandNo() != 1 || Bits < Log2_32(Subtarget->getXLen()))
        return false;
      break;
    case RISCX::SLLI:
      if (Bits < Subtarget->getXLen() - User->getConstantOperandVal(1))
        return false;
      break;
    case RISCX::ANDI:
      if (Bits >= (unsigned)llvm::bit_width(User->getConstantOperandVal(1)))
        break;
      goto RecCheck;
    case RISCX::ORI: {
      uint64_t Imm = cast<ConstantSDNode>(User->getOperand(1))->getSExtValue();
      if (Bits >= (unsigned)llvm::bit_width<uint64_t>(~Imm))
        break;
      [[fallthrough]];
    }
    case RISCX::AND:
    case RISCX::OR:
    case RISCX::XOR:
    case RISCX::XORI:
    RecCheck:
      if (hasAllNBitUsers(User, Bits, Depth + 1))
        break;
      return false;
    case RISCX::SRLI: {
      unsigned ShAmt = User->getConstantOperandVal(1);
      if (Bits > ShAmt && hasAllNBitUsers(User, Bits - ShAmt, Depth + 1))
        break;
      return false;
    }
    case RISCX::SB:
      if (UI.getOperandNo() != 0 || Bits < 8)
        return false;
      break;
    case RISCX::SH:
      if (UI.getOperandNo() != 0 || Bits < 16)
        return false;
      break;
    case RISCX::SW:
      if (UI.getOperandNo() != 0 || Bits < 32)
        return false;
      break;
    }
  }

  return true;
}

bool RISCXDAGToDAGISel::selectShiftMask(SDValue N, unsigned ShiftWidth, SDValue &ShAmt) {
  ShAmt = N;

  if (ShAmt->getOpcode() == ISD::ZERO_EXTEND)
    ShAmt = ShAmt.getOperand(0);

  if (ShAmt.getOpcode() == ISD::AND && isa<ConstantSDNode>(ShAmt.getOperand(1))) {
    const APInt &AndMask = ShAmt.getConstantOperandAPInt(1);

    APInt ShMask(AndMask.getBitWidth(), ShiftWidth - 1);

    if (ShMask.isSubsetOf(AndMask)) {
      ShAmt = ShAmt.getOperand(0);
    } else {
      KnownBits Known = CurDAG->computeKnownBits(ShAmt.getOperand(0));
      if (!ShMask.isSubsetOf(AndMask | Known.Zero))
        return true;
      ShAmt = ShAmt.getOperand(0);
    }
  }

  if (ShAmt.getOpcode() == ISD::ADD && isa<ConstantSDNode>(ShAmt.getOperand(1))) {
    uint64_t Imm = ShAmt.getConstantOperandVal(1);
    if (Imm != 0 && Imm % ShiftWidth == 0) {
      ShAmt = ShAmt.getOperand(0);
      return true;
    }
  } else if (ShAmt.getOpcode() == ISD::SUB && isa<ConstantSDNode>(ShAmt.getOperand(0))) {
    uint64_t Imm = ShAmt.getConstantOperandVal(0);
    if (Imm != 0 && Imm % ShiftWidth == 0) {
      SDLoc DL(ShAmt);
      EVT VT = ShAmt.getValueType();
      SDValue Zero = CurDAG->getRegister(RISCX::X0, VT);
      unsigned NegOpc = VT == MVT::i64 ? RISCX::SUBW : RISCX::SUB;
      MachineSDNode *Neg = CurDAG->getMachineNode(NegOpc, DL, VT, Zero, ShAmt.getOperand(1));
      ShAmt = SDValue(Neg, 0);
      return true;
    }
    if (Imm % ShiftWidth == ShiftWidth - 1) {
      SDLoc DL(ShAmt);
      EVT VT = ShAmt.getValueType();
      MachineSDNode *Not = CurDAG->getMachineNode(RISCX::XORI, DL, VT, ShAmt.getOperand(1), CurDAG->getTargetConstant(-1, DL, VT));
      ShAmt = SDValue(Not, 0);
      return true;
    }
  }
  return true;
}

FunctionPass *llvm::createRISCXISelDag(RISCXTargetMachine &TM, CodeGenOptLevel OptLevel) {
  return new RISCXDAGToDAGISel(TM, OptLevel);
}

char RISCXDAGToDAGISel::ID = 0;

INITIALIZE_PASS(RISCXDAGToDAGISel, DEBUG_TYPE, PASS_NAME, false, false)