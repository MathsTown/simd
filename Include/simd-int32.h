/********************************************************************************************************

Authors:		(c) 2023 Maths Town

Licence:		The MIT License

*********************************************************************************************************
Permission is hereby granted, free of charge, to any person obtaining a copy of this software and
associated documentation files (the "Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the
following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial
portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT
LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
********************************************************************************************************

Basic SIMD Types for 32-bit Signed Integers:

FallbackInt64		- Works on all build modes and CPUs.  Forwards most requests to the standard library.

Simd128Int64		- x86_64 Microarchitecture Level 1 - Works on all x86_64 CPUs.
					- Requires SSE and SSE2 support.  Will use SSE4.1 instructions when __SSE4_1__ or __AVX__ defined.

Simd256Int64		- x86_64 Microarchitecture Level 3.
					- Requires AVX, AVX2 and FMA support.

Simd512Int64		- x86_64 Microarchitecture Level 4.
					- Requires AVX512F, AVX512DQ, ACX512VL, AVX512CD, AVX512BW

SimdNativeInt64	- A Typedef referring to one of the above types.  Chosen based on compiler support/mode.
					- Just use this type in your code if you are building for a specific platform.


Checking CPU Support:
Unless you are using a SimdNative typedef, you must check for CPU support before using any of these types.
- MSVC - You may check at runtime or compile time.  (compile time checks generally results in much faster code)
- GCC/Clang - You must check at compile time (due to compiler limitations)

Types reqpresenting floats, doubles, ints, longs etc are arranged in microarchitecture level groups.
Generally CPUs have more SIMD support for floats than ints (and 32 bit is better than 64-bit).
Ensure the CPU supports the full "level" if you need to use more than one type.


To check support at compile time:
	- Use compiler_level_supported()
	- If you won't use any of the type conversion functions you can use compiler_supported()
	- For GCC you may need pre-processor defines to gate the calling site.

To check support at run time:
	- Use cpu_level_supported()
	- If you won't use any of the type conversion functions you can use cpu_supported()

Runtime detection notes:
Visual studio will support compiling all types and switching at runtime. However this often results in slower
code than compiling with full compiler support.  Visual studio will optimise AVX code well when build support is enabled.
If you are able, I recommend distributing code at different support levels. (1,3,4). Let the user choose which to download,
or your installer can make the switch.  It is also possible to dynamically load different .dlls

WASM Support:
I've included FallbackFloat32 for use with Emscripen, but use SimdNativeFloat32 as SIMD support will be added soon.


*********************************************************************************************************/
#pragma once


#include <stdint.h>
#include <algorithm>
#include <cstring>
#include "simd-cpuid.h"
#include "simd-concepts.h"

/**************************************************************************************************
* Fallback Int32 type.
* Contains 1 x 32bit Signed Integer
*
* This is a fallback for cpus that are not yet supported.
*
* It may be useful to evaluate a single value from the same code base, as it offers the same interface
* as the SIMD types.
*
* ************************************************************************************************/
struct FallbackInt32 {
	int32_t v;
	typedef int32_t F;
	FallbackInt32() = default;
	FallbackInt32(int32_t a) : v(a) {};

	//*****Support Informtion*****

	//Performs a runtime CPU check to see if this type is supported.  Checks this type ONLY (integers in same the same level may not be supported) 
	static bool cpu_supported() { return true; }

	//Performs a runtime CPU check to see if this type's microarchitecture level is supported.  (This will ensure that referernced integer types are also supported)
	static bool cpu_level_supported() { return true; }


#if MT_SIMD_ARCH_X64
	//Performs a runtime CPU check to see if this type is supported.  Checks this type ONLY (integers in same the same level may not be supported) 
	static bool cpu_supported(CpuInformation) { return true; }

	//Performs a runtime CPU check to see if this type's microarchitecture level is supported.  (This will ensure that referernced integer types are also supported)
	static bool cpu_level_supported(CpuInformation) { return true; }
#endif

	//Performs a compile time CPU check to see if this type is supported.  Checks this type ONLY (integers in same the same level may not be supported) 
	static constexpr bool compiler_supported() {
		return true;
	}

	//Performs a compile time support to see if the microarchitecture level is supported.  (This will ensure that referernced integer types are also supported)
	static constexpr bool compiler_level_supported() {
		return true;
	}

	//*****Elements*****
	static constexpr int size_of_element() { return sizeof(int32_t); }
	static constexpr int number_of_elements() { return 1; }
	F element(int) { return v; }
	void set_element(int, F value) { v = value; }

	//*****Make Functions****
	static FallbackInt32 make_sequential(int32_t first) { return FallbackInt32(first); }


	//*****Addition Operators*****
	FallbackInt32& operator+=(const FallbackInt32& rhs) noexcept { v += rhs.v; return *this; }
	FallbackInt32& operator+=(int32_t rhs) noexcept { v += rhs; return *this; }

	//*****Subtraction Operators*****
	FallbackInt32& operator-=(const FallbackInt32& rhs) noexcept { v -= rhs.v; return *this; }
	FallbackInt32& operator-=(int32_t rhs) noexcept { v -= rhs; return *this; }

	//*****Multiplication Operators*****
	FallbackInt32& operator*=(const FallbackInt32& rhs) noexcept { v *= rhs.v; return *this; }
	FallbackInt32& operator*=(int32_t rhs) noexcept { v *= rhs; return *this; }

	//*****Division Operators*****
	FallbackInt32& operator/=(const FallbackInt32& rhs) noexcept { v /= rhs.v; return *this; }
	FallbackInt32& operator/=(int32_t rhs) noexcept { v /= rhs;	return *this; }

	//*****Negate Operators*****
	FallbackInt32 operator-() const noexcept { return FallbackInt32(-v); }

