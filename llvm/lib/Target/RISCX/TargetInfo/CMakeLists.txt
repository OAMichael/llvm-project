add_llvm_component_library(LLVMRISCXInfo
  RISCXTargetInfo.cpp

  LINK_COMPONENTS
  MC
  Support

  ADD_TO_COMPONENT
  RISCX
  )
