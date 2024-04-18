class InstFormat<bits<5> val> {
  bits<5> Value = val;
}
def InstFormatPseudo : InstFormat<0>;
def InstFormatR      : InstFormat<1>;
def InstFormatI      : InstFormat<2>;
def InstFormatS      : InstFormat<3>;
def InstFormatB      : InstFormat<4>;
def InstFormatU      : InstFormat<5>;
def InstFormatJ      : InstFormat<6>;
def InstFormatOther  : InstFormat<7>;

class RISCXOpcode<string name, bits<7> val> {
  string Name = name;
  bits<7> Value = val;
}

def OPC_LOAD      : RISCXOpcode<"LOAD",       0b0000011>;
def OPC_MISC_MEM  : RISCXOpcode<"MISC_MEM",   0b0001111>;
def OPC_OP_IMM    : RISCXOpcode<"OP_IMM",     0b0010011>;
def OPC_AUIPC     : RISCXOpcode<"AUIPC",      0b0010111>;
def OPC_OP_IMM_32 : RISCXOpcode<"OP_IMM_32",  0b0011011>;
def OPC_STORE     : RISCXOpcode<"STORE",      0b0100011>;
def OPC_OP        : RISCXOpcode<"OP",         0b0110011>;
def OPC_LUI       : RISCXOpcode<"LUI",        0b0110111>;
def OPC_OP_32     : RISCXOpcode<"OP_32",      0b0111011>;
def OPC_BRANCH    : RISCXOpcode<"BRANCH",     0b1100011>;
def OPC_JALR      : RISCXOpcode<"JALR",       0b1100111>;
def OPC_JAL       : RISCXOpcode<"JAL",        0b1101111>;
def OPC_OP_P      : RISCXOpcode<"OP_P",       0b1110111>;
def OPC_PUT_PIXEL : RISCXOpcode<"PUT_PIXEL",  0b0101011>;
def OPC_FLUSH     : RISCXOpcode<"FLUSH",      0b1011011>;
def OPC_RAND      : RISCXOpcode<"RAND",       0b1111011>;

class RXInst<RISCXOpcode opcode, dag outs, dag ins, string opcodestr, string argstr, list<dag> pattern, InstFormat format> : Instruction {
  let Namespace = "RISCX";

  dag OutOperandList = outs;
  dag InOperandList = ins;
  let AsmString = opcodestr # !if(!empty(argstr), "", "\t" # argstr);
  let Pattern = pattern;

  let TSFlags{4-0} = format.Value;

  field bits<32> Inst;
  let Inst{6-0} = opcode.Value;
  let Size = 4;
}

class Pseudo<dag outs, dag ins, list<dag> pattern, string opcodestr = "", string argstr = "">
    : RXInst<RISCXOpcode<"", 0>, outs, ins, opcodestr, argstr, pattern, InstFormatPseudo> {
  let isPseudo = 1;
  let isCodeGenOnly = 1;
}

class RXInstRBase<bits<3> funct3, RISCXOpcode opcode, dag outs, dag ins, string opcodestr, string argstr>
    : RXInst<opcode, outs, ins, opcodestr, argstr, [], InstFormatR> {
  bits<5> rs2;
  bits<5> rs1;
  bits<5> rd;

  let Inst{24-20} = rs2;
  let Inst{19-15} = rs1;
  let Inst{14-12} = funct3;
  let Inst{11-7} = rd;
}

class RXInstR<bits<7> funct7, bits<3> funct3, RISCXOpcode opcode, dag outs, dag ins, string opcodestr, string argstr>
    : RXInstRBase<funct3, opcode, outs, ins, opcodestr, argstr> {
  let Inst{31-25} = funct7;
}


class RXInstIBase<bits<3> funct3, RISCXOpcode opcode, dag outs, dag ins, string opcodestr, string argstr>
    : RXInst<opcode, outs, ins, opcodestr, argstr, [], InstFormatI> {
  bits<5> rs1;
  bits<5> rd;

  let Inst{19-15} = rs1;
  let Inst{14-12} = funct3;
  let Inst{11-7} = rd;
}

class RXInstI<bits<3> funct3, RISCXOpcode opcode, dag outs, dag ins, string opcodestr, string argstr>
    : RXInstIBase<funct3, opcode, outs, ins, opcodestr, argstr> {
  bits<12> imm12;

  let Inst{31-20} = imm12;
}

class RXInstIShift<bits<5> imm11_7, bits<3> funct3, RISCXOpcode opcode, dag outs, dag ins, string opcodestr, string argstr>
    : RXInstIBase<funct3, opcode, outs, ins, opcodestr, argstr> {
  bits<6> shamt;

  let Inst{31-27} = imm11_7;
  let Inst{26} = 0;
  let Inst{25-20} = shamt;
}

class RXInstIShiftW<bits<7> imm11_5, bits<3> funct3, RISCXOpcode opcode, dag outs, dag ins, string opcodestr, string argstr>
    : RXInstIBase<funct3, opcode, outs, ins, opcodestr, argstr> {
  bits<5> shamt;

  let Inst{31-25} = imm11_5;
  let Inst{24-20} = shamt;
}

class RXInstS<bits<3> funct3, RISCXOpcode opcode, dag outs, dag ins, string opcodestr, string argstr>
    : RXInst<opcode, outs, ins, opcodestr, argstr, [], InstFormatS> {
  bits<12> imm12;
  bits<5> rs2;
  bits<5> rs1;

  let Inst{31-25} = imm12{11-5};
  let Inst{24-20} = rs2;
  let Inst{19-15} = rs1;
  let Inst{14-12} = funct3;
  let Inst{11-7} = imm12{4-0};
}

class RXInstB<bits<3> funct3, RISCXOpcode opcode, dag outs, dag ins, string opcodestr, string argstr>
    : RXInst<opcode, outs, ins, opcodestr, argstr, [], InstFormatB> {
  bits<12> imm12;
  bits<5> rs2;
  bits<5> rs1;

  let Inst{31} = imm12{11};
  let Inst{30-25} = imm12{9-4};
  let Inst{24-20} = rs2;
  let Inst{19-15} = rs1;
  let Inst{14-12} = funct3;
  let Inst{11-8} = imm12{3-0};
  let Inst{7} = imm12{10};
}

class RXInstU<RISCXOpcode opcode, dag outs, dag ins, string opcodestr, string argstr>
    : RXInst<opcode, outs, ins, opcodestr, argstr, [], InstFormatU> {
  bits<20> imm20;
  bits<5> rd;

  let Inst{31-12} = imm20;
  let Inst{11-7} = rd;
}

class RXInstJ<RISCXOpcode opcode, dag outs, dag ins, string opcodestr, string argstr>
    : RXInst<opcode, outs, ins, opcodestr, argstr, [], InstFormatJ> {
  bits<20> imm20;
  bits<5> rd;

  let Inst{31} = imm20{19};
  let Inst{30-21} = imm20{9-0};
  let Inst{20} = imm20{10};
  let Inst{19-12} = imm20{18-11};
  let Inst{11-7} = rd;
}