add_llvm_component_library(LLVMRISCXDesc
  RISCXMCExpr.cpp
  RISCXMatInt.cpp
  RISCXMCTargetDesc.cpp
  RISCXMCAsmInfo.cpp
  RISCXInstPrinter.cpp
  RISCXMCCodeEmitter.cpp
  RISCXAsmBackend.cpp
  RISCXELFObjectWriter.cpp

  LINK_COMPONENTS
  MC
  Support
  RISCXInfo

  ADD_TO_COMPONENT
  RISCX
  )
