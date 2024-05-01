//===----------------------------------------------------------------------===//
// SDNode Types
//===----------------------------------------------------------------------===//

def SDT_CallSeqStart : SDCallSeqStart<[SDTCisVT<0, i32>, SDTCisVT<1, i32>]>;
def SDT_CallSeqEnd   : SDCallSeqEnd<[SDTCisVT<0, i32>, SDTCisVT<1, i32>]>;

def SDT_RISCXCall     : SDTypeProfile<0, -1, [SDTCisVT<0, XLenVT>]>;
def SDT_RISCXSelectCC : SDTypeProfile<1, 5, [SDTCisSameAs<1, 2>,
                                             SDTCisVT<3, OtherVT>,
                                             SDTCisSameAs<0, 4>,
                                             SDTCisSameAs<4, 5>]>;

def SDT_RISCXBrCC : SDTypeProfile<0, 4, [SDTCisSameAs<0, 1>,
                                         SDTCisVT<2, OtherVT>,
                                         SDTCisVT<3, OtherVT>]>;

def SDT_RISCXIntBinOpW : SDTypeProfile<1, 2, [SDTCisSameAs<0, 1>, SDTCisSameAs<0, 2>, SDTCisVT<0, i64>]>;

def callseq_start   : SDNode<"ISD::CALLSEQ_START", SDT_CallSeqStart, [SDNPHasChain, SDNPOutGlue]>;
def callseq_end     : SDNode<"ISD::CALLSEQ_END", SDT_CallSeqEnd, [SDNPHasChain, SDNPOptInGlue, SDNPOutGlue]>;

def riscx_call      : SDNode<"RISCXISD::CALL", SDT_RISCXCall, [SDNPHasChain, SDNPOptInGlue, SDNPOutGlue, SDNPVariadic]>;
def riscx_ret_glue  : SDNode<"RISCXISD::RET_GLUE", SDTNone, [SDNPHasChain, SDNPOptInGlue, SDNPVariadic]>;
def riscx_selectcc  : SDNode<"RISCXISD::SELECT_CC", SDT_RISCXSelectCC>;
def riscx_brcc      : SDNode<"RISCXISD::BR_CC", SDT_RISCXBrCC, [SDNPHasChain]>;
def riscx_sllw      : SDNode<"RISCXISD::SLLW", SDT_RISCXIntBinOpW>;
def riscx_sraw      : SDNode<"RISCXISD::SRAW", SDT_RISCXIntBinOpW>;
def riscx_srlw      : SDNode<"RISCXISD::SRLW", SDT_RISCXIntBinOpW>;

def riscx_mulhsu    : SDNode<"RISCXISD::MULHSU", SDTIntBinOp>;
def riscx_divw      : SDNode<"RISCXISD::DIVW",  SDT_RISCXIntBinOpW>;
def riscx_divuw     : SDNode<"RISCXISD::DIVUW", SDT_RISCXIntBinOpW>;
def riscx_remuw     : SDNode<"RISCXISD::REMUW", SDT_RISCXIntBinOpW>;

def riscx_add_lo    : SDNode<"RISCXISD::ADD_LO", SDTIntBinOp>;
def riscx_hi        : SDNode<"RISCXISD::HI", SDTIntUnaryOp>;

class ImmAsmOperand<string prefix, int width, string suffix> : AsmOperandClass {
  let Name = prefix # "Imm" # width # suffix;
  let RenderMethod = "addImmOperands";
  let DiagnosticType = !strconcat("Invalid", Name);
}

class MemOperand<RegisterClass regClass> : RegisterOperand<regClass>{
  let OperandType = "OPERAND_MEMORY";
}

def GPRMem : MemOperand<GPR>;

//===----------------------------------------------------------------------===//
// Immediates
//===----------------------------------------------------------------------===//

class SImmAsmOperand<int width, string suffix = ""> : ImmAsmOperand<"S", width, suffix> {
}

class UImmAsmOperand<int width, string suffix = ""> : ImmAsmOperand<"U", width, suffix> {
}

class RISCXOp<ValueType vt = XLenVT> : Operand<vt> {
  let OperandNamespace = "RISCXOp";
}

class RISCXUImmOp<int bitsNum> : RISCXOp {
  let ParserMatchClass = UImmAsmOperand<bitsNum>;
  let DecoderMethod = "decodeUImmOperand<" # bitsNum # ">";
  let OperandType = "OPERAND_UIMM" # bitsNum;
}

class RISCXSImmOp<int bitsNum> : RISCXOp {
  let ParserMatchClass = SImmAsmOperand<bitsNum>;
  let EncoderMethod = "getImmOpValue";
  let DecoderMethod = "decodeSImmOperand<" # bitsNum # ">";
  let OperandType = "OPERAND_SIMM" # bitsNum;
}

class RISCXUImmLeafOp<int bitsNum> :
  RISCXUImmOp<bitsNum>, ImmLeaf<XLenVT, "return isUInt<" # bitsNum # ">(Imm);">;

class RISCXSImmLeafOp<int bitsNum> :
  RISCXSImmOp<bitsNum>, ImmLeaf<XLenVT, "return isInt<" # bitsNum # ">(Imm);">;

def UImmLog2XLenAsmOperand : AsmOperandClass {
  let Name = "UImmLog2XLen";
  let RenderMethod = "addImmOperands";
  let DiagnosticType = "InvalidUImmLog2XLen";
}

def uimmlog2xlen : RISCXOp, ImmLeaf<XLenVT, [{
  return isUInt<6>(Imm);
}]> {
  let ParserMatchClass = UImmLog2XLenAsmOperand;
  let DecoderMethod = "decodeUImmOperand<6>";
  let MCOperandPredicate = [{
    int64_t Imm;
    if (!MCOp.evaluateAsConstantImm(Imm))
      return false;
    if (STI.getTargetTriple().isArch64Bit())
      return isUInt<6>(Imm);
    return isUInt<5>(Imm);
  }];
  let OperandType = "OPERAND_UIMMLOG2XLEN";
}