	//*****Bitwise Logic Operators*****
	FallbackInt32& operator&=(const FallbackInt32& rhs) noexcept { v &= rhs.v; return *this; }
	FallbackInt32& operator|=(const FallbackInt32& rhs) noexcept { v |= rhs.v; return *this; }
	FallbackInt32& operator^=(const FallbackInt32& rhs) noexcept { v ^= rhs.v; return *this; }

	//*****Mathematical*****

};

//*****Addition Operators*****
inline static FallbackInt32 operator+(FallbackInt32  lhs, const FallbackInt32& rhs) noexcept { lhs += rhs; return lhs; }
inline static FallbackInt32 operator+(FallbackInt32  lhs, int32_t rhs) noexcept { lhs += rhs; return lhs; }
inline static FallbackInt32 operator+(int32_t lhs, FallbackInt32 rhs) noexcept { rhs += lhs; return rhs; }

//*****Subtraction Operators*****
inline static FallbackInt32 operator-(FallbackInt32  lhs, const FallbackInt32& rhs) noexcept { lhs -= rhs; return lhs; }
inline static FallbackInt32 operator-(FallbackInt32  lhs, int32_t rhs) noexcept { lhs -= rhs; return lhs; }
inline static FallbackInt32 operator-(const int32_t lhs, FallbackInt32 rhs) noexcept { rhs.v = lhs - rhs.v; return rhs; }

//*****Multiplication Operators*****
inline static FallbackInt32 operator*(FallbackInt32  lhs, const FallbackInt32& rhs) noexcept { lhs *= rhs; return lhs; }
inline static FallbackInt32 operator*(FallbackInt32  lhs, int32_t rhs) noexcept { lhs *= rhs; return lhs; }
inline static FallbackInt32 operator*(int32_t lhs, FallbackInt32 rhs) noexcept { rhs *= lhs; return rhs; }

//*****Division Operators*****
inline static FallbackInt32 operator/(FallbackInt32  lhs, const FallbackInt32& rhs) noexcept { lhs /= rhs;	return lhs; }
inline static FallbackInt32 operator/(FallbackInt32  lhs, int32_t rhs) noexcept { lhs /= rhs; return lhs; }
inline static FallbackInt32 operator/(const int32_t lhs, FallbackInt32 rhs) noexcept { rhs.v = lhs / rhs.v; return rhs; }


//*****Bitwise Logic Operators*****
inline static FallbackInt32 operator&(const FallbackInt32& lhs, const FallbackInt32& rhs) noexcept { return FallbackInt32(lhs.v & rhs.v); }
inline static FallbackInt32 operator|(const FallbackInt32& lhs, const FallbackInt32& rhs) noexcept { return FallbackInt32(lhs.v | rhs.v); }
inline static FallbackInt32 operator^(const FallbackInt32& lhs, const FallbackInt32& rhs) noexcept { return FallbackInt32(lhs.v ^ rhs.v); }
inline static FallbackInt32 operator~(FallbackInt32 lhs) noexcept { return FallbackInt32(~lhs.v); }


//*****Shifting Operators*****
inline static FallbackInt32 operator<<(FallbackInt32 lhs, int bits) noexcept { lhs.v <<= bits; return lhs; }
inline static FallbackInt32 operator>>(FallbackInt32 lhs, int bits) noexcept { lhs.v >>= bits; return lhs; }

//*****Min/Max*****
inline static FallbackInt32 min(FallbackInt32 a, FallbackInt32 b) { return FallbackInt32(std::min(a.v, b.v)); }
inline static FallbackInt32 max(FallbackInt32 a, FallbackInt32 b) { return FallbackInt32(std::max(a.v, b.v)); }


//*****Math Operators*****
inline static FallbackInt32 abs(FallbackInt32 a) { return FallbackInt32(std::abs(a.v)); }





//***************** x86_64 only code ******************
#if MT_SIMD_ARCH_X64
#include <immintrin.h>

namespace mt::simd_detail_i32 {
	// Portability layer: GCC/Clang cannot index SIMD lanes via MSVC vector members.
	inline int32_t lane_get(__m128i v, int i) noexcept {
#if MT_SIMD_HAS_MSVC_VECTOR_MEMBERS
		return v.m128i_i32[i];
#else
		alignas(16) int32_t lanes[4];
		_mm_storeu_si128(reinterpret_cast<__m128i*>(lanes), v);
		return lanes[i];
#endif
	}
	inline __m128i lane_set(__m128i v, int i, int32_t value) noexcept {
#if MT_SIMD_HAS_MSVC_VECTOR_MEMBERS
		v.m128i_i32[i] = value;
		return v;
#else
		alignas(16) int32_t lanes[4];
		_mm_storeu_si128(reinterpret_cast<__m128i*>(lanes), v);
		lanes[i] = value;
		return _mm_loadu_si128(reinterpret_cast<const __m128i*>(lanes));
#endif
	}

	inline int32_t lane_get(__m256i v, int i) noexcept {
#if MT_SIMD_HAS_MSVC_VECTOR_MEMBERS
		return v.m256i_i32[i];
#else
		alignas(32) int32_t lanes[8];
		_mm256_storeu_si256(reinterpret_cast<__m256i*>(lanes), v);
		return lanes[i];
#endif
	}
	inline __m256i lane_set(__m256i v, int i, int32_t value) noexcept {
#if MT_SIMD_HAS_MSVC_VECTOR_MEMBERS
		v.m256i_i32[i] = value;
		return v;
#else
		alignas(32) int32_t lanes[8];
		_mm256_storeu_si256(reinterpret_cast<__m256i*>(lanes), v);
		lanes[i] = value;
		return _mm256_loadu_si256(reinterpret_cast<const __m256i*>(lanes));
#endif
	}

