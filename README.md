# SIMD

A small header-only SIMD library extracted from Maths Town projects.

It currently provides native fallback, SSE, AVX2, and AVX-512 vector types for common
`float`, `double`, signed integer, and unsigned integer operations.

This repository is being prepared as a standalone open source library for reuse
in other projects (including via git subtree).

## Compilers SPecific Notes
Transcendental math operations well implemented on MSVC by using the SVML library.  However other compilers use a simple fallback mechanism where lanes are unpacked and the std library is used.  I will try to implement these operations one day.

### Microsoft C++ Compiler
  Transcendental math operations such as `sin()`, `cos()`, `tan()`, `log()`, and `exp()` use the SVML library so are fully simd.

### Clang
  SVML is not available.  Transcendental math operations such as `sin()`, `cos()`, `tan()`, `log()`, and `exp()` use my code, which are currently just slow fallbacks. 

### GCC
  SVML is not available.  Transcendental math operations such as `sin()`, `cos()`, `tan()`, `log()`, and `exp()` use my code, which are currently just slow fallbacks. 


## Testing Suite
If you wish to run tests, these are currently setup to run under PowerShell for Windows.  There are tests for 3 windows compilers: MSVC, gcc and clang (as clang-cl).

Windows (PowerShell), from the repository root (default generator/compiler on your machine):

```powershell
cmake -S Tests -B Build
cmake --build Build --config Release
.\Build\Release\simd_test.exe
```

### Test Microsoft C++ Compiler

Windows (PowerShell), explicitly with MSVC (`cl`):

```powershell
cmake -S Tests -B Build\msvc -G "Visual Studio 17 2022" -A x64
cmake --build Build\msvc --config Release
.\Build\msvc\Release\simd_test.exe
```

### Test Clang (clang-cl) C++ Compiler

Windows (PowerShell), explicitly with Clang (`clang-cl`):

```powershell
cmake -S Tests -B Build\clang -G "Visual Studio 17 2022" -A x64 -T ClangCL
cmake --build Build\clang --config Release
.\Build\clang\Release\simd_test.exe
```

### Test GCC C++ Compiler 
GCC really does not like to see intrinsics from higher levels in the code, even if they are unreachable. There is a separate compilation mode to test each level of support, to make sure I don't rely on runtime checks for this in GCC.  (Whereas other compilers we just check at runtime)

Windows (PowerShell), GCC level 1 mode (128 bit types, `SSE2` minimum):

```powershell
cmake -S Tests -B Build\gcc-sse2 -G "MinGW Makefiles" -DCMAKE_CXX_COMPILER=g++ -DSIMD_TEST_MODE=SSE2
cmake --build Build\gcc-sse2
.\Build\gcc-sse2\simd_test.exe
```

Windows (PowerShell), GCC level 3 mode (256 bit types, `AVX2` minimum):

```powershell
cmake -S Tests -B Build\gcc-avx2 -G "MinGW Makefiles" -DCMAKE_CXX_COMPILER=g++ -DSIMD_TEST_MODE=AVX2
cmake --build Build\gcc-avx2
.\Build\gcc-avx2\simd_test.exe
```

Windows (PowerShell), GCC level 4 mode (512 bit types, `AVX-512f and AVX-512dq` minimum):

```powershell
cmake -S Tests -B Build\gcc-avx512 -G "MinGW Makefiles" -DCMAKE_CXX_COMPILER=g++ -DSIMD_TEST_MODE=AVX512
cmake --build Build\gcc-avx512
.\Build\gcc-avx512\simd_test.exe
```

Use AVX-512 run mode only on CPUs that support AVX-512.
