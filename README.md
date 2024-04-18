# LLVM Backend for RISC-X

## Build
### Execute these commands:
```
cmake -S llvm -B build -G Ninja -DLLVM_ENABLE_PROJECTS='clang' -DCMAKE_BUILD_TYPE=Release -DLLVM_INCLUDE_TESTS=OFF -DLLVM_INCLUDE_DOCS=OFF -DLLVM_INCLUDE_EXAMPLES=OFF -DLLVM_INCLUDE_BENCHMARKS=OFF -DLLVM_TARGETS_TO_BUILD=RISCX
```
```
cmake --build build -j 8
```

### Or you can call `make_riscx.sh` script which does exactly these lines:
```
./make_riscx.sh
```

## Testing
### You can find some tests for backend in folder `riscx-tests`
### They include frontend and backend tests
### Frontend translates custom language `*.lang` to LLVM IR files `*.ll`. Backend converts LLVM IR `*.ll` files to machine code binary files. These can be simulated through Emulator.
### Here is the biggest part of the whole pipeline (excluding frontend by now):
```
./build/bin/llc ./riscx-tests/backend/App.ll -o App.o -march=riscx -filetype=obj
```
```
ld.lld App.o -o App --entry=app
```

### Now you have binary ELF file which can be executed on the Emulator

```
git submodule update --init --recursive
cd Emulator
cmake -B build -DCMAKE_BUILD_TYPE=Release .
cmake --build build -j 8
```

```
cd build
```
```
./risc-v ../../App
```


### Compiling and simulating other tests is exactly the same

## In case you want to read instructions by yourself, you can also produce human-readable assembly file:
```
./build/bin/llc ./riscx-tests/backend/App.ll -o App.s -march=riscx -filetype=asm
```
```
cat App.s
```

## Running full pipeline
### You can run full pipeline of `Frontend -> LLVM IR -> Backend -> Machine Code -> Binary file -> Simulator` by executing `full_pipeline.sh`:
```
./full_pipeline.sh
```