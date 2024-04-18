#include "RISCX.h"
#include "clang/Basic/Builtins.h"
#include "clang/Basic/MacroBuilder.h"
#include "clang/Basic/TargetBuiltins.h"

using namespace clang;
using namespace clang::targets;

static constexpr Builtin::Info BuiltinInfo[] = {
#define BUILTIN(ID, TYPE, ATTRS)                                               \
  {#ID, TYPE, ATTRS, nullptr, HeaderDesc::NO_HEADER, ALL_LANGUAGES},
#include "clang/Basic/BuiltinsRISCX.def"
};

void RISCXTargetInfo::getTargetDefines(const LangOptions &Opts, MacroBuilder &Builder) const {
  Builder.defineMacro("__riscx__");
}

ArrayRef<Builtin::Info> RISCXTargetInfo::getTargetBuiltins() const {
  return llvm::ArrayRef(BuiltinInfo, clang::RISCX::LastTSBuiltin - Builtin::FirstTSBuiltin);
}