	inline int32_t lane_get(__m512i v, int i) noexcept {
#if MT_SIMD_HAS_MSVC_VECTOR_MEMBERS
		return v.m512i_i32[i];
#else
		alignas(64) int32_t lanes[16];
		std::memcpy(lanes, &v, sizeof(v));
		return lanes[i];
#endif
	}
	inline __m512i lane_set(__m512i v, int i, int32_t value) noexcept {
#if MT_SIMD_HAS_MSVC_VECTOR_MEMBERS
		v.m512i_i32[i] = value;
		return v;
#else
		alignas(64) int32_t lanes[16];
		std::memcpy(lanes, &v, sizeof(v));
		lanes[i] = value;
		__m512i out_v;
		std::memcpy(&out_v, lanes, sizeof(out_v));
		return out_v;
#endif
	}

	// Fallback: integer SIMD divide intrinsics are non-standard across compilers.
	inline __m128i div_epi32(__m128i a, __m128i b) noexcept {
#if MT_SIMD_HAS_MSVC_VECTOR_MEMBERS
		return _mm_div_epi32(a, b);
#else
		alignas(16) int32_t lhs[4], rhs[4], out[4];
		_mm_storeu_si128(reinterpret_cast<__m128i*>(lhs), a);
		_mm_storeu_si128(reinterpret_cast<__m128i*>(rhs), b);
		for (int i = 0; i < 4; ++i) out[i] = lhs[i] / rhs[i];
		return _mm_loadu_si128(reinterpret_cast<const __m128i*>(out));
#endif
	}
	inline __m256i div_epi32(__m256i a, __m256i b) noexcept {
#if MT_SIMD_HAS_MSVC_VECTOR_MEMBERS
		return _mm256_div_epi32(a, b);
#else
		alignas(32) int32_t lhs[8], rhs[8], out[8];
		_mm256_storeu_si256(reinterpret_cast<__m256i*>(lhs), a);
		_mm256_storeu_si256(reinterpret_cast<__m256i*>(rhs), b);
		for (int i = 0; i < 8; ++i) out[i] = lhs[i] / rhs[i];
		return _mm256_loadu_si256(reinterpret_cast<const __m256i*>(out));
#endif
	}
	inline __m512i div_epi32(__m512i a, __m512i b) noexcept {
#if MT_SIMD_HAS_MSVC_VECTOR_MEMBERS
		return _mm512_div_epi32(a, b);
#else
		alignas(64) int32_t lhs[16], rhs[16], out[16];
		std::memcpy(lhs, &a, sizeof(a));
		std::memcpy(rhs, &b, sizeof(b));
		for (int i = 0; i < 16; ++i) out[i] = lhs[i] / rhs[i];
		__m512i out_v;
		std::memcpy(&out_v, out, sizeof(out_v));
		return out_v;
#endif
	}
}



#if MT_SIMD_ALLOW_LEVEL4_TYPES
/**************************************************************************************************
 * SIMD 512 type.  Contains 16 x 32bit Signed Integers
 * Requires AVX-512F
 * ************************************************************************************************/
struct Simd512Int32 {
	__m512i v;
	typedef int32_t F;

	Simd512Int32() = default;
	Simd512Int32(__m512i a) : v(a) {};
	Simd512Int32(F a) : v(_mm512_set1_epi32(a)) {};

	//*****Support Informtion*****
	static bool cpu_supported() {
		CpuInformation cpuid{};
		return cpu_supported(cpuid);
	}
	static bool cpu_supported(CpuInformation cpuid) {
		return cpuid.has_avx512_f();
	}

	//Performs a compile time support. Checks this type ONLY (integers in same class may not be supported) 
	static constexpr bool compiler_supported() {
		return mt::environment::compiler_has_avx512f;
	}

	//Performs a runtime CPU check to see if this type's microarchitecture level is supported.  (This will ensure that referernced integer types are also supported)
	static bool cpu_level_supported() {
		CpuInformation cpuid{};
		return cpu_level_supported(cpuid);
	}

	//Performs a runtime CPU check to see if this type's microarchitecture level is supported.  (This will ensure that referernced integer types are also supported)
	static bool cpu_level_supported(CpuInformation cpuid) {
		return cpuid.has_avx512_f() && cpuid.has_avx512_dq() && cpuid.has_avx512_vl() && cpuid.has_avx512_bw() && cpuid.has_avx512_cd();
	}

	//Performs a compile time support to see if the microarchitecture level is supported.  (This will ensure that referernced integer types are also supported)
	static constexpr bool compiler_level_supported() {
		return mt::environment::compiler_has_avx512f && mt::environment::compiler_has_avx512dq && mt::environment::compiler_has_avx512vl && mt::environment::compiler_has_avx512bw && mt::environment::compiler_has_avx512cd;
	}

	//*****Elements*****
	static constexpr int size_of_element() { return sizeof(int32_t); }
	static constexpr int number_of_elements() { return 16; }
	F element(int i) { return mt::simd_detail_i32::lane_get(v, i); }
	void set_element(int i, F value) { v = mt::simd_detail_i32::lane_set(v, i, value); }

	//*****Make Functions****
	static Simd512Int32 make_sequential(int32_t first) { return Simd512Int32(_mm512_set_epi32(first + 15, first + 14, first + 13, first + 12, first + 11, first + 10, first + 9, first + 8, first + 7, first + 6, first + 5, first + 4, first + 3, first + 2, first + 1, first)); }


	//*****Addition Operators*****
	Simd512Int32& operator+=(const Simd512Int32& rhs) noexcept { v = _mm512_add_epi32(v, rhs.v); return *this; }
	Simd512Int32& operator+=(int32_t rhs) noexcept { v = _mm512_add_epi32(v, _mm512_set1_epi32(rhs));	return *this; }

	//*****Subtraction Operators*****
	Simd512Int32& operator-=(const Simd512Int32& rhs) noexcept { v = _mm512_sub_epi32(v, rhs.v); return *this; }
	Simd512Int32& operator-=(int32_t rhs) noexcept { v = _mm512_sub_epi32(v, _mm512_set1_epi32(rhs));	return *this; }

