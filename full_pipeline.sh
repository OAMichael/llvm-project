#!/usr/bin/bash

SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )

TEST_APP="App"

if [ ! -z "$1" ]
  then
  TEST_APP=$1
fi

echo "Starting the pipeline for ${TEST_APP}..."

# Compile *.lang to LLVM IR:
echo "Compiling custom language to LLVM IR..."

cd ${SCRIPT_DIR}/LLVM-Course/LLVM_Language

bison -d MyLang.y &>/dev/null
lex MyLang.lex
clang++ lex.yy.c MyLang.tab.c `llvm-config --cppflags --ldflags --libs` -lSDL2 -o MyLang -I/usr/include/SDL2 &>/dev/null
cat ${SCRIPT_DIR}/riscx-tests/frontend/${TEST_APP}.lang | ./MyLang > ${SCRIPT_DIR}/riscx-tests/backend/${TEST_APP}.ll

# Convert LLVM IR to machine code using LLVM backend:
echo "Converting LLVM IR to machine code..."

cd ${SCRIPT_DIR}
mkdir -p riscx-tests/binary
./build/bin/llc riscx-tests/backend/${TEST_APP}.ll -o riscx-tests/binary/${TEST_APP}.o -march=riscx -filetype=obj
ld.lld riscx-tests/binary/${TEST_APP}.o -o riscx-tests/binary/${TEST_APP} --entry=app
rm riscx-tests/binary/${TEST_APP}.o

# Run binary ELF file on the Emulator:
echo "Running the simulation on the Emulator..."

cd ${SCRIPT_DIR}/Emulator/build
./risc-v ${SCRIPT_DIR}/riscx-tests/binary/${TEST_APP}
