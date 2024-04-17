let Namespace = "RISCX" in {
class RISCXReg<bits<5> Enc, string n, list<string> alt = []> : Register<n> {
  let HWEncoding{4-0} = Enc;
  let AltNames = alt;
}

let FallbackRegAltNameIndex = NoRegAltName in
def ABIRegAltName : RegAltNameIndex;
} // Namespace = "RISCX"


let RegAltNameIndices = [ABIRegAltName] in {
  let isConstant = true in
  def X0  : RISCXReg<0, "x0",  ["zero"]>;
  def X1  : RISCXReg<1, "x1",  ["ra"]>;
  def X2  : RISCXReg<2, "x2",  ["sp"]>;
  def X3  : RISCXReg<3, "x3",  ["gp"]>;
  def X4  : RISCXReg<4, "x4",  ["tp"]>;
  def X5  : RISCXReg<5, "x5",  ["t0"]>;
  def X6  : RISCXReg<6, "x6",  ["t1"]>;
  def X7  : RISCXReg<7, "x7",  ["t2"]>;
  def X8  : RISCXReg<8, "x8",  ["s0"]>;
  def X9  : RISCXReg<9, "x9",  ["s1"]>;
  def X10 : RISCXReg<10,"x10", ["a0"]>;
  def X11 : RISCXReg<11,"x11", ["a1"]>;
  def X12 : RISCXReg<12,"x12", ["a2"]>;
  def X13 : RISCXReg<13,"x13", ["a3"]>;
  def X14 : RISCXReg<14,"x14", ["a4"]>;
  def X15 : RISCXReg<15,"x15", ["a5"]>;
  def X16 : RISCXReg<16,"x16", ["a6"]>;
  def X17 : RISCXReg<17,"x17", ["a7"]>;
  def X18 : RISCXReg<18,"x18", ["s2"]>;
  def X19 : RISCXReg<19,"x19", ["s3"]>;
  def X20 : RISCXReg<20,"x20", ["s4"]>;
  def X21 : RISCXReg<21,"x21", ["s5"]>;
  def X22 : RISCXReg<22,"x22", ["s6"]>;
  def X23 : RISCXReg<23,"x23", ["s7"]>;
  def X24 : RISCXReg<24,"x24", ["s8"]>;
  def X25 : RISCXReg<25,"x25", ["s9"]>;
  def X26 : RISCXReg<26,"x26", ["s10"]>;
  def X27 : RISCXReg<27,"x27", ["s11"]>;
  def X28 : RISCXReg<28,"x28", ["t3"]>;
  def X29 : RISCXReg<29,"x29", ["t4"]>;
  def X30 : RISCXReg<30,"x30", ["t5"]>;
  def X31 : RISCXReg<31,"x31", ["t6"]>;
}

def XLenVT : ValueTypeByHwMode<[RX64], [i64]>;
def XLenRI : RegInfoByHwMode<[RX64], [RegInfo<64, 64, 64>]>;

class GPRRegisterClass<dag regList> : RegisterClass<"RISCX", [XLenVT, i32], 32, regList> {
  let RegInfos = XLenRI;
}

def GPR : GPRRegisterClass<(add (sequence "X%u", 10, 17),
                                (sequence "X%u", 5, 7),
                                (sequence "X%u", 28, 31),
                                (sequence "X%u", 8, 9),
                                (sequence "X%u", 18, 27),
                                (sequence "X%u", 0, 4))>;