	//*****Multiplication Operators*****
	Simd512Int32& operator*=(const Simd512Int32& rhs) noexcept { v = _mm512_mullo_epi32(v, rhs.v); return *this; }
	Simd512Int32& operator*=(int32_t rhs) noexcept { v = _mm512_mullo_epi32(v, _mm512_set1_epi32(rhs)); return *this; }

	//*****Division Operators*****
	Simd512Int32& operator/=(const Simd512Int32& rhs) noexcept { v = mt::simd_detail_i32::div_epi32(v, rhs.v); return *this; }
	Simd512Int32& operator/=(int32_t rhs) noexcept {
		if constexpr (mt::environment::compiler_has_avx512f) {
			v = mt::simd_detail_i32::div_epi32(v, _mm512_set1_epi32(rhs));
			return *this;
		}
		else {
			//I don't know why but visual studio linker was hanging when compiling this without AVX.
			//Since we wish to support runtime dispatch in visual studio, we fallback to scaler division in this case.
			//For future investigation.
			v = _mm512_set_epi32(
				mt::simd_detail_i32::lane_get(v, 15) / rhs,
				mt::simd_detail_i32::lane_get(v, 14) / rhs,
				mt::simd_detail_i32::lane_get(v, 13) / rhs,
				mt::simd_detail_i32::lane_get(v, 12) / rhs,
				mt::simd_detail_i32::lane_get(v, 11) / rhs,
				mt::simd_detail_i32::lane_get(v, 10) / rhs,
				mt::simd_detail_i32::lane_get(v, 9) / rhs,
				mt::simd_detail_i32::lane_get(v, 8) / rhs,
				mt::simd_detail_i32::lane_get(v, 7) / rhs,
				mt::simd_detail_i32::lane_get(v, 6) / rhs,
				mt::simd_detail_i32::lane_get(v, 5) / rhs,
				mt::simd_detail_i32::lane_get(v, 4) / rhs,
				mt::simd_detail_i32::lane_get(v, 3) / rhs,
				mt::simd_detail_i32::lane_get(v, 2) / rhs,
				mt::simd_detail_i32::lane_get(v, 1) / rhs,
				mt::simd_detail_i32::lane_get(v, 0) / rhs
			);
			return *this;
		}
	}

	//*****Negate Operators*****
	Simd512Int32 operator-() const noexcept {
		return Simd512Int32(_mm512_sub_epi32(_mm512_setzero_epi32(), v));
	}

	//*****Bitwise Logic Operators*****
	Simd512Int32& operator&=(const Simd512Int32& rhs) noexcept { v = _mm512_and_si512(v, rhs.v); return *this; }
	Simd512Int32& operator|=(const Simd512Int32& rhs) noexcept { v = _mm512_or_si512(v, rhs.v); return *this; }
	Simd512Int32& operator^=(const Simd512Int32& rhs) noexcept { v = _mm512_xor_si512(v, rhs.v); return *this; }




};

//*****Addition Operators*****
inline static Simd512Int32 operator+(Simd512Int32  lhs, const Simd512Int32& rhs) noexcept { lhs += rhs; return lhs; }
inline static Simd512Int32 operator+(Simd512Int32  lhs, int32_t rhs) noexcept { lhs += rhs; return lhs; }
inline static Simd512Int32 operator+(int32_t lhs, Simd512Int32 rhs) noexcept { rhs += lhs; return rhs; }

//*****Subtraction Operators*****
inline static Simd512Int32 operator-(Simd512Int32  lhs, const Simd512Int32& rhs) noexcept { lhs -= rhs; return lhs; }
inline static Simd512Int32 operator-(Simd512Int32  lhs, int32_t rhs) noexcept { lhs -= rhs; return lhs; }
inline static Simd512Int32 operator-(const int32_t lhs, const Simd512Int32& rhs) noexcept { return Simd512Int32(_mm512_sub_epi32(_mm512_set1_epi32(lhs), rhs.v)); }

//*****Multiplication Operators*****
inline static Simd512Int32 operator*(Simd512Int32  lhs, const Simd512Int32& rhs) noexcept { lhs *= rhs; return lhs; }
inline static Simd512Int32 operator*(Simd512Int32  lhs, int32_t rhs) noexcept { lhs *= rhs; return lhs; }
inline static Simd512Int32 operator*(int32_t lhs, Simd512Int32 rhs) noexcept { rhs *= lhs; return rhs; }

//*****Division Operators*****
inline static Simd512Int32 operator/(Simd512Int32  lhs, const Simd512Int32& rhs) noexcept { lhs /= rhs;	return lhs; }
inline static Simd512Int32 operator/(Simd512Int32  lhs, int32_t rhs) noexcept { lhs /= rhs; return lhs; }
inline static Simd512Int32 operator/(const int32_t lhs, const Simd512Int32& rhs) noexcept { return Simd512Int32(mt::simd_detail_i32::div_epi32(_mm512_set1_epi32(lhs), rhs.v)); }


//*****Bitwise Logic Operators*****
inline static Simd512Int32 operator&(Simd512Int32  lhs, const Simd512Int32& rhs) noexcept { lhs &= rhs; return lhs; }
inline static Simd512Int32 operator|(Simd512Int32  lhs, const Simd512Int32& rhs) noexcept { lhs |= rhs; return lhs; }
inline static Simd512Int32 operator^(Simd512Int32  lhs, const Simd512Int32& rhs) noexcept { lhs ^= rhs; return lhs; }
inline static Simd512Int32 operator~(const Simd512Int32& lhs) noexcept { return Simd512Int32(_mm512_xor_epi32(lhs.v, _mm512_set1_epi32(0xFFFFFFFF))); } //No bitwise not in AVX512 so we use xor with a constant..


//*****Shifting Operators*****
inline static Simd512Int32 operator<<(const Simd512Int32& lhs, int bits) noexcept { return Simd512Int32(_mm512_slli_epi32(lhs.v, bits)); }
inline static Simd512Int32 operator>>(const Simd512Int32& lhs, int bits) noexcept { return Simd512Int32(_mm512_srai_epi32(lhs.v, bits)); }


