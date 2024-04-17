include "llvm/Target/Target.td"

defvar RX64 = DefaultMode;

include "RISCXRegisterInfo.td"
include "RISCXCallingConv.td"
include "RISCXInstrInfo.td"


class Proc<string Name, list<SubtargetFeature> Features>
 : Processor<Name, NoItineraries, Features>;

def : Proc<"generic", []>;


def RISCXAsmWriter : AsmWriter {
  int PassSubtarget = 1;
}

def RISCXInstrInfo : InstrInfo;
def RISCX : Target {
  let InstructionSet = RISCXInstrInfo;
  let AssemblyWriters = [RISCXAsmWriter];
}