def uimm5 : RISCXUImmLeafOp<5>;
def simm12 : RISCXSImmLeafOp<12> {
  let MCOperandPredicate = [{
    int64_t Imm;
    if (MCOp.evaluateAsConstantImm(Imm))
      return isInt<12>(Imm);
    return MCOp.isBareSymbolRef();
  }];
}

def simm13_lsb0 : Operand<OtherVT> {
  let ParserMatchClass = SImmAsmOperand<13, "Lsb0">;
  let PrintMethod = "printBranchOperand";
  let EncoderMethod = "getImmOpValueAsr1";
  let DecoderMethod = "decodeSImmOperandAndLsl1<13>";
  let MCOperandPredicate = [{
    int64_t Imm;
    if (MCOp.evaluateAsConstantImm(Imm))
      return isShiftedInt<12, 1>(Imm);
    return MCOp.isBareSymbolRef();
  }];
  let OperandType = "OPERAND_PCREL";
}

class UImm20Operand : RISCXOp {
  let EncoderMethod = "getImmOpValue";
  let DecoderMethod = "decodeUImmOperand<20>";
  let OperandType = "OPERAND_UIMM20";
}

class UImm20OperandMaybeSym : UImm20Operand {
  let MCOperandPredicate = [{
    int64_t Imm;
    if (MCOp.evaluateAsConstantImm(Imm))
      return isUInt<20>(Imm);
    return MCOp.isBareSymbolRef();
  }];
}

def uimm20_lui : UImm20OperandMaybeSym {
  let ParserMatchClass = UImmAsmOperand<20, "LUI">;
}
def uimm20_auipc : UImm20OperandMaybeSym {
  let ParserMatchClass = UImmAsmOperand<20, "AUIPC">;
}

def Simm21Lsb0JALAsmOperand : SImmAsmOperand<21, "Lsb0JAL"> {
  let ParserMethod = "parseJALOffset";
}

def simm21_lsb0_jal : Operand<OtherVT> {
  let ParserMatchClass = Simm21Lsb0JALAsmOperand;
  let PrintMethod = "printBranchOperand";
  let EncoderMethod = "getImmOpValueAsr1";
  let DecoderMethod = "decodeSImmOperandAndLsl1<21>";
  let MCOperandPredicate = [{
    int64_t Imm;
    if (MCOp.evaluateAsConstantImm(Imm))
      return isShiftedInt<20, 1>(Imm);
    return MCOp.isBareSymbolRef();
  }];
  let OperandType = "OPERAND_PCREL";
}

def BareSymbol : AsmOperandClass {
  let Name = "BareSymbol";
  let RenderMethod = "addImmOperands";
  let DiagnosticType = "InvalidBareSymbol";
  let ParserMethod = "parseBareSymbol";
}

def bare_symbol : Operand<XLenVT> {
  let ParserMatchClass = BareSymbol;
}

def CallSymbol : AsmOperandClass {
  let Name = "CallSymbol";
  let RenderMethod = "addImmOperands";
  let DiagnosticType = "InvalidCallSymbol";
  let ParserMethod = "parseCallSymbol";
}

def call_symbol : Operand<XLenVT> {
  let ParserMatchClass = CallSymbol;
}

def PseudoJumpSymbol : AsmOperandClass {
  let Name = "PseudoJumpSymbol";
  let RenderMethod = "addImmOperands";
  let DiagnosticType = "InvalidPseudoJumpSymbol";
  let ParserMethod = "parsePseudoJumpSymbol";
}

def pseudo_jump_symbol : Operand<XLenVT> {
  let ParserMatchClass = PseudoJumpSymbol;
}

def ixlenimm : Operand<XLenVT>;

def uimm6gt32 : ImmLeaf<XLenVT, [{
  return isUInt<6>(Imm) && Imm > 32;
}]>;

def AddrRegImm : ComplexPattern<iPTR, 2, "SelectAddrRegImm">;

def ImmSub32 : SDNodeXForm<imm, [{
  return CurDAG->getTargetConstant(N->getSExtValue() - 32, SDLoc(N), N->getValueType(0));
}]>;

def ImmSubFrom32 : SDNodeXForm<imm, [{
  return CurDAG->getTargetConstant(32 - N->getZExtValue(), SDLoc(N), N->getValueType(0));
}]>;

def TrailingZeros : SDNodeXForm<imm, [{
  return CurDAG->getTargetConstant(llvm::countr_zero(N->getZExtValue()), SDLoc(N), N->getValueType(0));
}]>;

def XLenSubTrailingOnes : SDNodeXForm<imm, [{
  uint64_t XLen = Subtarget->getXLen();
  uint64_t TrailingOnes = llvm::countr_one(N->getZExtValue());
  return CurDAG->getTargetConstant(XLen - TrailingOnes, SDLoc(N), N->getValueType(0));
}]>;

def LeadingOnesMask : PatLeaf<(imm), [{
  if (!N->hasOneUse())
    return false;
  return !isInt<32>(N->getSExtValue()) && isMask_64(~N->getSExtValue());
}], TrailingZeros>;

def TrailingOnesMask : PatLeaf<(imm), [{
  if (!N->hasOneUse())
    return false;
  return !isInt<12>(N->getSExtValue()) && isMask_64(N->getZExtValue());
}], XLenSubTrailingOnes>;

def LeadingOnesWMask : PatLeaf<(imm), [{
  if (!N->hasOneUse())
    return false;
  int64_t Imm = N->getSExtValue();
  return !isInt<32>(Imm) && isUInt<32>(Imm) && isShiftedMask_64(Imm) && Imm != UINT64_C(0xffffffff);
}], TrailingZeros>;


//===----------------------------------------------------------------------===//
// Instruction Formats
//===----------------------------------------------------------------------===//

include "RISCXInstrFormats.td"


//===----------------------------------------------------------------------===//
// Instruction Class Templates
//===----------------------------------------------------------------------===//

let hasSideEffects = 0, mayLoad = 0, mayStore = 0 in
class BranchCC_rri<bits<3> funct3, string opcodestr>
    : RXInstB<funct3, OPC_BRANCH, (outs), (ins GPR:$rs1, GPR:$rs2, simm13_lsb0:$imm12), opcodestr, "$rs1, $rs2, $imm12"> {
  let isBranch = 1;
  let isTerminator = 1;
}