//*****Min/Max*****
inline static Simd512Int32 min(Simd512Int32 a, Simd512Int32 b) { return Simd512Int32(_mm512_min_epi32(a.v, b.v)); }
inline static Simd512Int32 max(Simd512Int32 a, Simd512Int32 b) { return Simd512Int32(_mm512_max_epi32(a.v, b.v)); }


//*****Mathematical*****
inline static Simd512Int32 abs(Simd512Int32 a) { return Simd512Int32(_mm512_abs_epi32(a.v)); }

#endif // MT_SIMD_ALLOW_LEVEL4_TYPES

#if MT_SIMD_ALLOW_LEVEL3_TYPES
/**************************************************************************************************
 * SIMD 256 type.  Contains 8 x 32bit Signed Integers
 * Requires AVX2 support.
 * ************************************************************************************************/
struct Simd256Int32 {
	__m256i v;
	typedef int32_t F;

	Simd256Int32() = default;
	Simd256Int32(__m256i a) : v(a) {};
	Simd256Int32(F a) : v(_mm256_set1_epi32(a)) {};

	//*****Support Informtion*****
	static bool cpu_supported() {
		CpuInformation cpuid{};
		return cpu_supported(cpuid);
	}
	static bool cpu_supported(CpuInformation cpuid) {
		return cpuid.has_avx() && cpuid.has_avx2();
	}

	//Performs a compile time support. Checks this type ONLY (integers in same class may not be supported) 
	static constexpr bool compiler_supported() {
		return mt::environment::compiler_has_avx && mt::environment::compiler_has_fma;
	}

	//Performs a runtime CPU check to see if this type's microarchitecture level is supported.  (This will ensure that referernced integer types are also supported)
	static bool cpu_level_supported() {
		CpuInformation cpuid{};
		return cpu_level_supported(cpuid);
	}

	//Performs a runtime CPU check to see if this type's microarchitecture level is supported.  (This will ensure that referernced integer types are also supported)
	static bool cpu_level_supported(CpuInformation cpuid) {
		return cpuid.has_avx2() && cpuid.has_avx() && cpuid.has_fma();
	}

	//Performs a compile time support to see if the microarchitecture level is supported.  (This will ensure that referernced integer types are also supported)
	static constexpr bool compiler_level_supported() {
		return mt::environment::compiler_has_avx2 && mt::environment::compiler_has_avx && mt::environment::compiler_has_fma;
	}

	//*****Elements*****
	static constexpr int size_of_element() { return sizeof(int32_t); }
	static constexpr int number_of_elements() { return 8; }
	F element(int i) const { return mt::simd_detail_i32::lane_get(v, i); }
	void set_element(int i, F value) { v = mt::simd_detail_i32::lane_set(v, i, value); }

	//*****Addition Operators*****
	Simd256Int32& operator+=(const Simd256Int32& rhs) noexcept { v = _mm256_add_epi32(v, rhs.v); return *this; }
	Simd256Int32& operator+=(int32_t rhs) noexcept { v = _mm256_add_epi32(v, _mm256_set1_epi32(rhs));	return *this; }

	//*****Subtraction Operators*****
	Simd256Int32& operator-=(const Simd256Int32& rhs) noexcept { v = _mm256_sub_epi32(v, rhs.v); return *this; }
	Simd256Int32& operator-=(int32_t rhs) noexcept { v = _mm256_sub_epi32(v, _mm256_set1_epi32(rhs));	return *this; }

	//*****Multiplication Operators*****
	Simd256Int32& operator*=(const Simd256Int32& rhs) noexcept { v = _mm256_mullo_epi32(v, rhs.v);	return *this; }
	Simd256Int32& operator*=(int32_t rhs) noexcept { *this *= Simd256Int32(_mm256_set1_epi32(rhs)); return *this; }

	//*****Division Operators*****
	Simd256Int32& operator/=(const Simd256Int32& rhs) noexcept { 
		v = mt::simd_detail_i32::div_epi32(v, rhs.v);
		return *this;
	}
	Simd256Int32& operator/=(int32_t rhs) noexcept { 
		if constexpr (mt::environment::compiler_has_avx2) {
			v = mt::simd_detail_i32::div_epi32(v, _mm256_set1_epi32(rhs));
			return *this;
		}else {
			//I don't know why but visual studio was hanging when compiling this without AVX.
			//Since we wish to support runtime dispatch in visual studio, we fallback to scaler division in this case.
			v = _mm256_set_epi32(
				mt::simd_detail_i32::lane_get(v, 7) / rhs,
				mt::simd_detail_i32::lane_get(v, 6) / rhs,
				mt::simd_detail_i32::lane_get(v, 5) / rhs,
				mt::simd_detail_i32::lane_get(v, 4) / rhs,
				mt::simd_detail_i32::lane_get(v, 3) / rhs,
				mt::simd_detail_i32::lane_get(v, 2) / rhs,
				mt::simd_detail_i32::lane_get(v, 1) / rhs,
				mt::simd_detail_i32::lane_get(v, 0) / rhs
			);
			return *this;
		}
		
	}

	//*****Negate Operators*****
	Simd256Int32 operator-() const noexcept {
		return Simd256Int32(_mm256_sub_epi32(_mm256_setzero_si256(), v));
	}

	//*****Bitwise Logic Operators*****
	Simd256Int32& operator&=(const Simd256Int32& rhs) noexcept { v = _mm256_and_si256(v, rhs.v); return *this; }
	Simd256Int32& operator|=(const Simd256Int32& rhs) noexcept { v = _mm256_or_si256(v, rhs.v); return *this; }
	Simd256Int32& operator^=(const Simd256Int32& rhs) noexcept { v = _mm256_xor_si256(v, rhs.v); return *this; }

