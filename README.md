# SIMD

A small header-only SIMD library extracted from Maths Town projects.

It currently provides fallback, SSE, AVX2, and AVX-512 vector types for common
`float`, `double`, signed integer, and unsigned integer operations.

This repository is being prepared as a standalone open source library for reuse
in other projects (including via git subtree).

## Math Backends
The trancedental math function such as sin() cos() tan() log() exp() etc require a backend library.

Microsft C++ Compiler 
    - SVML is built-in (matches Intel documentation).  No additional library required.

GCC/Clang 
    - No built-in.  Sleef is recommended, and should be included as a single header. (see instructions)
    - A fallback mode is available.  This simply unpacks vectors and sends them to the standard library.


Select one backend with a compile definition:

- `MT_USE_SVML=1`
- `MT_USE_SLEEF=1`
- `MT_USE_LIBC_FALLBACK=1`

Default behavior:
- MSVC builds use `MT_USE_SVML=1` (no external math library required).
- GCC/Clang builds use `MT_USE_LIBC_FALLBACK=1` (portable fallback).

### GCC/Clang with SLEEF (inline headers)

SLEEF can be used as header-only inline math on GCC/Clang.

```powershell
git clone https://github.com/shibatch/sleef.git
cmake -S sleef -B sleef\build -DSLEEF_BUILD_INLINE_HEADERS=TRUE -DCMAKE_INSTALL_PREFIX="$PWD\sleef\install"
cmake --build sleef\build --config Release --target install
```

Then compile your target with:

- `MT_USE_SLEEF=1`
- Include path to `sleef\install\include`
- `-ffp-contract=off` (required by SLEEF inline headers)

This project expects these headers in the include path:

- `sleefinline_sse2.h`
- `sleefinline_avx2128.h` and `sleefinline_avx2.h` (for 256 types)
- `sleefinline_avx512f.h` (for 512 types)

For tests, this can now be automated by CMake (no manual SLEEF install needed).
Set `SIMD_TEST_USE_SLEEF=ON` and CMake will download/build SLEEF into the build tree.

### GCC/Clang without SLEEF

Use `MT_USE_LIBC_FALLBACK=1` (or rely on the default). This works without extra
libraries, but transcendental SIMD operations are slower because lanes are
unpacked and mapped through standard library calls.

## Running Tests

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
GCC really does not like to see intrinsics from higher levels in the code, even if they are unreachable. There is a separate compilation mode to test each level of support.  This is a nice check because it ensure invalid intrisics are not buried in the code.

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

Windows (PowerShell), GCC level 3 mode with auto-downloaded SLEEF:

```powershell
cmake -S Tests -B Build\gcc-avx2-sleef -G "MinGW Makefiles" -DCMAKE_CXX_COMPILER=g++ -DSIMD_TEST_MODE=AVX2 -DSIMD_TEST_USE_SLEEF=ON
cmake --build Build\gcc-avx2-sleef
.\Build\gcc-avx2-sleef\simd_test.exe
```

Optional: choose a SLEEF branch/tag with `-DSIMD_TEST_SLEEF_GIT_TAG=<tag>`.
If needed, set `-DSED_COMMAND=<path-to-sed.exe>` (SLEEF uses `sed` to generate inline headers).
Auto-SLEEF test mode is supported for `SIMD_TEST_MODE=AVX2` or `AVX512`.

Windows (PowerShell), GCC level 4 mode (512 bit types, `AVX-512f and AVX-512dq` minimum):

```powershell
cmake -S Tests -B Build\gcc-avx512 -G "MinGW Makefiles" -DCMAKE_CXX_COMPILER=g++ -DSIMD_TEST_MODE=AVX512
cmake --build Build\gcc-avx512
.\Build\gcc-avx512\simd_test.exe
```

Use AVX-512 run mode only on CPUs that support AVX-512.