let hasSideEffects = 0, mayLoad = 1, mayStore = 0 in {
class Load_ri<bits<3> funct3, string opcodestr>
    : RXInstI<funct3, OPC_LOAD, (outs GPR:$rd), (ins GPRMem:$rs1, simm12:$imm12), opcodestr, "$rd, ${imm12}(${rs1})">;
}

let hasSideEffects = 0, mayLoad = 0, mayStore = 1 in {
class Store_rri<bits<3> funct3, string opcodestr>
    : RXInstS<funct3, OPC_STORE, (outs), (ins GPR:$rs2, GPRMem:$rs1, simm12:$imm12), opcodestr, "$rs2, ${imm12}(${rs1})">;
}

let hasSideEffects = 0, mayLoad = 0, mayStore = 0 in
class ALU_ri<bits<3> funct3, string opcodestr>
    : RXInstI<funct3, OPC_OP_IMM, (outs GPR:$rd), (ins GPR:$rs1, simm12:$imm12), opcodestr, "$rd, $rs1, $imm12">;

let hasSideEffects = 0, mayLoad = 0, mayStore = 0 in
class Shift_ri<bits<5> imm11_7, bits<3> funct3, string opcodestr>
    : RXInstIShift<imm11_7, funct3, OPC_OP_IMM, (outs GPR:$rd), (ins GPR:$rs1, uimmlog2xlen:$shamt), opcodestr, "$rd, $rs1, $shamt">;

let hasSideEffects = 0, mayLoad = 0, mayStore = 0 in
class ALU_rr<bits<7> funct7, bits<3> funct3, string opcodestr, bit Commutable = 0>
    : RXInstR<funct7, funct3, OPC_OP, (outs GPR:$rd), (ins GPR:$rs1, GPR:$rs2), opcodestr, "$rd, $rs1, $rs2"> {
  let isCommutable = Commutable;
}

let hasSideEffects = 0, mayLoad = 0, mayStore = 0 in
class ShiftW_ri<bits<7> imm11_5, bits<3> funct3, string opcodestr>
    : RXInstIShiftW<imm11_5, funct3, OPC_OP_IMM_32, (outs GPR:$rd), (ins GPR:$rs1, uimm5:$shamt), opcodestr, "$rd, $rs1, $shamt">;

let hasSideEffects = 0, mayLoad = 0, mayStore = 0 in
class ALUW_rr<bits<7> funct7, bits<3> funct3, string opcodestr, bit Commutable = 0>
    : RXInstR<funct7, funct3, OPC_OP_32, (outs GPR:$rd), (ins GPR:$rs1, GPR:$rs2), opcodestr, "$rd, $rs1, $rs2"> {
  let isCommutable = Commutable;
}


//===----------------------------------------------------------------------===//
// Instructions
//===----------------------------------------------------------------------===//

let hasSideEffects = 0, mayLoad = 0, mayStore = 0 in {
let isReMaterializable = 1, isAsCheapAsAMove = 1 in
def LUI : RXInstU<OPC_LUI, (outs GPR:$rd), (ins uimm20_lui:$imm20), "lui", "$rd, $imm20">;
def AUIPC : RXInstU<OPC_AUIPC, (outs GPR:$rd), (ins uimm20_auipc:$imm20), "auipc", "$rd, $imm20">;
def JAL : RXInstJ<OPC_JAL, (outs GPR:$rd), (ins simm21_lsb0_jal:$imm20), "jal", "$rd, $imm20">;
def JALR : RXInstI<0b000, OPC_JALR, (outs GPR:$rd), (ins GPR:$rs1, simm12:$imm12), "jalr", "$rd, ${imm12}(${rs1})">;
} // hasSideEffects = 0, mayLoad = 0, mayStore = 0

def BEQ  : BranchCC_rri<0b000, "beq">;
def BNE  : BranchCC_rri<0b001, "bne">;
def BLT  : BranchCC_rri<0b100, "blt">;
def BGE  : BranchCC_rri<0b101, "bge">;
def BLTU : BranchCC_rri<0b110, "bltu">;
def BGEU : BranchCC_rri<0b111, "bgeu">;

def LB  : Load_ri<0b000, "lb">;
def LH  : Load_ri<0b001, "lh">;
def LW  : Load_ri<0b010, "lw">;
def LBU : Load_ri<0b100, "lbu">;
def LHU : Load_ri<0b101, "lhu">;

def SB : Store_rri<0b000, "sb">;
def SH : Store_rri<0b001, "sh">;
def SW : Store_rri<0b010, "sw">;

let isReMaterializable = 1, isAsCheapAsAMove = 1 in
def ADDI  : ALU_ri<0b000, "addi">;

def SLTI  : ALU_ri<0b010, "slti">;
def SLTIU : ALU_ri<0b011, "sltiu">;

let isReMaterializable = 1, isAsCheapAsAMove = 1 in {
def XORI  : ALU_ri<0b100, "xori">;
def ORI   : ALU_ri<0b110, "ori">;
}

def ANDI  : ALU_ri<0b111, "andi">;

def SLLI : Shift_ri<0b00000, 0b001, "slli">;
def SRLI : Shift_ri<0b00000, 0b101, "srli">;
def SRAI : Shift_ri<0b01000, 0b101, "srai">;

def ADD  : ALU_rr<0b0000000, 0b000, "add", Commutable=1>;
def SUB  : ALU_rr<0b0100000, 0b000, "sub">;
def SLL  : ALU_rr<0b0000000, 0b001, "sll">;

def SLT  : ALU_rr<0b0000000, 0b010, "slt">;
def SLTU : ALU_rr<0b0000000, 0b011, "sltu">;

def XOR  : ALU_rr<0b0000000, 0b100, "xor", Commutable=1>;
def SRL  : ALU_rr<0b0000000, 0b101, "srl">;
def SRA  : ALU_rr<0b0100000, 0b101, "sra">;
def OR   : ALU_rr<0b0000000, 0b110, "or", Commutable=1>;
def AND  : ALU_rr<0b0000000, 0b111, "and", Commutable=1>;