	//*****Make Functions****
	static Simd256Int32 make_sequential(int32_t first) { return Simd256Int32(_mm256_set_epi32(first + 7, first + 6, first + 5, first + 4, first + 3, first + 2, first + 1, first)); }


};

//*****Addition Operators*****
inline static Simd256Int32 operator+(Simd256Int32  lhs, const Simd256Int32& rhs) noexcept { lhs += rhs; return lhs; }
inline static Simd256Int32 operator+(Simd256Int32  lhs, int32_t rhs) noexcept { lhs += rhs; return lhs; }
inline static Simd256Int32 operator+(int32_t lhs, Simd256Int32 rhs) noexcept { rhs += lhs; return rhs; }

//*****Subtraction Operators*****
inline static Simd256Int32 operator-(Simd256Int32  lhs, const Simd256Int32& rhs) noexcept { lhs -= rhs; return lhs; }
inline static Simd256Int32 operator-(Simd256Int32  lhs, int32_t rhs) noexcept { lhs -= rhs; return lhs; }
inline static Simd256Int32 operator-(const int32_t lhs, const Simd256Int32& rhs) noexcept { return Simd256Int32(_mm256_sub_epi32(_mm256_set1_epi32(lhs), rhs.v)); }

//*****Multiplication Operators*****
inline static Simd256Int32 operator*(Simd256Int32  lhs, const Simd256Int32& rhs) noexcept { lhs *= rhs; return lhs; }
inline static Simd256Int32 operator*(Simd256Int32  lhs, int32_t rhs) noexcept { lhs *= rhs; return lhs; }
inline static Simd256Int32 operator*(int32_t lhs, Simd256Int32 rhs) noexcept { rhs *= lhs; return rhs; }

//*****Division Operators*****
inline Simd256Int32 operator/(Simd256Int32  lhs, const Simd256Int32& rhs) noexcept { lhs /= rhs;	return lhs; }
inline Simd256Int32 operator/(Simd256Int32  lhs, int32_t rhs) noexcept { lhs /= rhs; return lhs; }
inline Simd256Int32 operator/(const int32_t lhs, const Simd256Int32& rhs) noexcept { return Simd256Int32(mt::simd_detail_i32::div_epi32(_mm256_set1_epi32(lhs), rhs.v)); }


//*****Bitwise Logic Operators*****
inline static Simd256Int32 operator&(Simd256Int32  lhs, const Simd256Int32& rhs) noexcept { lhs &= rhs; return lhs; }
inline static Simd256Int32 operator|(Simd256Int32  lhs, const Simd256Int32& rhs) noexcept { lhs |= rhs; return lhs; }
inline static Simd256Int32 operator^(Simd256Int32  lhs, const Simd256Int32& rhs) noexcept { lhs ^= rhs; return lhs; }
inline static Simd256Int32 operator~(const Simd256Int32& lhs) noexcept { return Simd256Int32(_mm256_xor_si256(lhs.v, _mm256_cmpeq_epi32(lhs.v, lhs.v))); } //No bitwise not in AVX2 so we use xor with a constant..


//*****Shifting Operators*****
inline static Simd256Int32 operator<<(const Simd256Int32& lhs, int bits) noexcept { return Simd256Int32(_mm256_slli_epi32(lhs.v, bits)); }
//Arithmatic Shift is used for signed integers
inline static Simd256Int32 operator>>(const Simd256Int32& lhs, int bits) noexcept { return Simd256Int32(_mm256_srai_epi32(lhs.v, bits)); }

//*****Min/Max*****
inline static Simd256Int32 min(Simd256Int32 a, Simd256Int32 b) { return Simd256Int32(_mm256_min_epi32(a.v, b.v)); }
inline static Simd256Int32 max(Simd256Int32 a, Simd256Int32 b) { return Simd256Int32(_mm256_max_epi32(a.v, b.v)); }

//*****Mathematical*****
inline static Simd256Int32 abs(Simd256Int32 a) { return Simd256Int32(_mm256_abs_epi32(a.v)); }







#endif // MT_SIMD_ALLOW_LEVEL3_TYPES

/**************************************************************************************************
*SIMD 128 type.Contains 4 x 32bit Signed Integers
* Requires SSE2 support.
* (will be faster with SSE4.1 enabled at compile time)
* ************************************************************************************************/
struct Simd128Int32 {
	__m128i v;
	typedef int32_t F;

	Simd128Int32() = default;
	Simd128Int32(__m128i a) : v(a) {};
	Simd128Int32(F a) : v(_mm_set1_epi32(a)) {};

	//*****Support Informtion*****
	static bool cpu_supported() {
		CpuInformation cpuid{};
		return cpu_supported(cpuid);
	}
	static bool cpu_supported(CpuInformation cpuid) {
		return cpuid.has_sse2() && cpuid.has_sse();
	}

	//Performs a compile time support. Checks this type ONLY (integers in same class may not be supported) 
	static constexpr bool compiler_supported() {
		return mt::environment::compiler_has_sse && mt::environment::compiler_has_sse2;
	}

	//Performs a runtime CPU check to see if this type's microarchitecture level is supported.  (This will ensure that referernced integer types are also supported)
	static bool cpu_level_supported() {
		CpuInformation cpuid{};
		return cpu_level_supported(cpuid);
	}

	//Performs a runtime CPU check to see if this type's microarchitecture level is supported.  (This will ensure that referernced integer types are also supported)
	static bool cpu_level_supported(CpuInformation cpuid) {
		return cpuid.has_sse2() && cpuid.has_sse();
	}

	//Performs a compile time support to see if the microarchitecture level is supported.  (This will ensure that referernced integer types are also supported)
	static constexpr bool compiler_level_supported() {
		return mt::environment::compiler_has_sse && mt::environment::compiler_has_sse2;
	}

	//*****Elements*****
	static constexpr int size_of_element() { return sizeof(int32_t); }
	static constexpr int number_of_elements() { return 4; }
	F element(int i) { return mt::simd_detail_i32::lane_get(v, i); }
	void set_element(int i, F value) { v = mt::simd_detail_i32::lane_set(v, i, value); }

