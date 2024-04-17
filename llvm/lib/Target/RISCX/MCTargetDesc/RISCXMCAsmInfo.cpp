#include "RISCXMCAsmInfo.h"

using namespace llvm;

RISCXMCAsmInfo::RISCXMCAsmInfo(const Triple &TheTriple) {
  CodePointerSize = CalleeSaveStackSlotSize = 8;
  CommentString = ";";
  AlignmentIsInBytes = false;
  SupportsDebugInformation = true;
  Data16bitsDirective = "\t.half\t";
  Data32bitsDirective = "\t.word\t";
  Data64bitsDirective = "\t.dword\t";
  ExceptionsType = ExceptionHandling::None;
}