def MUL     : ALU_rr<0b0000001, 0b000, "mul", Commutable=1>;
def MULH    : ALU_rr<0b0000001, 0b001, "mulh", Commutable=1>;
def MULHSU  : ALU_rr<0b0000001, 0b010, "mulhsu">;
def MULHU   : ALU_rr<0b0000001, 0b011, "mulhu", Commutable=1>;
def DIV     : ALU_rr<0b0000001, 0b100, "div">;
def DIVU    : ALU_rr<0b0000001, 0b101, "divu">;
def REM     : ALU_rr<0b0000001, 0b110, "rem">;
def REMU    : ALU_rr<0b0000001, 0b111, "remu">;
def MULW    : ALUW_rr<0b0000001, 0b000, "mulw", Commutable=1>;
def DIVW    : ALUW_rr<0b0000001, 0b100, "divw">;
def DIVUW   : ALUW_rr<0b0000001, 0b101, "divuw">;
def REMW    : ALUW_rr<0b0000001, 0b110, "remw">;
def REMUW   : ALUW_rr<0b0000001, 0b111, "remuw">;

// Sim put pixel
def PUT_PIXEL : RXInstGraphics<OPC_PUT_PIXEL, (outs), (ins GPR:$rs1, GPR:$rs2, GPR:$rd),
                       "put.pixel", "$rs1 $rs2 $rd", [(int_riscx_putpixel GPR:$rs1, GPR:$rs2, GPR:$rd)]>;

// Sim flush
let rs1 = 0, rs2 = 0, rd = 0 in
def FLUSH : RXInstGraphics<OPC_FLUSH, (outs), (ins), "flush", "", [(int_riscx_flush)]>;

// Sim rand
let rs1 = 0, rs2 = 0 in
def RAND : RXInstGraphics<OPC_RAND, (outs GPR:$rd), (ins), "rand", "$rd", [(set GPR:$rd, (int_riscx_rand))]>;

def LWU   : Load_ri<0b110, "lwu">;
def LD    : Load_ri<0b011, "ld">;
def SD    : Store_rri<0b011, "sd">;

def ADDIW : RXInstI<0b000, OPC_OP_IMM_32, (outs GPR:$rd), (ins GPR:$rs1, simm12:$imm12), "addiw", "$rd, $rs1, $imm12">;

def SLLIW : ShiftW_ri<0b0000000, 0b001, "slliw">;
def SRLIW : ShiftW_ri<0b0000000, 0b101, "srliw">;
def SRAIW : ShiftW_ri<0b0100000, 0b101, "sraiw">;

def ADDW  : ALUW_rr<0b0000000, 0b000, "addw", Commutable=1>;
def SUBW  : ALUW_rr<0b0100000, 0b000, "subw">;
def SLLW  : ALUW_rr<0b0000000, 0b001, "sllw">;
def SRLW  : ALUW_rr<0b0000000, 0b101, "srlw">;
def SRAW  : ALUW_rr<0b0100000, 0b101, "sraw">;

//===----------------------------------------------------------------------===//
// Aliases
//===----------------------------------------------------------------------===//

def : InstAlias<"nop",           (ADDI X0, X0, 0)>;

def : InstAlias<"li $rd, $imm",  (ADDI GPR:$rd, X0, simm12:$imm)>;
def : InstAlias<"mv $rd, $rs",   (ADDI GPR:$rd, GPR:$rs, 0)>;
def : InstAlias<"not $rd, $rs",  (XORI GPR:$rd, GPR:$rs, -1)>;
def : InstAlias<"neg $rd, $rs",  (SUB GPR:$rd, X0, GPR:$rs)>;

def : InstAlias<"negw $rd, $rs",   (SUBW GPR:$rd, X0, GPR:$rs)>;
def : InstAlias<"sext.w $rd, $rs", (ADDIW GPR:$rd, GPR:$rs, 0)>;

def : InstAlias<"seqz $rd, $rs", (SLTIU GPR:$rd, GPR:$rs, 1)>;
def : InstAlias<"snez $rd, $rs", (SLTU GPR:$rd, X0, GPR:$rs)>;
def : InstAlias<"sltz $rd, $rs", (SLT GPR:$rd, GPR:$rs, X0)>;
def : InstAlias<"sgtz $rd, $rs", (SLT GPR:$rd, X0, GPR:$rs)>;

def : InstAlias<"sgt $rd, $rs, $rt", (SLT GPR:$rd, GPR:$rt, GPR:$rs), 0>;
def : InstAlias<"sgtu $rd, $rs, $rt", (SLTU GPR:$rd, GPR:$rt, GPR:$rs), 0>;

def : InstAlias<"beqz $rs, $offset", (BEQ GPR:$rs, X0, simm13_lsb0:$offset)>;
def : InstAlias<"bnez $rs, $offset", (BNE GPR:$rs, X0, simm13_lsb0:$offset)>;
def : InstAlias<"blez $rs, $offset", (BGE X0, GPR:$rs, simm13_lsb0:$offset)>;
def : InstAlias<"bgez $rs, $offset", (BGE GPR:$rs, X0, simm13_lsb0:$offset)>;
def : InstAlias<"bltz $rs, $offset", (BLT GPR:$rs, X0, simm13_lsb0:$offset)>;
def : InstAlias<"bgtz $rs, $offset", (BLT X0, GPR:$rs, simm13_lsb0:$offset)>;

def : InstAlias<"bgt $rs, $rt, $offset", (BLT GPR:$rt, GPR:$rs, simm13_lsb0:$offset), 0>;
def : InstAlias<"ble $rs, $rt, $offset", (BGE GPR:$rt, GPR:$rs, simm13_lsb0:$offset), 0>;
def : InstAlias<"bgtu $rs, $rt, $offset", (BLTU GPR:$rt, GPR:$rs, simm13_lsb0:$offset), 0>;
def : InstAlias<"bleu $rs, $rt, $offset", (BGEU GPR:$rt, GPR:$rs, simm13_lsb0:$offset), 0>;