	//*****Addition Operators*****
	Simd128Int32& operator+=(const Simd128Int32& rhs) noexcept { v = _mm_add_epi32(v, rhs.v); return *this; }
	Simd128Int32& operator+=(int32_t rhs) noexcept { v = _mm_add_epi32(v, _mm_set1_epi32(rhs));	return *this; }

	//*****Subtraction Operators*****
	Simd128Int32& operator-=(const Simd128Int32& rhs) noexcept { v = _mm_sub_epi32(v, rhs.v); return *this; }
	Simd128Int32& operator-=(int32_t rhs) noexcept { v = _mm_sub_epi32(v, _mm_set1_epi32(rhs));	return *this; }

	//*****Multiplication Operators*****
	Simd128Int32& operator*=(const Simd128Int32& rhs) noexcept {
		if constexpr (mt::environment::compiler_has_sse4_1) {
			v = _mm_mullo_epi32(v, rhs.v); //SSE4.1
			return *this;
		}
		else {
			// SSE2 fallback: scalar lane multiply preserves signed semantics.
			const auto m3 = mt::simd_detail_i32::lane_get(v, 3) * mt::simd_detail_i32::lane_get(rhs.v, 3);
			const auto m2 = mt::simd_detail_i32::lane_get(v, 2) * mt::simd_detail_i32::lane_get(rhs.v, 2);
			const auto m1 = mt::simd_detail_i32::lane_get(v, 1) * mt::simd_detail_i32::lane_get(rhs.v, 1);
			const auto m0 = mt::simd_detail_i32::lane_get(v, 0) * mt::simd_detail_i32::lane_get(rhs.v, 0);
			v = _mm_set_epi32(m3, m2, m1, m0);
			return *this;
		}
	}

	Simd128Int32& operator*=(int32_t rhs) noexcept { *this *= Simd128Int32(_mm_set1_epi32(rhs)); return *this; }

	//*****Division Operators*****
	Simd128Int32& operator/=(const Simd128Int32& rhs) noexcept { v = mt::simd_detail_i32::div_epi32(v, rhs.v); return *this; }
	Simd128Int32& operator/=(int32_t rhs) noexcept { v = mt::simd_detail_i32::div_epi32(v, _mm_set1_epi32(rhs));	return *this; } //SSE

	//*****Negate Operators*****
	Simd128Int32 operator-() const noexcept {
		return Simd128Int32(_mm_sub_epi32(_mm_setzero_si128(), v));
	}

	//*****Bitwise Logic Operators*****
	Simd128Int32& operator&=(const Simd128Int32& rhs) noexcept { v = _mm_and_si128(v, rhs.v); return *this; } //SSE2
	Simd128Int32& operator|=(const Simd128Int32& rhs) noexcept { v = _mm_or_si128(v, rhs.v); return *this; }
	Simd128Int32& operator^=(const Simd128Int32& rhs) noexcept { v = _mm_xor_si128(v, rhs.v); return *this; }

	//*****Make Functions****
	static Simd128Int32 make_sequential(int32_t first) { return Simd128Int32(_mm_set_epi32(first + 3, first + 2, first + 1, first)); }






};

//*****Addition Operators*****
inline static Simd128Int32 operator+(Simd128Int32  lhs, const Simd128Int32& rhs) noexcept { lhs += rhs; return lhs; }
inline static Simd128Int32 operator+(Simd128Int32  lhs, int32_t rhs) noexcept { lhs += rhs; return lhs; }
inline static Simd128Int32 operator+(int32_t lhs, Simd128Int32 rhs) noexcept { rhs += lhs; return rhs; }

//*****Subtraction Operators*****
inline static Simd128Int32 operator-(Simd128Int32  lhs, const Simd128Int32& rhs) noexcept { lhs -= rhs; return lhs; }
inline static Simd128Int32 operator-(Simd128Int32  lhs, int32_t rhs) noexcept { lhs -= rhs; return lhs; }
inline static Simd128Int32 operator-(const int32_t lhs, const Simd128Int32& rhs) noexcept { return Simd128Int32(_mm_sub_epi32(_mm_set1_epi32(lhs), rhs.v)); }

//*****Multiplication Operators*****
inline static Simd128Int32 operator*(Simd128Int32  lhs, const Simd128Int32& rhs) noexcept { lhs *= rhs; return lhs; }
inline static Simd128Int32 operator*(Simd128Int32  lhs, int32_t rhs) noexcept { lhs *= rhs; return lhs; }
inline static Simd128Int32 operator*(int32_t lhs, Simd128Int32 rhs) noexcept { rhs *= lhs; return rhs; }

//*****Division Operators*****
inline static Simd128Int32 operator/(Simd128Int32  lhs, const Simd128Int32& rhs) noexcept { lhs /= rhs;	return lhs; }
inline static Simd128Int32 operator/(Simd128Int32  lhs, int32_t rhs) noexcept { lhs /= rhs; return lhs; }
inline static Simd128Int32 operator/(const int32_t lhs, const Simd128Int32& rhs) noexcept { return Simd128Int32(mt::simd_detail_i32::div_epi32(_mm_set1_epi32(lhs), rhs.v)); }


//*****Bitwise Logic Operators*****
inline static Simd128Int32 operator&(Simd128Int32  lhs, const Simd128Int32& rhs) noexcept { lhs &= rhs; return lhs; }
inline static Simd128Int32 operator|(Simd128Int32  lhs, const Simd128Int32& rhs) noexcept { lhs |= rhs; return lhs; }
inline static Simd128Int32 operator^(Simd128Int32  lhs, const Simd128Int32& rhs) noexcept { lhs ^= rhs; return lhs; }
inline static Simd128Int32 operator~(const Simd128Int32& lhs) noexcept { return Simd128Int32(_mm_xor_si128(lhs.v, _mm_cmpeq_epi32(lhs.v, lhs.v))); }


