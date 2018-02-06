//
// Copyright 2017 Animal Logic
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.//
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
#pragma once

// If you need to modify this file for some reason, you'll notice that I'm using some SSE and AVX2 intrinsics in this file.
// If you're not comfortable with SIMD intrinsics, I apologize. The following macro will disable the intrinsics and fallback
// to bog standard C++.
#if !defined(AL_UTILS_ENABLE_SIMD)
# define AL_UTILS_ENABLE_SIMD 1
#endif

// If neither of these are defined, don't use SIMD at all.
#if !defined(__SSE3__) && !defined(__AVX2__)
# undef AL_UTILS_ENABLE_SIMD
# define AL_UTILS_ENABLE_SIMD 0
#endif

#ifdef _WIN32
# define ALIGN16(X) __declspec(align(16)) X
# define ALIGN32(X) __declspec(align(32)) X
#else
# define ALIGN16(X) X __attribute__((aligned(16)))
# define ALIGN32(X) X __attribute__((aligned(32)))
#endif


#if AL_UTILS_ENABLE_SIMD

# ifdef __AVX2__
#  include <immintrin.h>
# endif

# ifdef __SSE3__
#  include <pmmintrin.h>
# endif

# ifdef __SSE4_1__
#  include <smmintrin.h>
# endif

# if __GNUC__ >= 4
#  define AL_DLL_HIDDEN  __attribute__ ((visibility ("hidden")))
# else
#  define AL_DLL_HIDDEN
# endif

// For reasons unknown, GCC 4.8 failsusdmaya to correctly assemble certain AVX2 instructions.
// This is a known issue that was fixed in gcc 4.9.
#if (__GNUC__ <= 4) && (__GNUC_MINOR__ <= 8)
# define ENABLE_SOME_AVX_ROUTINES 0
#else
# define ENABLE_SOME_AVX_ROUTINES 1
#endif