def : InstAlias<"j $offset",   (JAL X0, simm21_lsb0_jal:$offset)>;
def : InstAlias<"jal $offset", (JAL X1, simm21_lsb0_jal:$offset)>;

def : InstAlias<"jr $rs",                (JALR X0, GPR:$rs, 0), 3>;
def : InstAlias<"jr ${offset}(${rs})",   (JALR X0, GPR:$rs, simm12:$offset)>;
def : InstAlias<"jalr $rs",              (JALR X1, GPR:$rs, 0), 3>;
def : InstAlias<"jalr ${offset}(${rs})", (JALR X1, GPR:$rs, simm12:$offset)>;
def : InstAlias<"jalr $rd, $rs",         (JALR GPR:$rd, GPR:$rs, 0), 2>;
def : InstAlias<"ret",                   (JALR X0, X1, 0), 4>;

def : InstAlias<"jr $rs, $offset",        (JALR X0, GPR:$rs, simm12:$offset), 0>;
def : InstAlias<"jalr $rs, $offset",      (JALR X1, GPR:$rs, simm12:$offset), 0>;
def : InstAlias<"jalr $rd, $rs, $offset", (JALR GPR:$rd, GPR:$rs, simm12:$offset), 0>;

let EmitPriority = 0 in {
def : InstAlias<"lb $rd, (${rs1})", (LB GPR:$rd, GPR:$rs1, 0)>;
def : InstAlias<"lh $rd, (${rs1})", (LH GPR:$rd, GPR:$rs1, 0)>;
def : InstAlias<"lw $rd, (${rs1})", (LW GPR:$rd, GPR:$rs1, 0)>;
def : InstAlias<"lbu $rd, (${rs1})", (LBU GPR:$rd, GPR:$rs1, 0)>;
def : InstAlias<"lhu $rd, (${rs1})", (LHU GPR:$rd, GPR:$rs1, 0)>;

def : InstAlias<"sb $rs2, (${rs1})", (SB GPR:$rs2, GPR:$rs1, 0)>;
def : InstAlias<"sh $rs2, (${rs1})", (SH GPR:$rs2, GPR:$rs1, 0)>;
def : InstAlias<"sw $rs2, (${rs1})", (SW GPR:$rs2, GPR:$rs1, 0)>;

def : InstAlias<"add $rd, $rs1, $imm12", (ADDI GPR:$rd, GPR:$rs1, simm12:$imm12)>;
def : InstAlias<"and $rd, $rs1, $imm12", (ANDI GPR:$rd, GPR:$rs1, simm12:$imm12)>;
def : InstAlias<"xor $rd, $rs1, $imm12", (XORI GPR:$rd, GPR:$rs1, simm12:$imm12)>;
def : InstAlias<"or $rd, $rs1, $imm12", (ORI GPR:$rd, GPR:$rs1, simm12:$imm12)>;
def : InstAlias<"sll $rd, $rs1, $shamt", (SLLI GPR:$rd, GPR:$rs1, uimmlog2xlen:$shamt)>;
def : InstAlias<"srl $rd, $rs1, $shamt", (SRLI GPR:$rd, GPR:$rs1, uimmlog2xlen:$shamt)>;
def : InstAlias<"sra $rd, $rs1, $shamt", (SRAI GPR:$rd, GPR:$rs1, uimmlog2xlen:$shamt)>;

def : InstAlias<"lwu $rd, (${rs1})", (LWU GPR:$rd, GPR:$rs1, 0)>;
def : InstAlias<"ld $rd, (${rs1})", (LD GPR:$rd, GPR:$rs1, 0)>;
def : InstAlias<"sd $rs2, (${rs1})", (SD GPR:$rs2, GPR:$rs1, 0)>;

def : InstAlias<"addw $rd, $rs1, $imm12", (ADDIW GPR:$rd, GPR:$rs1, simm12:$imm12)>;
def : InstAlias<"sllw $rd, $rs1, $shamt", (SLLIW GPR:$rd, GPR:$rs1, uimm5:$shamt)>;
def : InstAlias<"srlw $rd, $rs1, $shamt", (SRLIW GPR:$rd, GPR:$rs1, uimm5:$shamt)>;
def : InstAlias<"sraw $rd, $rs1, $shamt", (SRAIW GPR:$rd, GPR:$rs1, uimm5:$shamt)>;

def : InstAlias<"slt $rd, $rs1, $imm12", (SLTI GPR:$rd, GPR:$rs1, simm12:$imm12)>;
def : InstAlias<"sltu $rd, $rs1, $imm12", (SLTIU GPR:$rd, GPR:$rs1, simm12:$imm12)>;
}


//===----------------------------------------------------------------------===//
// Patterns
//===----------------------------------------------------------------------===//

class PatGpr<SDPatternOperator OpNode, RXInst Inst, ValueType vt = XLenVT>
    : Pat<(vt (OpNode (vt GPR:$rs1))), (Inst GPR:$rs1)>;

class PatGprGpr<SDPatternOperator OpNode, RXInst Inst, ValueType vt1 = XLenVT, ValueType vt2 = XLenVT>
    : Pat<(vt1 (OpNode (vt1 GPR:$rs1), (vt2 GPR:$rs2))), (Inst GPR:$rs1, GPR:$rs2)>;

class PatGprImm<SDPatternOperator OpNode, RXInst Inst, ImmLeaf ImmType, ValueType vt = XLenVT>
    : Pat<(vt (OpNode (vt GPR:$rs1), ImmType:$imm)), (Inst GPR:$rs1, ImmType:$imm)>;

class PatGprSimm12<SDPatternOperator OpNode, RXInstI Inst>
    : PatGprImm<OpNode, Inst, simm12>;

class PatGprUimmLog2XLen<SDPatternOperator OpNode, RXInstIShift Inst>
    : PatGprImm<OpNode, Inst, uimmlog2xlen>;


