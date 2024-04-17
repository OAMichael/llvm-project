add_llvm_component_group(RISCX)

set(LLVM_TARGET_DEFINITIONS RISCX.td)

tablegen(LLVM RISCXGenAsmWriter.inc -gen-asm-writer)
tablegen(LLVM RISCXGenDAGISel.inc -gen-dag-isel)
tablegen(LLVM RISCXGenInstrInfo.inc -gen-instr-info)
tablegen(LLVM RISCXGenMCCodeEmitter.inc -gen-emitter)
tablegen(LLVM RISCXGenMCPseudoLowering.inc -gen-pseudo-lowering)
tablegen(LLVM RISCXGenRegisterInfo.inc -gen-register-info)
tablegen(LLVM RISCXGenSubtargetInfo.inc -gen-subtarget)

add_public_tablegen_target(RISCXCommonTableGen)

add_llvm_target(RISCXCodeGen
  RISCXTargetMachine.cpp
  RISCXRegisterInfo.cpp
  RISCXInstrInfo.cpp
  RISCXSubtarget.cpp
  RISCXAsmPrinter.cpp
  RISCXFrameLowering.cpp
  RISCXISelDAGToDAG.cpp
  RISCXISelLowering.cpp

  LINK_COMPONENTS
  Analysis
  AsmPrinter
  CodeGen
  Core
  MC
  RISCXDesc
  SelectionDAG
  Support
  Target
  RISCXInfo
  GlobalISel

  ADD_TO_COMPONENT
  RISCX
  )

add_subdirectory(MCTargetDesc)
add_subdirectory(TargetInfo)
