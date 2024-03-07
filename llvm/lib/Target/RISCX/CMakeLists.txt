add_llvm_component_group(RISCX)

add_llvm_target(RISCXCodeGen
  RISCXTargetMachine.cpp

  LINK_COMPONENTS
  CodeGen
  Core
  Support
  Target
  RISCXInfo

  ADD_TO_COMPONENT
  RISCX
  )

add_subdirectory(MCTargetDesc)
add_subdirectory(TargetInfo)