def assertsexti32 : PatFrag<(ops node:$src), (assertsext node:$src), [{
  return cast<VTSDNode>(N->getOperand(1))->getVT().bitsLE(MVT::i32);
}]>;
def sexti16 : ComplexPattern<XLenVT, 1, "selectSExtBits<16>">;
def sexti32 : ComplexPattern<i64, 1, "selectSExtBits<32>">;
def assertzexti32 : PatFrag<(ops node:$src), (assertzext node:$src), [{
  return cast<VTSDNode>(N->getOperand(1))->getVT().bitsLE(MVT::i32);
}]>;
def zexti32 : ComplexPattern<i64, 1, "selectZExtBits<32>">;
def zexti16 : ComplexPattern<XLenVT, 1, "selectZExtBits<16>">;
def zexti16i32 : ComplexPattern<i32, 1, "selectZExtBits<16>">;
def zexti8 : ComplexPattern<XLenVT, 1, "selectZExtBits<8>">;
def zexti8i32 : ComplexPattern<i32, 1, "selectZExtBits<8>">;

def ext : PatFrags<(ops node:$A), [(sext node:$A), (zext node:$A)]>;

class binop_oneuse<SDPatternOperator operator>
    : PatFrag<(ops node:$A, node:$B), (operator node:$A, node:$B), [{
  return N->hasOneUse();
}]>;

class unop_oneuse<SDPatternOperator operator>
    : PatFrag<(ops node:$A), (operator node:$A), [{
  return N->hasOneUse();
}]>;

def sext_oneuse   : unop_oneuse<sext>;
def zext_oneuse   : unop_oneuse<zext>;
def anyext_oneuse : unop_oneuse<anyext>;
def ext_oneuse    : unop_oneuse<ext>;

def riscx_setne : ComplexPattern<XLenVT, 1, "selectSETNE", [setcc]>;
def riscx_seteq : ComplexPattern<XLenVT, 1, "selectSETEQ", [setcc]>;

def shiftMaskXLen : ComplexPattern<XLenVT, 1, "selectShiftMaskXLen", [], [], 0>;
def shiftMask32   : ComplexPattern<i64, 1, "selectShiftMask32", [], [], 0>;

class shiftop<SDPatternOperator operator>
    : PatFrag<(ops node:$val, node:$count), (operator node:$val, (XLenVT (shiftMaskXLen node:$count)))>;
class shiftopw<SDPatternOperator operator>
    : PatFrag<(ops node:$val, node:$count), (operator node:$val, (i64 (shiftMask32 node:$count)))>;

/// Branches and jumps

multiclass BccPat<CondCode Cond, RXInstB Inst> {
  def : Pat<(riscx_brcc (XLenVT GPR:$rs1), GPR:$rs2, Cond, bb:$imm12), (Inst GPR:$rs1, GPR:$rs2, simm13_lsb0:$imm12)>;
  def : Pat<(riscx_brcc (XLenVT GPR:$rs1), 0, Cond, bb:$imm12), (Inst GPR:$rs1, (XLenVT X0), simm13_lsb0:$imm12)>;
}

defm : BccPat<SETEQ, BEQ>;
defm : BccPat<SETNE, BNE>;
defm : BccPat<SETLT, BLT>;
defm : BccPat<SETGE, BGE>;
defm : BccPat<SETULT, BLTU>;
defm : BccPat<SETUGE, BGEU>;

let isBarrier = 1, isBranch = 1, isTerminator = 1 in
def PseudoBR : Pseudo<(outs), (ins simm21_lsb0_jal:$imm20), [(br bb:$imm20)]>, PseudoInstExpansion<(JAL X0, simm21_lsb0_jal:$imm20)>;

let isCall = 1, Defs = [X1], isCodeGenOnly = 0, Size = 8 in
def PseudoCALL : Pseudo<(outs), (ins call_symbol:$func), [], "call", "$func">;

let isBarrier = 1, isReturn = 1, isTerminator = 1 in
def PseudoRET : Pseudo<(outs), (ins), [(riscx_ret_glue)]>, PseudoInstExpansion<(JALR X0, X1, 0)>;

let isCall = 0, isBarrier = 1, isBranch = 1, isTerminator = 1, Size = 8, isCodeGenOnly = 0, hasSideEffects = 0, mayStore = 0, mayLoad = 0 in
def PseudoJump : Pseudo<(outs GPR:$rd), (ins pseudo_jump_symbol:$target), [], "jump", "$target, $rd">;

let hasSideEffects = 0, mayLoad = 1, mayStore = 0, Size = 8, isCodeGenOnly = 0, isAsmParserOnly = 1 in
def PseudoLGA : Pseudo<(outs GPR:$dst), (ins bare_symbol:$src), [], "lga", "$dst, $src">;


class LdPat<PatFrag LoadOp, RXInst Inst, ValueType vt = XLenVT>
    : Pat<(vt (LoadOp (AddrRegImm (XLenVT GPR:$rs1), simm12:$imm12))), (Inst GPR:$rs1, simm12:$imm12)>;

class StPat<PatFrag StoreOp, RXInst Inst, RegisterClass StTy, ValueType vt>
    : Pat<(StoreOp (vt StTy:$rs2), (AddrRegImm (XLenVT GPR:$rs1), simm12:$imm12)), (Inst StTy:$rs2, GPR:$rs1, simm12:$imm12)>;

/// Other pseudo-instructions

let Defs = [X2], Uses = [X2] in {
def ADJCALLSTACKDOWN : Pseudo<(outs), (ins i32imm:$amt1, i32imm:$amt2), [(callseq_start timm:$amt1, timm:$amt2)]>;
def ADJCALLSTACKUP   : Pseudo<(outs), (ins i32imm:$amt1, i32imm:$amt2), [(callseq_end timm:$amt1, timm:$amt2)]>;
} // Defs = [X2], Uses = [X2]


class binop_allwusers<SDPatternOperator operator>
    : PatFrag<(ops node:$lhs, node:$rhs), (i64 (operator node:$lhs, node:$rhs)), [{
  return hasAllWUsers(Node);
}]>;

def sexti32_allwusers : PatFrag<(ops node:$src),
                                (sext_inreg node:$src, i32), [{
  return hasAllWUsers(Node);
}]>;