namespace AL {

# if __SSE3__
typedef __m128 f128;
typedef __m128i i128;
typedef __m128d d128;

#define shiftBytesLeft(reg, count) _mm_slli_si128(reg, count)
#define shiftBytesRight(reg, count) _mm_srli_si128(reg, count)
#define shuffle4f(a, b, W, Z, Y, X) _mm_shuffle_ps(a, b, _MM_SHUFFLE(W, Z, Y, X))

#define lshift64(X, N) _mm_slli_epi64(X, N)

AL_DLL_HIDDEN inline f128 zero4f() { return _mm_setzero_ps(); }
AL_DLL_HIDDEN inline i128 zero4i() { return _mm_setzero_si128(); }
AL_DLL_HIDDEN inline d128 zero2d() { return _mm_setzero_pd(); }

AL_DLL_HIDDEN inline f128 cast4f(const d128 reg) { return _mm_castpd_ps(reg); }
AL_DLL_HIDDEN inline f128 cast4f(const i128 reg) { return _mm_castsi128_ps(reg); }
AL_DLL_HIDDEN inline i128 cast4i(const d128 reg) { return _mm_castpd_si128(reg); }
AL_DLL_HIDDEN inline i128 cast4i(const f128 reg) { return _mm_castps_si128(reg); }
AL_DLL_HIDDEN inline d128 cast2d(const f128 reg) { return _mm_castps_pd(reg); }
AL_DLL_HIDDEN inline d128 cast2d(const i128 reg) { return _mm_castsi128_pd(reg); }

AL_DLL_HIDDEN inline f128 load1f(const float* const ptr) { return _mm_load_ss(ptr); }
AL_DLL_HIDDEN inline f128 load2f(const float* const ptr) { return cast4f(_mm_load_sd((const double*)ptr)); }

AL_DLL_HIDDEN inline int32_t movemask16i8(const i128 reg) { return _mm_movemask_epi8(reg); }
AL_DLL_HIDDEN inline int32_t movemask4i(const i128 reg) { return _mm_movemask_ps(cast4f(reg)); }
AL_DLL_HIDDEN inline int32_t movemask4f(const f128 reg) { return _mm_movemask_ps(reg); }
AL_DLL_HIDDEN inline int32_t movemask2d(const d128 reg) { return _mm_movemask_pd(reg); }

AL_DLL_HIDDEN inline i128 cmpeq4i(const i128 a, const i128 b) { return _mm_cmpeq_epi32(a, b); }
AL_DLL_HIDDEN inline i128 cmpeq16i8(const i128 a, const i128 b) { return _mm_cmpeq_epi8(a, b); }
AL_DLL_HIDDEN inline i128 cmplt16i8(const i128 a, const i128 b) { return _mm_cmplt_epi8(a, b); }
AL_DLL_HIDDEN inline i128 cmpgt16i8(const i128 a, const i128 b) { return _mm_cmpgt_epi8(a, b); }

AL_DLL_HIDDEN inline f128 set4f(const float a, const float b, const float c, const float d) {return _mm_setr_ps(a, b, c, d); }
AL_DLL_HIDDEN inline i128 set4i(const int32_t a, const int32_t b, const int32_t c, const int32_t d) {return _mm_setr_epi32(a, b, c, d); }
AL_DLL_HIDDEN inline d128 set2d(const double a, const double b) {return _mm_setr_pd(a, b); }

AL_DLL_HIDDEN inline i128 set16i8(
    const int8_t a0, const int8_t b0, const int8_t c0, const int8_t d0,
    const int8_t a1, const int8_t b1, const int8_t c1, const int8_t d1,
    const int8_t a2, const int8_t b2, const int8_t c2, const int8_t d2,
    const int8_t a3, const int8_t b3, const int8_t c3, const int8_t d3)
{return _mm_setr_epi8(a0, b0, c0, d0, a1, b1, c1, d1, a2, b2, c2, d2, a3, b3, c3, d3); }

AL_DLL_HIDDEN inline f128 loadu4f(const void* const ptr) { return _mm_loadu_ps((const float*)ptr); }
AL_DLL_HIDDEN inline i128 loadu4i(const void* const ptr) { return _mm_loadu_si128((const i128*)ptr); }
AL_DLL_HIDDEN inline d128 loadu2d(const void* const ptr) { return _mm_loadu_pd((const double*)ptr); }

AL_DLL_HIDDEN inline f128 load4f(const void* const ptr) { return _mm_load_ps((const float*)ptr); }
AL_DLL_HIDDEN inline i128 load4i(const void* const ptr) { return _mm_load_si128((const i128*)ptr); }
AL_DLL_HIDDEN inline d128 load2d(const void* const ptr) { return _mm_load_pd((const double*)ptr); }

AL_DLL_HIDDEN inline void storeu4f(void* const ptr, const f128 reg) { _mm_storeu_ps((float*)ptr, reg); }
AL_DLL_HIDDEN inline void storeu4i(void* const ptr, const i128 reg) { _mm_storeu_si128((i128*)ptr, reg); }
AL_DLL_HIDDEN inline void storeu2d(void* const ptr, const d128 reg) { _mm_storeu_pd((double*)ptr, reg); }

AL_DLL_HIDDEN inline void store4f(void* const ptr, const f128 reg) { _mm_store_ps((float*)ptr, reg); }
AL_DLL_HIDDEN inline void store4i(void* const ptr, const i128 reg) { _mm_store_si128((i128*)ptr, reg); }
AL_DLL_HIDDEN inline void store2d(void* const ptr, const d128 reg) { _mm_store_pd((double*)ptr, reg); }

AL_DLL_HIDDEN inline d128 cvt2f_to_2d(const f128 reg) { return _mm_cvtps_pd(reg); }
AL_DLL_HIDDEN inline f128 cvt2d_to_2f(const d128 reg) { return _mm_cvtpd_ps(reg); }

AL_DLL_HIDDEN inline f128 movehl4f(const f128 a, const f128 b) { return _mm_movehl_ps(a, b); }
AL_DLL_HIDDEN inline f128 movelh4f(const f128 a, const f128 b) { return _mm_movelh_ps(a, b); }
AL_DLL_HIDDEN inline i128 movehl4i(const i128 a, const i128 b) { return cast4i(_mm_movehl_ps(cast4f(a), cast4f(b))); }
AL_DLL_HIDDEN inline i128 movelh4i(const i128 a, const i128 b) { return cast4i(_mm_movelh_ps(cast4f(a), cast4f(b))); }

AL_DLL_HIDDEN inline f128 or4f(const f128 a, const f128 b) { return _mm_or_ps(a, b); }
AL_DLL_HIDDEN inline f128 and4f(const f128 a, const f128 b) { return _mm_and_ps(a, b); }
AL_DLL_HIDDEN inline f128 andnot4f(const f128 a, const f128 b) { return _mm_andnot_ps(a, b); }

AL_DLL_HIDDEN inline i128 or4i(const i128 a, const i128 b) { return _mm_or_si128(a, b); }
AL_DLL_HIDDEN inline i128 and4i(const i128 a, const i128 b) { return _mm_and_si128(a, b); }
AL_DLL_HIDDEN inline i128 andnot4i(const i128 a, const i128 b) { return _mm_andnot_si128(a, b); }

AL_DLL_HIDDEN inline f128 mul4f(const f128 a, const f128 b) { return _mm_mul_ps(a, b); }
AL_DLL_HIDDEN inline d128 mul2d(const d128 a, const d128 b) { return _mm_mul_pd(a, b); }

AL_DLL_HIDDEN inline f128 add4f(const f128 a, const f128 b) { return _mm_add_ps(a, b); }
AL_DLL_HIDDEN inline i128 add4i(const i128 a, const i128 b) { return _mm_add_epi32(a, b); }
AL_DLL_HIDDEN inline d128 add2d(const d128 a, const d128 b) { return _mm_add_pd(a, b); }
AL_DLL_HIDDEN inline i128 add2i64(const i128 a, const i128 b) { return _mm_add_epi64(a, b); }

AL_DLL_HIDDEN inline f128 sub4f(const f128 a, const f128 b) { return _mm_sub_ps(a, b); }
AL_DLL_HIDDEN inline i128 sub4i(const i128 a, const i128 b) { return _mm_sub_epi32(a, b); }
AL_DLL_HIDDEN inline d128 sub2d(const d128 a, const d128 b) { return _mm_sub_pd(a, b); }
AL_DLL_HIDDEN inline i128 sub2i64(const i128 a, const i128 b) { return _mm_sub_epi64(a, b); }

AL_DLL_HIDDEN inline f128 splat4f(float f) { return _mm_set1_ps(f); }
AL_DLL_HIDDEN inline d128 splat2d(double f) { return _mm_set1_pd(f); }
AL_DLL_HIDDEN inline i128 splat4i(int32_t f) { return _mm_set1_epi32(f); }
AL_DLL_HIDDEN inline i128 splat2i64(const int64_t f) { return _mm_set1_epi64x(f); }

AL_DLL_HIDDEN inline f128 unpacklo4f(const f128 a, const f128 b) { return _mm_unpacklo_ps(a, b); }
AL_DLL_HIDDEN inline f128 unpackhi4f(const f128 a, const f128 b) { return _mm_unpackhi_ps(a, b); }

#if !defined(__SSE4__) && !defined(__SSE4_1__) && !defined(__SSE4_2__) && !defined(__AVX__) && !defined(__AVX2__)
AL_DLL_HIDDEN inline __m128 _mm_blendv_ps(__m128 a, __m128 b, __m128 c)
{
  return _mm_or_ps(_mm_and_ps(c, b), _mm_andnot_ps(c, a));
}
#else
AL_DLL_HIDDEN inline i128 cvt2i32_to_2i64(const i128 reg) { return _mm_cvtepi32_epi64(reg); }
#endif

AL_DLL_HIDDEN inline f128 select4f(const f128 falseResult, const f128 trueResult, const f128 cmp) { return _mm_blendv_ps(falseResult, trueResult, cmp); }

#define shiftBytesLeft128(reg, count) _mm_slli_si128(reg, count)
#define shiftBytesRight128(reg, count) _mm_srli_si128(reg, count)
#define shiftBitsLeft4i32(reg, count) _mm_slli_epi32(reg, count)
#define shiftBitsRight4i32(reg, count) _mm_srli_epi32(reg, count)
#define shiftBitsLeft2i64(reg, count) _mm_slli_epi64(reg, count)
#define shiftBitsRight2i64(reg, count) _mm_srli_epi64(reg, count)

#define extract128i64(reg, index) _mm_extract_epi64(reg, index)

#endif

# if __AVX2__
typedef __m256 f256;
typedef __m256i i256;
typedef __m256d d256;

#define shuffle8f(a, b, W, Z, Y, X) _mm256_shuffle_ps(a, b, _MM_SHUFFLE(W, Z, Y, X))

AL_DLL_HIDDEN inline f256 zero8f() { return _mm256_setzero_ps(); }
AL_DLL_HIDDEN inline i256 zero8i() { return _mm256_setzero_si256(); }
AL_DLL_HIDDEN inline d256 zero4d() { return _mm256_setzero_pd(); }

AL_DLL_HIDDEN inline f256 cast8f(const d256 reg) { return _mm256_castpd_ps(reg); }
AL_DLL_HIDDEN inline f256 cast8f(const i256 reg) { return _mm256_castsi256_ps(reg); }
AL_DLL_HIDDEN inline i256 cast8i(const d256 reg) { return _mm256_castpd_si256(reg); }
AL_DLL_HIDDEN inline i256 cast8i(const f256 reg) { return _mm256_castps_si256(reg); }
AL_DLL_HIDDEN inline d256 cast4d(const f256 reg) { return _mm256_castps_pd(reg); }
AL_DLL_HIDDEN inline d256 cast4d(const i256 reg) { return _mm256_castsi256_pd(reg); }

AL_DLL_HIDDEN inline int32_t movemask8i(const i256 reg) { return _mm256_movemask_ps(cast8f(reg)); }
AL_DLL_HIDDEN inline int32_t movemask8f(const f256 reg) { return _mm256_movemask_ps(reg); }
AL_DLL_HIDDEN inline int32_t movemask4d(const d256 reg) { return _mm256_movemask_pd(reg); }

AL_DLL_HIDDEN inline i256 cmpeq8i(const i256 a, const i256 b) { return _mm256_cmpeq_epi32(a, b); }

#define permute2f128(a, b, mask) _mm256_permute2f128_ps(a, b, mask)

AL_DLL_HIDDEN inline f256 set8f(const float a, const float b, const float c, const float d,
                                  const float e, const float f, const float g, const float h)
  {return _mm256_setr_ps(a,b,c,d,e,f,g,h); }
AL_DLL_HIDDEN inline i256 set8i(const int32_t a, const int32_t b, const int32_t c, const int32_t d,
                                const int32_t e, const int32_t f, const int32_t g, const int32_t h)
  {return _mm256_setr_epi32(a,b,c,d,e,f,g,h); }
AL_DLL_HIDDEN inline d256 set4f(const double a, const double b, const double c, const double d)
  {return _mm256_setr_pd(a, b, c, d); }

AL_DLL_HIDDEN inline f256 loadu8f(const void* const ptr) { return _mm256_loadu_ps((const float*)ptr); }
AL_DLL_HIDDEN inline i256 loadu8i(const void* const ptr) { return _mm256_loadu_si256((const i256*)ptr); }
AL_DLL_HIDDEN inline d256 loadu4d(const void* const ptr) { return _mm256_loadu_pd((const double*)ptr); }

AL_DLL_HIDDEN inline f256 load8f(const void* const ptr) { return _mm256_load_ps((const float*)ptr); }
AL_DLL_HIDDEN inline i256 load8i(const void* const ptr) { return _mm256_load_si256((const i256*)ptr); }
AL_DLL_HIDDEN inline d256 load4d(const void* const ptr) { return _mm256_load_pd((const double*)ptr); }

AL_DLL_HIDDEN inline void storeu8f(void* const ptr, const f256 reg) { _mm256_storeu_ps((float*)ptr, reg); }
AL_DLL_HIDDEN inline void storeu8i(void* const ptr, const i256 reg) { _mm256_storeu_si256((i256*)ptr, reg); }
AL_DLL_HIDDEN inline void storeu4d(void* const ptr, const d256 reg) { _mm256_storeu_pd((double*)ptr, reg); }

AL_DLL_HIDDEN inline void store8f(void* const ptr, const f256 reg) { _mm256_store_ps((float*)ptr, reg); }
AL_DLL_HIDDEN inline void store8i(void* const ptr, const i256 reg) { _mm256_store_si256((i256*)ptr, reg); }
AL_DLL_HIDDEN inline void store4d(void* const ptr, const d256 reg) { _mm256_store_pd((double*)ptr, reg); }

AL_DLL_HIDDEN inline d256 cvt4f_to_4d(const f128 reg) { return _mm256_cvtps_pd(reg); }
AL_DLL_HIDDEN inline f128 cvt4d_to_4f(const d256 reg) { return _mm256_cvtpd_ps(reg); }
AL_DLL_HIDDEN inline i256 cvt4i32_to_4i64(const i128 reg) { return _mm256_cvtepi32_epi64(reg); }

AL_DLL_HIDDEN inline f256 or8f(const f256 a, const f256 b) { return _mm256_or_ps(a, b); }
AL_DLL_HIDDEN inline f256 and8f(const f256 a, const f256 b) { return _mm256_and_ps(a, b); }
AL_DLL_HIDDEN inline f256 andnot8f(const f256 a, const f256 b) { return _mm256_andnot_ps(a, b); }

AL_DLL_HIDDEN inline i256 or8i(const i256 a, const i256 b) { return _mm256_or_si256(a, b); }
AL_DLL_HIDDEN inline i256 and8i(const i256 a, const i256 b) { return _mm256_and_si256(a, b); }
AL_DLL_HIDDEN inline i256 andnot8i(const i256 a, const i256 b) { return _mm256_andnot_si256(a, b); }

AL_DLL_HIDDEN inline f256 mul8f(const f256 a, const f256 b) { return _mm256_mul_ps(a, b); }
AL_DLL_HIDDEN inline d256 mul4d(const d256 a, const d256 b) { return _mm256_mul_pd(a, b); }

AL_DLL_HIDDEN inline f256 add8f(const f256 a, const f256 b) { return _mm256_add_ps(a, b); }
AL_DLL_HIDDEN inline i256 add8i(const i256 a, const i256 b) { return _mm256_add_epi32(a, b); }
AL_DLL_HIDDEN inline d256 add4d(const d256 a, const d256 b) { return _mm256_add_pd(a, b); }
AL_DLL_HIDDEN inline i256 add4i64(const i256 a, const i256 b) { return _mm256_add_epi64(a, b); }

AL_DLL_HIDDEN inline f256 select8f(const f256 falseResult, const f256 trueResult, const f256 cmp) { return _mm256_blendv_ps(falseResult, trueResult, cmp); }

AL_DLL_HIDDEN inline f256 permutevar8x32f(const f256 a, const i256 b) { return _mm256_permutevar8x32_ps(a, b); }

AL_DLL_HIDDEN inline f256 unpacklo8f(const f256 a, const f256 b) { return _mm256_unpacklo_ps(a, b); }
AL_DLL_HIDDEN inline f256 unpackhi8f(const f256 a, const f256 b) { return _mm256_unpackhi_ps(a, b); }

#define extract4f(reg, index) _mm256_extractf128_ps(reg, index)
#define extract256i64(reg, index) _mm256_extract_epi64(reg, index)

AL_DLL_HIDDEN inline f256 splat8f(const float f) { return _mm256_set1_ps(f); }
AL_DLL_HIDDEN inline d256 splat4d(const double f) { return _mm256_set1_pd(f); }
AL_DLL_HIDDEN inline i256 splat8i(const int32_t f) { return _mm256_set1_epi32(f); }
AL_DLL_HIDDEN inline i256 splat4i64(const int64_t f) { return _mm256_set1_epi64x(f); }

AL_DLL_HIDDEN inline f128 i32gather4f(const float* const ptr, const i128 indices) { return _mm_i32gather_ps(ptr, indices, 4); }
AL_DLL_HIDDEN inline f256 i32gather8f(const float* const ptr, const i256 indices) { return _mm256_i32gather_ps(ptr, indices, 4); }
AL_DLL_HIDDEN inline i128 i32gather4i(const int32_t* const ptr, const i128 indices) { return _mm_i32gather_epi32(ptr, indices, 4); }
AL_DLL_HIDDEN inline i256 i32gather8i(const int32_t* const ptr, const i256 indices) { return _mm256_i32gather_epi32(ptr, indices, 4); }

AL_DLL_HIDDEN inline f256 set2f128(const f128 lo, const f128 hi) { return _mm256_insertf128_ps(_mm256_castps128_ps256(lo), hi, 1); }

#define shiftBytesLeft256(reg, count) _mm256_slli_si256(reg, count)
#define shiftBytesRight256(reg, count) _mm256_srli_si256(reg, count)
#define shiftBitsLeft8i32(reg, count) _mm256_slli_epi32(reg, count)
#define shiftBitsRight8i32(reg, count) _mm256_srli_epi32(reg, count)
#define shiftBitsLeft4i64(reg, count) _mm256_slli_epi64(reg, count)
#define shiftBitsRight4i64(reg, count) _mm256_srli_epi64(reg, count)

# endif
}// AL
#endif

