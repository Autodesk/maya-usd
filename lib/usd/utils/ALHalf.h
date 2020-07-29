//
// Copyright 2017 Animal Logic
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
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
//----------------------------------------------------------------------------------------------------------------------
/// \file   ALHalf.h
/// \brief  The half <-> float conversions included within USD make use of a somewhat inefficient
/// LUT to perform
///         conversion from half to float, and the float to half conversion requires quite a bit
///         shifting and masking. The Intel Ivy Bridge CPUs actually implemented float <->
///         conversions in hardware via the vcvtps2ph and vcvtph2ps instructions (which convert 8
///         floats at a time with a latency of 4 or 5 cycles). This header file provides some
///         methods to convert between half/float and half/double using the F16C conversion
///         intrinsics. To enable HW conversions, pass the compiler flag -mf16c to clang or gcc.
//----------------------------------------------------------------------------------------------------------------------
#pragma once

#if __F16C__
#include <immintrin.h>
#endif

#include <pxr/base/gf/half.h>
#include <pxr/base/gf/ilmbase_half.h>

PXR_NAMESPACE_USING_DIRECTIVE

namespace MayaUsdUtils {

#ifdef __F16C__

/// converts 8xhalf to 8xfloat
inline void half2float_8f(const GfHalf input[8], float out[8])
{
    const __m128i a = _mm_loadu_si128((const __m128i*)input);
    _mm256_storeu_ps(out, _mm256_cvtph_ps(a));
}

/// converts 4xhalf to 4xfloat
inline void half2float_4f(const GfHalf input[4], float out[4])
{
    const __m128i a = _mm_castpd_si128(_mm_load_sd((const double*)input));
    _mm_storeu_ps(out, _mm_cvtph_ps(a));
}

/// converts a half to a float
inline float half2float_1f(const GfHalf h)
{
    const __m128i a = _mm_set1_epi32(uint32_t(h.bits()) << 16 | h.bits());
    const __m128  f = _mm_cvtph_ps(a);
    float         r;
    _mm_store_ss(&r, f);
    return r;
}

/// converts 8xhalf to 8xfloat
inline void half2double_8f(const GfHalf input[8], double out[8])
{
    const __m128i a = _mm_loadu_si128((const __m128i*)input);
    const __m256  f = _mm256_cvtph_ps(a);
    const __m256d flo = _mm256_cvtps_pd(_mm256_extractf128_ps(f, 0));
    const __m256d fhi = _mm256_cvtps_pd(_mm256_extractf128_ps(f, 1));
    _mm256_storeu_pd(out, flo);
    _mm256_storeu_pd(out + 4, fhi);
}

/// converts 4xhalf to 4xfloat
inline void half2double_4f(const GfHalf input[4], double out[4])
{
    const __m128i a = _mm_castpd_si128(_mm_load_sd((const double*)input));
    _mm256_storeu_pd(out, _mm256_cvtps_pd(_mm_cvtph_ps(a)));
}

/// converts a half to a float
inline double half2double_1f(const GfHalf h)
{
    const __m128i a = _mm_set1_epi32(uint32_t(h.bits()) << 16 | h.bits());
    const __m128  f = _mm_cvtph_ps(a);
    float         r;
    _mm_store_ss(&r, f);
    return double(r);
}

/// converts 8xfloat to 8xhalf
inline void float2half_8f(const float input[8], GfHalf out[8])
{
    const __m256 a = _mm256_loadu_ps(input);
    _mm_storeu_si128((__m128i*)out, _mm256_cvtps_ph(a, _MM_FROUND_CUR_DIRECTION));
}

/// converts 4xfloat to 4xhalf
inline void float2half_4f(const float input[4], GfHalf out[4])
{
    const __m128 a = _mm_loadu_ps(input);
    _mm_store_sd((double*)out, _mm_castsi128_pd(_mm_cvtps_ph(a, _MM_FROUND_CUR_DIRECTION)));
}

/// converts a float to a half
inline GfHalf float2half_1f(const float f)
{
    const __m128   a = _mm_load_ss(&f);
    const __m128i  b = _mm_cvtps_ph(a, _MM_FROUND_CUR_DIRECTION);
    const uint16_t i = _mm_extract_epi16(b, 0);
    return *(const GfHalf*)(&i);
}

/// converts 8xdouble to 8xhalf
inline void double2half_8f(const double input[8], GfHalf out[8])
{
    const __m256d alo = _mm256_loadu_pd(input);
    const __m256d ahi = _mm256_loadu_pd(input + 4);
    const __m256  a = _mm256_insertf128_ps(
        _mm256_castps128_ps256(_mm256_cvtpd_ps(alo)), _mm256_cvtpd_ps(ahi), 1);
    _mm_storeu_si128((__m128i*)out, _mm256_cvtps_ph(a, _MM_FROUND_CUR_DIRECTION));
}

/// converts 4xdouble to 4xhalf
inline void double2half_4f(const double input[4], GfHalf out[4])
{
    const __m128 a = _mm256_cvtpd_ps(_mm256_loadu_pd(input));
    _mm_store_sd((double*)out, _mm_castsi128_pd(_mm_cvtps_ph(a, _MM_FROUND_CUR_DIRECTION)));
}

/// converts a double to a half
inline GfHalf double2half_1f(const double f)
{
    const __m128d  d = _mm_load_sd(&f);
    const __m128   a = _mm_cvtpd_ps(d);
    const __m128i  b = _mm_cvtps_ph(a, _MM_FROUND_CUR_DIRECTION);
    const uint16_t i = _mm_extract_epi16(b, 0);
    return *(const GfHalf*)(&i);
}

#else
/// converts 8xhalf to 8xfloat
inline void half2float_8f(const GfHalf input[8], float out[8])
{
    out[0] = float(input[0]);
    out[1] = float(input[1]);
    out[2] = float(input[2]);
    out[3] = float(input[3]);
    out[4] = float(input[4]);
    out[5] = float(input[5]);
    out[6] = float(input[6]);
    out[7] = float(input[7]);
}

/// converts 4xhalf to 4xfloat
inline void half2float_4f(const GfHalf input[4], float out[4])
{
    out[0] = float(input[0]);
    out[1] = float(input[1]);
    out[2] = float(input[2]);
    out[3] = float(input[3]);
}

/// convert half to float
inline float half2float_1f(const GfHalf h) { return float(h); }

/// converts 8xhalf to 8xfloat
inline void half2double_8f(const GfHalf input[8], double out[8])
{
    out[0] = double(float(input[0]));
    out[1] = double(float(input[1]));
    out[2] = double(float(input[2]));
    out[3] = double(float(input[3]));
    out[4] = double(float(input[4]));
    out[5] = double(float(input[5]));
    out[6] = double(float(input[6]));
    out[7] = double(float(input[7]));
}

/// converts 4xhalf to 4xfloat
inline void half2double_4f(const GfHalf input[4], double out[4])
{
    out[0] = double(float(input[0]));
    out[1] = double(float(input[1]));
    out[2] = double(float(input[2]));
    out[3] = double(float(input[3]));
}

/// convert half to float
inline double half2fdouble_1f(const GfHalf h) { return double(float(h)); }

/// converts 8xfloat to 8xhalf
inline void float2half_8f(const float input[8], GfHalf out[8])
{
    out[0] = GfHalf(input[0]);
    out[1] = GfHalf(input[1]);
    out[2] = GfHalf(input[2]);
    out[3] = GfHalf(input[3]);
    out[4] = GfHalf(input[4]);
    out[5] = GfHalf(input[5]);
    out[6] = GfHalf(input[6]);
    out[7] = GfHalf(input[7]);
}

/// converts 4xfloat to 4xhalf
inline void float2half_4f(const float input[4], GfHalf out[4])
{
    out[0] = GfHalf(input[0]);
    out[1] = GfHalf(input[1]);
    out[2] = GfHalf(input[2]);
    out[3] = GfHalf(input[3]);
}

/// converts a float to a half
inline GfHalf float2half_1f(const float f) { return GfHalf(f); }

/// converts 8xdouble to 8xhalf
inline void double2half_8f(const double input[8], GfHalf out[8])
{
    out[0] = GfHalf(float(input[0]));
    out[1] = GfHalf(float(input[1]));
    out[2] = GfHalf(float(input[2]));
    out[3] = GfHalf(float(input[3]));
    out[4] = GfHalf(float(input[4]));
    out[5] = GfHalf(float(input[5]));
    out[6] = GfHalf(float(input[6]));
    out[7] = GfHalf(float(input[7]));
}

/// converts 4xdouble to 4xhalf
inline void double2half_4f(const double input[4], GfHalf out[4])
{
    out[0] = GfHalf(float(input[0]));
    out[1] = GfHalf(float(input[1]));
    out[2] = GfHalf(float(input[2]));
    out[3] = GfHalf(float(input[3]));
}

/// converts a double to a half
inline GfHalf double2half_1f(const double f) { return GfHalf(float(f)); }
#endif

} // namespace MayaUsdUtils