def ImmSExt32 : SDNodeXForm<imm, [{
  return CurDAG->getTargetConstant(SignExtend64<32>(N->getSExtValue()),
                                   SDLoc(N), N->getValueType(0));
}]>;

def u32simm12 : ImmLeaf<XLenVT, [{
  return isUInt<32>(Imm) && isInt<12>(SignExtend64<32>(Imm));
}], ImmSExt32>;


def simm12i32 : ImmLeaf<i32, [{return isInt<12>(Imm);}]>;

def as_i64imm : SDNodeXForm<imm, [{
  return CurDAG->getTargetConstant(N->getSExtValue(), SDLoc(N), MVT::i64);
}]>;

def zext_is_sext : PatFrag<(ops node:$src), (zext node:$src), [{
  KnownBits Known = CurDAG->computeKnownBits(N->getOperand(0), 0);
  return Known.isNonNegative();
}]>;


/// Arithmetic

def : PatGprGpr<add, ADD>;
def : PatGprSimm12<add, ADDI>;
def : PatGprGpr<sub, SUB>;
def : PatGprGpr<or, OR>;
def : PatGprSimm12<or, ORI>;
def : PatGprGpr<and, AND>;
def : PatGprSimm12<and, ANDI>;
def : PatGprGpr<xor, XOR>;
def : PatGprSimm12<xor, XORI>;
def : PatGprUimmLog2XLen<shl, SLLI>;
def : PatGprUimmLog2XLen<srl, SRLI>;
def : PatGprUimmLog2XLen<sra, SRAI>;
def : PatGprGpr<mul, MULW, i32, i32>;
def : PatGprGpr<sdiv, DIVW, i32, i32>;
def : PatGprGpr<udiv, DIVUW, i32, i32>;
def : PatGprGpr<srem, REMW, i32, i32>;
def : PatGprGpr<urem, REMUW, i32, i32>;
def : PatGprGpr<shiftop<shl>, SLL>;
def : PatGprGpr<shiftop<srl>, SRL>;
def : PatGprGpr<shiftop<sra>, SRA>;
def : PatGprGpr<setlt, SLT>;
def : PatGprSimm12<setlt, SLTI>;
def : PatGprGpr<setult, SLTU>;
def : PatGprSimm12<setult, SLTIU>;
def : PatGprGpr<shiftopw<riscx_sllw>, SLLW>;
def : PatGprGpr<shiftopw<riscx_srlw>, SRLW>;
def : PatGprGpr<shiftopw<riscx_sraw>, SRAW>;
def : PatGprGpr<binop_allwusers<add>, ADDW>;
def : PatGprSimm12<binop_allwusers<add>, ADDIW>;
def : PatGprGpr<binop_allwusers<sub>, SUBW>;
def : PatGprImm<binop_allwusers<shl>, SLLIW, uimm5>;
def : PatGprGpr<add, ADDW, i32, i32>;
def : PatGprGpr<sub, SUBW, i32, i32>;
def : PatGprGpr<and, AND, i32, i32>;
def : PatGprGpr<or, OR, i32, i32>;
def : PatGprGpr<xor, XOR, i32, i32>;
def : PatGprGpr<shiftopw<shl>, SLLW, i32, i64>;
def : PatGprGpr<shiftopw<srl>, SRLW, i32, i64>;
def : PatGprGpr<shiftopw<sra>, SRAW, i32, i64>;
def : PatGprImm<shl, SLLIW, uimm5, i32>;
def : PatGprImm<srl, SRLIW, uimm5, i32>;
def : PatGprImm<sra, SRAIW, uimm5, i32>;
def : PatGprGpr<mul, MUL>;
def : PatGprGpr<mulhs, MULH>;
def : PatGprGpr<mulhu, MULHU>;
def : PatGprGpr<riscx_mulhsu, MULHSU>;
def : PatGprGpr<sdiv, DIV>;
def : PatGprGpr<udiv, DIVU>;
def : PatGprGpr<srem, REM>;
def : PatGprGpr<urem, REMU>;
def : PatGprGpr<binop_allwusers<mul>, MULW>;
def : PatGprGpr<riscx_divw, DIVW>;
def : PatGprGpr<riscx_divuw, DIVUW>;
def : PatGprGpr<riscx_remuw, REMUW>;

/// Loads

def : LdPat<sextloadi8, LB>;
def : LdPat<extloadi8, LBU>;
def : LdPat<sextloadi16, LH>;
def : LdPat<extloadi16, LH>;
def : LdPat<load, LW, i32>;
def : LdPat<zextloadi8, LBU>;
def : LdPat<zextloadi16, LHU>;
def : LdPat<sextloadi32, LW, i64>;
def : LdPat<extloadi32, LW, i64>;
def : LdPat<zextloadi32, LWU, i64>;
def : LdPat<load, LD, i64>;
def : LdPat<sextloadi8, LB, i32>;
def : LdPat<extloadi8, LBU, i32>;
def : LdPat<sextloadi16, LH, i32>;
def : LdPat<extloadi16, LH, i32>;
def : LdPat<zextloadi8, LBU, i32>;
def : LdPat<zextloadi16, LHU, i32>;

/// Stores

def : StPat<truncstorei8, SB, GPR, XLenVT>;
def : StPat<truncstorei16, SH, GPR, XLenVT>;
def : StPat<store, SW, GPR, i32>;
def : StPat<truncstorei32, SW, GPR, i64>;
def : StPat<store, SD, GPR, i64>;
def : StPat<truncstorei8, SB, GPR, i32>;
def : StPat<truncstorei16, SH, GPR, i32>;

/// Miscellaneous

def : Pat<(i64 (and GPR:$rs, LeadingOnesMask:$mask)), (SLLI (SRLI $rs, LeadingOnesMask:$mask), LeadingOnesMask:$mask)>;
def : Pat<(XLenVT (and GPR:$rs, TrailingOnesMask:$mask)), (SRLI (SLLI $rs, TrailingOnesMask:$mask), TrailingOnesMask:$mask)>;