//*****Shifting Operators*****
inline static Simd128Int32 operator<<(const Simd128Int32& lhs, const int bits) noexcept { return Simd128Int32(_mm_slli_epi32(lhs.v, bits)); } //SSE2
inline static Simd128Int32 operator>>(const Simd128Int32& lhs, const int bits) noexcept { return Simd128Int32(_mm_srai_epi32(lhs.v, bits)); }


//*****Min/Max*****
inline static Simd128Int32 min(Simd128Int32 a, Simd128Int32 b) {
	if constexpr (mt::environment::compiler_has_sse4_1) {
		return Simd128Int32(_mm_min_epi32(a.v, b.v)); //SSE4.1
	}
	else {
		//No min/max or compare for Signed ints in SSE2 so we will just unroll.
		auto m3 = std::min(a.element(3), b.element(3));
		auto m2 = std::min(a.element(2), b.element(2));
		auto m1 = std::min(a.element(1), b.element(1));
		auto m0 = std::min(a.element(0), b.element(0));
		return Simd128Int32(_mm_set_epi32(m3, m2, m1, m0));
	}
}



inline static Simd128Int32 max(Simd128Int32 a, Simd128Int32 b) {
	if constexpr (mt::environment::compiler_has_sse4_1) {
		return Simd128Int32(_mm_max_epi32(a.v, b.v));  //SSE4.1
	}
	else {
		//No min/max or compare for Signed ints in SSE2 so we will just unroll.
		auto m3 = std::max(a.element(3), b.element(3));
		auto m2 = std::max(a.element(2), b.element(2));
		auto m1 = std::max(a.element(1), b.element(1));
		auto m0 = std::max(a.element(0), b.element(0));
		return Simd128Int32(_mm_set_epi32(m3, m2, m1, m0));
	}
}

//*****Mathematical*****
inline static Simd128Int32 abs(Simd128Int32 a) {
	if constexpr (mt::environment::compiler_has_ssse3) {
		return Simd128Int32(_mm_abs_epi32(a.v));
	}
	else {
		//Not supported by SSE2, so we need to emulate it.
		//This clever little code sequence is thanks to Agner Fog.
		const auto sign = _mm_srai_epi32(a.v, 31); //shift right moving in sign bits
		const auto inv = _mm_xor_si128(a.v, sign);   // invert bits if negative
		const auto result = _mm_sub_epi32(inv, sign); //add 1 if needed
		return Simd128Int32(result);
	}
}


#endif //x86_64





/**************************************************************************************************
 * Check that each type implements the desired types from simd-concepts.h
 * (Not sure why this fails on intelisense, but compliles ok.)
 * ************************************************************************************************/
static_assert(Simd<FallbackInt32>, "FallbackInt32 does not implement the concept Simd");
static_assert(SimdSigned<FallbackInt32>, "FallbackInt32 does not implement the concept SimdSigned");
static_assert(SimdInt<FallbackInt32>, "FallbackInt32 does not implement the concept SimdInt");
static_assert(SimdInt32<FallbackInt32>, "FallbackInt32 does not implement the concept SimdInt32");

#if MT_SIMD_ARCH_X64
static_assert(Simd<Simd128Int32>, "Simd128Int32 does not implement the concept Simd");
#if MT_SIMD_ALLOW_LEVEL3_TYPES
static_assert(Simd<Simd256Int32>, "Simd256Int32 does not implement the concept Simd");
#endif
#if MT_SIMD_ALLOW_LEVEL4_TYPES
static_assert(Simd<Simd512Int32>, "Simd512Int32 does not implement the concept Simd");
#endif

static_assert(SimdSigned<Simd128Int32>, "Simd128Int32 does not implement the concept SimdSigned");
#if MT_SIMD_ALLOW_LEVEL3_TYPES
static_assert(SimdSigned<Simd256Int32>, "Simd256Int32 does not implement the concept SimdSigned");
#endif
#if MT_SIMD_ALLOW_LEVEL4_TYPES
static_assert(SimdSigned<Simd512Int32>, "Simd512Int32 does not implement the concept SimdSigned");
#endif

static_assert(SimdInt<Simd128Int32>, "Simd256Int32 does not implement the concept SimdInt");
#if MT_SIMD_ALLOW_LEVEL3_TYPES
static_assert(SimdInt<Simd256Int32>, "Simd256Int32 does not implement the concept SimdInt");
#endif
#if MT_SIMD_ALLOW_LEVEL4_TYPES
static_assert(SimdInt<Simd512Int32>, "Simd512Int32 does not implement the concept SimdInt");
#endif



static_assert(SimdInt32<Simd128Int32>, "Simd128Int32 does not implement the concept SimdInt32");
#if MT_SIMD_ALLOW_LEVEL3_TYPES
static_assert(SimdInt32<Simd256Int32>, "Simd256Int32 does not implement the concept SimdInt32");
#endif
#if MT_SIMD_ALLOW_LEVEL4_TYPES
static_assert(SimdInt32<Simd512Int32>, "Simd512Int32 does not implement the concept SimdInt32");
#endif
#endif



/**************************************************************************************************
 Define SimdNativeInt32 as the best supported type at compile time.
*************************************************************************************************/
#if MT_SIMD_ARCH_X64
#if MT_SIMD_ALLOW_LEVEL4_TYPES
typedef Simd512Int32 SimdNativeInt32;
#else
#if MT_SIMD_ALLOW_LEVEL3_TYPES
typedef Simd256Int32 SimdNativeInt32;
#else
#if defined(__SSE4_1__) && defined(__SSE4_1__) && defined(__SSE3__) && defined(__SSSE3__) 
typedef Simd128Int32 SimdNativeInt32;
#else
typedef Simd128Int32 SimdNativeInt32;
#endif	
#endif	
#endif
#else
typedef FallbackInt32 SimdNativeInt32;
#endif