def : Pat<(riscx_hi tglobaladdr:$in), (LUI tglobaladdr:$in)>;
def : Pat<(riscx_hi tblockaddress:$in), (LUI tblockaddress:$in)>;
def : Pat<(riscx_hi tjumptable:$in), (LUI tjumptable:$in)>;
def : Pat<(riscx_hi tconstpool:$in), (LUI tconstpool:$in)>;

def : Pat<(riscx_add_lo GPR:$hi, tglobaladdr:$lo), (ADDI GPR:$hi, tglobaladdr:$lo)>;
def : Pat<(riscx_add_lo GPR:$hi, tblockaddress:$lo), (ADDI GPR:$hi, tblockaddress:$lo)>;
def : Pat<(riscx_add_lo GPR:$hi, tjumptable:$lo), (ADDI GPR:$hi, tjumptable:$lo)>;
def : Pat<(riscx_add_lo GPR:$hi, tconstpool:$lo), (ADDI GPR:$hi, tconstpool:$lo)>;

def : Pat<(riscx_seteq (XLenVT GPR:$rs1)), (SLTIU GPR:$rs1, 1)>;
def : Pat<(riscx_setne (XLenVT GPR:$rs1)), (SLTU (XLenVT X0), GPR:$rs1)>;
def : Pat<(XLenVT (setne (XLenVT GPR:$rs1), -1)), (SLTIU GPR:$rs1, -1)>;

def : Pat<(riscx_call tglobaladdr:$func), (PseudoCALL tglobaladdr:$func)>;
def : Pat<(riscx_call texternalsym:$func), (PseudoCALL texternalsym:$func)>;

def : Pat<(i64 (and GPR:$rs1, 0xffffffff)), (SRLI (SLLI GPR:$rs1, 32), 32)>;
def : Pat<(i64 (shl (and GPR:$rs1, 0xffffffff), uimm5:$shamt)), (SRLI (SLLI GPR:$rs1, 32), (ImmSubFrom32 uimm5:$shamt))>;
def : Pat<(i64 (and GPR:$rs, LeadingOnesWMask:$mask)), (SLLI (SRLIW $rs, LeadingOnesWMask:$mask), LeadingOnesWMask:$mask)>;

def : Pat<(sexti32_allwusers GPR:$rs1), (XLenVT GPR:$rs1)>;
def : Pat<(sext_inreg GPR:$rs1, i32), (ADDIW GPR:$rs1, 0)>;

def : Pat<(i64 (srl (and GPR:$rs1, 0xffffffff), uimm5:$shamt)), (SRLIW GPR:$rs1, uimm5:$shamt)>;
def : Pat<(i64 (srl (shl GPR:$rs1, (i64 32)), uimm6gt32:$shamt)), (SRLIW GPR:$rs1, (ImmSub32 uimm6gt32:$shamt))>;
def : Pat<(sra (sext_inreg GPR:$rs1, i32), uimm5:$shamt), (SRAIW GPR:$rs1, uimm5:$shamt)>;
def : Pat<(i64 (sra (shl GPR:$rs1, (i64 32)), uimm6gt32:$shamt)), (SRAIW GPR:$rs1, (ImmSub32 uimm6gt32:$shamt))>;

def : Pat<(binop_allwusers<srl> (sext_inreg GPR:$rs1, i32), uimm5:$shamt), (SRAIW GPR:$rs1, uimm5:$shamt)>;
def : Pat<(binop_allwusers<and> GPR:$rs1, u32simm12:$imm), (ANDI GPR:$rs1, u32simm12:$imm)>;
def : Pat<(binop_allwusers<or> GPR:$rs1, u32simm12:$imm), (ORI GPR:$rs1, u32simm12:$imm)>;
def : Pat<(binop_allwusers<xor> GPR:$rs1, u32simm12:$imm), (XORI GPR:$rs1, u32simm12:$imm)>;

def : Pat<(anyext GPR:$src), (COPY GPR:$src)>;
def : Pat<(sext GPR:$src), (ADDIW GPR:$src, 0)>;
def : Pat<(trunc GPR:$src), (COPY GPR:$src)>;

def : Pat<(i32 (add GPR:$rs1, simm12i32:$imm)), (ADDIW GPR:$rs1, (i64 (as_i64imm $imm)))>;
def : Pat<(i32 (and GPR:$rs1, simm12i32:$imm)), (ANDI GPR:$rs1, (i64 (as_i64imm $imm)))>;
def : Pat<(i32 (or GPR:$rs1, simm12i32:$imm)), (ORI GPR:$rs1, (i64 (as_i64imm $imm)))>;
def : Pat<(i32 (xor GPR:$rs1, simm12i32:$imm)), (XORI GPR:$rs1, (i64 (as_i64imm $imm)))>;

def : Pat<(i32 (and GPR:$rs, TrailingOnesMask:$mask)), (SRLI (SLLI $rs, (i64 (XLenSubTrailingOnes $mask))), (i64 (XLenSubTrailingOnes $mask)))>;
def : Pat<(zext_is_sext GPR:$src), (ADDIW GPR:$src, 0)>;
def : Pat<(zext GPR:$src), (SRLI (SLLI GPR:$src, 32), 32)>;
def : Pat<(shl (zext GPR:$rs), uimm5:$shamt), (SRLI (SLLI GPR:$rs, 32), (ImmSubFrom32 uimm5:$shamt))>;

def : Pat<(and (riscx_divuw (assertzexti32 GPR:$rs1), (assertzexti32 GPR:$rs2)), 0xffffffff), (DIVU GPR:$rs1, GPR:$rs2)>;
def : Pat<(and (riscx_remuw (assertzexti32 GPR:$rs1), (assertzexti32 GPR:$rs2)), 0xffffffff), (REMU GPR:$rs1, GPR:$rs2)>;
def : Pat<(srem (sexti32 (i64 GPR:$rs1)), (sexti32 (i64 GPR:$rs2))), (REMW GPR:$rs1, GPR:$rs2)>;
def : Pat<(i64 (mul (and GPR:$rs1, 0xffffffff), (and GPR:$rs2, 0xffffffff))), (MULHU (SLLI GPR:$rs1, 32), (SLLI GPR:$rs2, 32))>;