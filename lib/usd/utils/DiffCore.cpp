//
// Copyright 2018 Animal Logic
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
#include "DiffCore.h"

#include <mayaUsdUtils/SIMD.h>

#include <algorithm>
#include <cmath>

namespace MayaUsdUtils {

//----------------------------------------------------------------------------------------------------------------------
bool vec2AreAllTheSame(const float* u, const float* v, size_t count)
{
    // if already at the end of the array, we're done
    if (count <= 1) {
        return true;
    }

#ifdef __AVX2__

    const f256 u8 = splat8f(u[0]);
    const f256 v8 = splat8f(v[0]);

    const size_t count8 = count & ~7ULL;
    for (size_t i = 0; i < count8; i += 8) {
        const f256 uu = loadu8f(u + i);
        const f256 vv = loadu8f(v + i);
        const f256 cmpu = cmpne8f(uu, u8);
        const f256 cmpv = cmpne8f(vv, v8);
        if (movemask8f(or8f(cmpu, cmpv)))
            return false;
    }

    for (size_t i = count8; i < count; ++i) {
        if (u[i] != u[0] || v[i] != v[0])
            return false;
    }
    return true;

#elif defined(__SSE__)

    const f128 u4 = splat4f(u[0]);
    const f128 v4 = splat4f(v[0]);

    const size_t count4 = count & ~3ULL;
    for (size_t i = 0; i < count4; i += 4) {
        const f128 uu = loadu4f(u + i);
        const f128 vv = loadu4f(v + i);
        const f128 cmpu = cmpne4f(uu, u4);
        const f128 cmpv = cmpne4f(vv, v4);
        if (movemask4f(or4f(cmpu, cmpv)))
            return false;
    }

    for (size_t i = count4; i < count; ++i) {
        if (u[i] != u[0] || v[i] != v[0])
            return false;
    }
    return true;
#else
    for (size_t i = 1; i < count; ++i) {
        if (u[0] != u[i] || v[0] != v[i])
            return false;
    }
    return true;
#endif
}

//----------------------------------------------------------------------------------------------------------------------
bool vec2AreAllTheSame(const float* array, size_t count)
{
    // if already at the end of the array, we're done
    if (count <= 1) {
        return true;
    }
#ifdef __AVX2__

    const float x = array[0];
    const float y = array[1];
    const f256  xy = set8f(x, y, x, y, x, y, x, y);
    size_t      count4 = count & ~3ULL;
    for (size_t i = 0, n = count4 * 2; i < n; i += 8) {
        const f256 temp = loadu8f(array + i);
        const f256 cmp = cmpne8f(temp, xy);
        if (movemask8f(cmp))
            return false;
    }
    if (count & 2) {
        const f128 temp = loadu4f(array + count4 * 2);
        const f128 cmp = cmpne4f(temp, cast4f(xy));
        if (movemask4f(cmp))
            return false;
        count4 += 2;
    }
    if (count & 1) {
        const float nx = array[count4 * 2];
        const float ny = array[count4 * 2 + 1];
        if (nx != x || ny != y)
            return false;
    }
    return true;

#elif defined(__SSE__)

    const float  x = array[0];
    const float  y = array[1];
    const f128   xy = set4f(x, y, x, y);
    const size_t count2 = count & ~1ULL;
    for (size_t i = 0, n = count2 * 2; i < n; i += 4) {
        const f128 temp = loadu4f(array + i);
        const f128 cmp = cmpne4f(temp, xy);
        if (movemask4f(cmp))
            return false;
    }
    if (count & 1) {
        const float nx = array[count2 * 2];
        const float ny = array[count2 * 2 + 1];
        if (nx != x || ny != y)
            return false;
    }
    return true;

#else
    const float x = array[0];
    const float y = array[1];
    for (size_t i = 2, n = count * 2; i < n; i += 2) {
        if (x != array[i] || y != array[i + 1]) {
            return false;
        }
    }
    return true;
#endif
}

//----------------------------------------------------------------------------------------------------------------------
bool vec3AreAllTheSame(const float* array, size_t count)
{
    // if already at the end of the array, we're done
    if (count <= 1) {
        return true;
    }
#ifdef __AVX2__

    const float x = array[0];
    const float y = array[1];
    const float z = array[2];

    // test the first 8 in the array
    for (int32_t i = 3, n = 3 * std::min(size_t(8), count); i < n; i += 3) {
        if (x != array[i] || y != array[i + 1] || z != array[i + 2])
            return false;
    }
    // if already at the end of the array, we're done
    if (count <= 8) {
        return true;
    }

    // load 8 vec3s
    const f256 first8[3] = { loadu8f(array + 0), loadu8f(array + 8), loadu8f(array + 16) };

    // now test groups of 8 x 3D vectors
    size_t count8 = count & ~7ULL;
    for (int32_t i = 3 * 8, n = 3 * count8; i < n; i += 3 * 8) {
        const f256 a = loadu8f(array + i + 0);
        const f256 b = loadu8f(array + i + 8);
        const f256 c = loadu8f(array + i + 16);
        const f256 cmpa = cmpne8f(first8[0], a);
        const f256 cmpb = cmpne8f(first8[1], b);
        const f256 cmpc = cmpne8f(first8[2], c);
        const f256 cmp = or8f(or8f(cmpa, cmpb), cmpc);
        if (movemask8f(cmp))
            return false;
    }

    // now test a final group of 4 x 3D vectors
    if (count & 4) {
        const f128 a = loadu4f(array + 3 * count8 + 0);
        const f128 b = loadu4f(array + 3 * count8 + 4);
        const f128 c = loadu4f(array + 3 * count8 + 8);
        const f128 cmpa = cmpne4f(extract4f(first8[0], 0), a);
        const f128 cmpb = cmpne4f(extract4f(first8[0], 1), b);
        const f128 cmpc = cmpne4f(extract4f(first8[1], 0), c);
        const f128 cmp = or4f(or4f(cmpa, cmpb), cmpc);
        if (movemask4f(cmp))
            return false;
        count8 += 4;
    }

    // and now the remaining three
    if (count & 3) {
        for (int i = 3 * count8, n = 3 * count; i < n; i += 3) {
            if (x != array[i] || y != array[i + 1] || z != array[i + 2]) {
                return false;
            }
        }
    }
    return true;

#elif defined(__SSE__)

    const float x = array[0];
    const float y = array[1];
    const float z = array[2];

    // test the first 8 in the array
    for (int32_t i = 3, n = 3 * std::min(size_t(4), count); i < n; i += 3) {
        if (x != array[i] || y != array[i + 1] || z != array[i + 2])
            return false;
    }
    // if already at the end of the array, we're done
    if (count <= 4) {
        return true;
    }

    // load 8 vec3s
    const f128 first4[3] = { loadu4f(array + 0), loadu4f(array + 4), loadu4f(array + 8) };

    // now test groups of 8 x 3D vectors
    const size_t count4 = count & ~3ULL;
    for (int32_t i = 3 * 4, n = 3 * count4; i < n; i += 3 * 4) {
        const f128 a = loadu4f(array + i + 0);
        const f128 b = loadu4f(array + i + 4);
        const f128 c = loadu4f(array + i + 8);
        const f128 cmpa = cmpne4f(first4[0], a);
        const f128 cmpb = cmpne4f(first4[1], b);
        const f128 cmpc = cmpne4f(first4[2], c);
        const f128 cmp = or4f(or4f(cmpa, cmpb), cmpc);
        if (movemask4f(cmp))
            return false;
    }

    // and now the remaining three
    if (count & 3) {
        for (int i = 3 * count4, n = 3 * count; i < n; i += 3) {
            if (x != array[i] || y != array[i + 1] || z != array[i + 2]) {
                return false;
            }
        }
    }
    return true;
#else
    const float x = array[0];
    const float y = array[1];
    const float z = array[2];
    for (size_t i = 3, n = count * 3; i < n; i += 3) {
        if (x != array[i] || y != array[i + 1] || z != array[i + 2]) {
            return false;
        }
    }
    return true;
#endif
}

//----------------------------------------------------------------------------------------------------------------------
bool vec4AreAllTheSame(const float* array, size_t count)
{
    // if already at the end of the array, we're done
    if (count <= 1) {
        return true;
    }
#ifdef __AVX2__

    const f128 first = load4f(array + 0);
    const f256 pair = set8f(first, first);

    const size_t count2 = count & ~1ULL;
    for (size_t i = 0, n = count2 * 4; i < n; i += 8) {
        const f256 temp = loadu8f(array + i);
        const f256 cmp = cmpne8f(temp, pair);
        if (movemask8f(cmp))
            return false;
    }
    if (count & 1) {
        const f128 temp = loadu4f(array + (count2 << 2));
        const f128 cmp = cmpne4f(temp, cast4f(pair));
        if (movemask4f(cmp))
            return false;
    }
    return true;

#elif defined(__SSE__)

    const f128 first = load4f(array + 0);
    for (size_t i = 4, n = count * 4; i < n; i += 4) {
        const f128 temp = loadu4f(array + i);
        const f128 cmp = cmpne4f(temp, first);
        if (movemask4f(cmp))
            return false;
    }
    return true;

#else
    const float x = array[0];
    const float y = array[1];
    const float z = array[2];
    const float w = array[3];
    for (size_t i = 4, n = count * 4; i < n; i += 4) {
        if (x != array[i] || y != array[i + 1] || z != array[i + 2] || w != array[i + 3]) {
            return false;
        }
    }
    return true;
#endif
}

//----------------------------------------------------------------------------------------------------------------------
bool vec2AreAllTheSame(const double* array, size_t count)
{

    // if already at the end of the array, we're done
    if (count <= 1) {
        return true;
    }
#ifdef __AVX2__

    const d128   xy = loadu2d(array);
    const d256   xyxy = set4d(xy, xy);
    const size_t count2 = count & ~1ULL;
    for (size_t i = 0, n = count2 * 2; i < n; i += 4) {
        const d256 temp = loadu4d(array + i);
        const d256 cmp = cmpne4d(temp, xyxy);
        if (movemask4d(cmp))
            return false;
    }
    if (count & 1) {
        const d128 temp = loadu2d(array + count2 * 2);
        const d128 cmp = cmpne2d(temp, xy);
        if (movemask2d(cmp))
            return false;
    }
    return true;

#elif defined(__SSE__)

    const d128 xy = loadu2d(array);
    for (size_t i = 2, n = count * 2; i < n; i += 2) {
        const d128 temp = loadu2d(array + i);
        const d128 cmp = cmpne2d(temp, xy);
        if (movemask2d(cmp))
            return false;
    }
    return true;

#else
    const double x = array[0];
    const double y = array[1];
    for (size_t i = 2, n = count * 2; i < n; i += 2) {
        if (x != array[i] || y != array[i + 1]) {
            return false;
        }
    }
    return true;
#endif
}

//----------------------------------------------------------------------------------------------------------------------
bool vec3AreAllTheSame(const double* array, size_t count)
{

    // if already at the end of the array, we're done
    if (count <= 1) {
        return true;
    }
#ifdef __AVX2__

    const double x = array[0];
    const double y = array[1];
    const double z = array[2];

    // test the first 4 in the array
    for (int32_t i = 3, n = 3 * std::min(size_t(4), count); i < n; i += 3) {
        if (x != array[i] || y != array[i + 1] || z != array[i + 2])
            return false;
    }
    // if already at the end of the array, we're done
    if (count <= 4) {
        return true;
    }

    // load 8 vec3s
    const d256 first4[3] = { loadu4d(array + 0), loadu4d(array + 4), loadu4d(array + 8) };

    // now test groups of 8 x 3D vectors
    const size_t count4 = count & ~3ULL;
    for (int32_t i = 3 * 4, n = 3 * count4; i < n; i += 3 * 4) {
        const d256 a = loadu4d(array + i + 0);
        const d256 b = loadu4d(array + i + 4);
        const d256 c = loadu4d(array + i + 8);
        const d256 cmpa = cmpne4d(first4[0], a);
        const d256 cmpb = cmpne4d(first4[1], b);
        const d256 cmpc = cmpne4d(first4[2], c);
        const d256 cmp = or4d(or4d(cmpa, cmpb), cmpc);
        if (movemask4d(cmp))
            return false;
    }

    // and now the remaining three
    if (count & 3) {
        for (int i = 3 * count4, n = 3 * count; i < n; i += 3) {
            if (x != array[i] || y != array[i + 1] || z != array[i + 2]) {
                return false;
            }
        }
    }
    return true;
#elif defined(__SSE__)

    const double x = array[0];
    const double y = array[1];
    const double z = array[2];

    // test the first 2 in the array
    if (x != array[3] || y != array[4] || z != array[5])
        return false;

    // if already at the end of the array, we're done
    if (count <= 2) {
        return true;
    }

    // load 8 vec3s
    const d128 first4[3] = { loadu2d(array + 0), loadu2d(array + 2), loadu2d(array + 4) };

    // now test groups of 8 x 3D vectors
    const size_t count2 = count & ~1ULL;
    for (int32_t i = 3 * 2, n = 3 * count2; i < n; i += 3 * 2) {
        const d128 a = loadu2d(array + i + 0);
        const d128 b = loadu2d(array + i + 2);
        const d128 c = loadu2d(array + i + 4);
        const d128 cmpa = cmpne2d(first4[0], a);
        const d128 cmpb = cmpne2d(first4[1], b);
        const d128 cmpc = cmpne2d(first4[2], c);
        const d128 cmp = or2d(or2d(cmpa, cmpb), cmpc);
        if (movemask2d(cmp))
            return false;
    }

    // and now the remaining three
    if (count & 1) {
        if (x != array[count2 * 3] || y != array[count2 * 3 + 1] || z != array[count2 * 3 + 2]) {
            return false;
        }
    }
    return true;
#else
    const double x = array[0];
    const double y = array[1];
    const double z = array[2];
    for (size_t i = 3, n = count * 3; i < n; i += 3) {
        if (x != array[i] || y != array[i + 1] || z != array[i + 2]) {
            return false;
        }
    }
    return true;
#endif
}

//----------------------------------------------------------------------------------------------------------------------
bool vec4AreAllTheSame(const double* array, size_t count)
{
    // if already at the end of the array, we're done
    if (count <= 1) {
        return true;
    }

#ifdef __AVX2__
    const d256 first = loadu4d(array + 0);
    for (size_t i = 4, n = count * 4; i < n; i += 4) {
        const d256 temp = loadu4d(array + i);
        const d256 cmp = cmpne4d(temp, first);
        if (movemask4d(cmp))
            return false;
    }
    return true;
#elif defined(__SSE__)
    const d128 xy = loadu2d(array + 0);
    const d128 zw = loadu2d(array + 2);
    for (size_t i = 4, n = count * 4; i < n; i += 4) {
        const d128 tempxy = loadu2d(array + i);
        const d128 tempzw = loadu2d(array + i + 2);
        const d128 cmpxy = cmpne2d(tempxy, xy);
        const d128 cmpzw = cmpne2d(tempzw, zw);
        if (movemask2d(or2d(cmpxy, cmpzw)))
            return false;
    }
    return true;
#else
    const double x = array[0];
    const double y = array[1];
    const double z = array[2];
    const double w = array[3];
    for (size_t i = 4, n = count * 4; i < n; i += 4) {
        if (x != array[i] || y != array[i + 1] || z != array[i + 2] || w != array[i + 3]) {
            return false;
        }
    }
    return true;
#endif
}

//----------------------------------------------------------------------------------------------------------------------
bool compareArray(
    const GfHalf* const input0,
    const float* const  input1,
    const size_t        count0,
    const size_t        count1,
    const float         eps)
{
    if (count0 != count1) {
        return false;
    }
#ifdef __AVX2__
    const f256   eps8 = splat8f(eps);
    const size_t count8 = count0 & ~0x7ULL;
    size_t       i = 0;

    // check all values that can be processed in blocks of 8
    for (; i < count8; i += 8) {
        const i128 in0 = loadu4i(input0 + i);
        const f256 in1 = loadu8f(input1 + i);
        const f256 diff = abs8f(sub8f(cvtph8(in0), in1));
        const f256 cmp = cmpgt8f(diff, eps8);
        if (movemask8f(cmp))
            return false;
    }

    // use a masked load to load the last 0 -> 7 elements in each array. The unused
    // elements will be set to zero, so the if(diff > eps) test should return 0
    // in the movemask for those elements.
    const f256         in1 = loadmask7f(input1 + i, count0);
    alignas(16) GfHalf values[8] = { 0 };
    for (uint16_t j = 0, n = (count0 & 0x7); j < n; ++i, ++j)
        values[j] = input0[i];
    const f256 in0 = cvtph8(load4i(values));
    const f256 diff = abs8f(sub8f(in0, in1));
    const f256 cmp = cmpgt8f(diff, eps8);
    return movemask8f(cmp) == 0;

#elif defined(__SSE__)
    const f128   eps4 = splat4f(eps);
    const size_t count4 = count0 & ~0x3ULL;
    size_t       i = 0;
    for (; i < count4; i += 4) {
        const f128 in1 = loadu4f(input1 + i);
// if HW float16 support available
#ifdef __F16C__
        const i128 in0 = load2i(input0 + i);
        const f128 diff = abs4f(sub4f(cvtph4(in0), in1));
#else
        const f128 temp = set4f(input0[i], input0[i + 1], input0[i + 2], input0[i + 3]);
        const f128 diff = abs4f(sub4f(temp, in1));
#endif
        const f128 cmp = cmpgt4f(diff, eps4);
        if (movemask4f(cmp))
            return false;
    }

    // check the final 3 elements (deliberate fallthrough in switch cases)
    // using switch to make sure the compiler isn't *clever* and inserts an
    // optimised loop (clang 5.0 can't optimise the loop in this case).
    bool result = true;
    switch (count0 & 0x3) {
    case 3: result = result & (std::abs(input0[i + 2] - input1[i + 2]) <= eps);
    case 2: result = result & (std::abs(input0[i + 1] - input1[i + 1]) <= eps);
    case 1: result = result & (std::abs(input0[i + 0] - input1[i + 0]) <= eps);
    default: break;
    }
    return result;
#else
    for (size_t i = 0; i < count0; ++i) {
        if (std::abs(float(input0[i]) - float(input1[i])) > eps) {
            return false;
        }
    }
    return true;
#endif
}

//----------------------------------------------------------------------------------------------------------------------
bool compareArray(
    const GfHalf* const input0,
    const double* const input1,
    const size_t        count0,
    const size_t        count1,
    const double        eps)
{
    if (count0 != count1) {
        return false;
    }
    // TODO: the AVX2 and SSE version are incorrect. On Linux, these fail. Disable them for now.
#if defined(__AVX2__) && 0
    const f256   eps8 = splat8f(eps);
    const size_t count8 = count0 & ~0x7ULL;
    size_t       i = 0;

    // check all values that can be processed in blocks of 8
    for (; i < count8; i += 8) {
        const i128 in0 = loadu4i(input0 + i);
        const f128 in1a = cvt4d_to_4f(loadu4d(input1 + i));
        const f128 in1b = cvt4d_to_4f(loadu4d(input1 + i + 4));
        const f256 in1 = set2f128(in1a, in1b);
        const f256 diff = abs8f(sub8f(cvtph8(in0), in1));
        const f256 cmp = cmpgt8f(diff, eps8);
        if (movemask8f(cmp)) {
            return false;
        }
    }
    alignas(16) GfHalf a[8] = { 0 };
    for (int j = 0, k = i, n = count0 % 8; j < n; ++k, ++j) {
        a[j] = input0[k];
    }

    const f256 in0 = cvtph8(loadu4i(a));
    f256       in1;
    if (count0 & 0x4) {
        const f128 in1a = cvt4d_to_4f(loadu4d(input1 + i));
        const f128 in1b = cvt4d_to_4f(loadmask3d(input1 + i + 4, count0));
        in1 = set2f128(in1a, in1b);
    } else {
        const f128 in1a = cvt4d_to_4f(loadmask3d(input1 + i, count0));
        in1 = set2f128(in1a, zero4f());
    }
    const f256 diff = abs8f(sub8f(in0, in1));
    const f256 cmp = cmpgt8f(diff, eps8);
    if (movemask8f(cmp))
        return false;

    return true;

#elif defined(__SSE__) && 0
    const f128   eps4 = splat4f(eps);
    const size_t count4 = count0 & ~0x3ULL;
    size_t       i = 0;
    for (; i < count4; i += 4) {
        const f128 in1a = cvt2d_to_2f(loadu2d(input1 + i));
        const f128 in1b = cvt2d_to_2f(loadu2d(input1 + i + 2));
        const f128 in1 = movelh4f(in1a, in1b);

// if HW float16 support available
#ifdef __F16C__
        const i128 in0 = load2i(input0 + i);
        const f128 diff = abs4f(sub4f(cvtph4(in0), in1));
#else
        const f128 temp = set4f(input0[i], input0[i + 1], input0[i + 2], input0[i + 3]);
        const f128 diff = abs4f(sub4f(temp, in1));
#endif

        const f128 cmp = cmpgt4f(diff, eps4);
        if (movemask4f(cmp))
            return false;
    }

    // check the final 3 elements (deliberate fallthrough in switch cases)
    // using switch to make sure the compiler isn't *clever* and inserts an
    // optimised loop (clang 5.0 can't optimise the loop in this case).
    bool result = true;
    switch (count0 & 0x3) {
    case 3: result = result & (float(std::abs(input0[i + 2]) - float(input1[i + 2])) <= eps);
    case 2: result = result & (float(std::abs(input0[i + 1]) - float(input1[i + 1])) <= eps);
    case 1: result = result & (float(std::abs(input0[i + 0]) - float(input1[i + 0])) <= eps);
    default: break;
    }
    return result;
#else
    for (size_t i = 0; i < count0; ++i) {
        if (std::abs(float(input0[i]) - float(input1[i])) > eps)
            return false;
    }
    return true;
#endif
}

//----------------------------------------------------------------------------------------------------------------------
bool compareArray(
    const double* const input0,
    const float* const  input1,
    const size_t        count0,
    const size_t        count1,
    const float         eps)
{
    if (count0 != count1) {
        return false;
    }
    for (size_t i = 0; i < count0; ++i) {
        if (std::abs(input0[i] - input1[i]) > eps)
            return false;
    }
    return true;
}

//----------------------------------------------------------------------------------------------------------------------
bool compareArray(
    const double* const input0,
    const double* const input1,
    const size_t        count0,
    const size_t        count1,
    const double        eps)
{
    if (count0 != count1) {
        return false;
    }
#ifdef __AVX2__
    const d256   eps4 = splat4d(eps);
    const size_t count4 = count0 & ~0x3ULL;
    size_t       i = 0;

    // check all values that can be processed in blocks of 8
    for (; i < count4; i += 4) {
        const d256 in0 = loadu4d(input0 + i);
        const d256 in1 = loadu4d(input1 + i);
        const d256 diff = abs4d(sub4d(in0, in1));
        const d256 cmp = cmpgt4d(diff, eps4);
        if (movemask4d(cmp))
            return false;
    }

    // use a masked load to load the last 0 -> 7 elements in each array. The unused
    // elements will be set to zero, so the if(diff > eps) test should return 0
    // in the movemask for those elements.
    const d256 in0 = loadmask3d(input0 + i, count0);
    const d256 in1 = loadmask3d(input1 + i, count0);
    const d256 diff = abs4d(sub4d(in0, in1));
    const d256 cmp = cmpgt4d(diff, eps4);
    return movemask4d(cmp) == 0;

#elif defined(__SSE__)
    const d128   eps2 = splat2d(eps);
    const size_t count2 = count0 & ~0x1ULL;
    size_t       i = 0;
    for (; i < count2; i += 2) {
        const d128 in0 = loadu2d(input0 + i);
        const d128 in1 = loadu2d(input1 + i);
        const d128 diff = abs2d(sub2d(in0, in1));
        const d128 cmp = cmpgt2d(diff, eps2);
        if (movemask2d(cmp))
            return false;
    }

    // check the final element (If it's there)
    bool result = true;
    if (count0 & 0x1) {
        result = std::abs(input0[i] - input1[i]) <= eps;
    }
    return result;
#else
    for (size_t i = 0; i < count0; ++i) {
        if (std::abs(input0[i] - input1[i]) > eps)
            return false;
    }
    return true;
#endif
}

//----------------------------------------------------------------------------------------------------------------------
bool compareArray(
    const float* const input0,
    const float* const input1,
    const size_t       count0,
    const size_t       count1,
    const float        eps)
{
    if (count0 != count1) {
        return false;
    }
#ifdef __AVX2__
    const f256   eps8 = splat8f(eps);
    const size_t count8 = count0 & ~0x7ULL;
    size_t       i = 0;

    // check all values that can be processed in blocks of 8
    for (; i < count8; i += 8) {
        const f256 in0 = loadu8f(input0 + i);
        const f256 in1 = loadu8f(input1 + i);
        const f256 diff = abs8f(sub8f(in0, in1));
        const f256 cmp = cmpgt8f(diff, eps8);
        if (movemask8f(cmp)) {
            return false;
        }
    }

    // use a masked load to load the last 0 -> 7 elements in each array. The unused
    // elements will be set to zero, so the if(diff > eps) test should return 0
    // in the movemask for those elements.
    const f256 in0 = loadmask7f(input0 + i, count0);
    const f256 in1 = loadmask7f(input1 + i, count0);
    const f256 diff = abs8f(sub8f(in0, in1));
    const f256 cmp = cmpgt8f(diff, eps8);
    return movemask8f(cmp) == 0;

#elif defined(__SSE__)
    const f128   eps4 = splat4f(eps);
    const size_t count4 = count0 & ~0x3ULL;
    size_t       i = 0;
    for (; i < count4; i += 4) {
        const f128 in0 = loadu4f(input0 + i);
        const f128 in1 = loadu4f(input1 + i);
        const f128 diff = abs4f(sub4f(in0, in1));
        const f128 cmp = cmpgt4f(diff, eps4);

        if (movemask4f(cmp)) {
            return false;
        }
    }

    // check the final 3 elements (deliberate fallthrough in switch cases)
    // using switch to make sure the compiler isn't *clever* and inserts an
    // optimised loop (clang 5.0 can't optimise the loop in this case).
    bool result = true;
    switch (count0 & 0x3) {
    case 3: result = result & (std::abs(input0[i + 2] - input1[i + 2]) <= eps);
    case 2: result = result & (std::abs(input0[i + 1] - input1[i + 1]) <= eps);
    case 1: result = result & (std::abs(input0[i + 0] - input1[i + 0]) <= eps);
    default: break;
    }
    return result;
#else
    for (size_t i = 0; i < count0; ++i) {
        if (std::abs(input0[i] - input1[i]) > eps) {
            return false;
        }
    }
    return true;
#endif
}

//----------------------------------------------------------------------------------------------------------------------
bool compareArray(
    const GfHalf* const input0,
    const GfHalf* const input1,
    const size_t        count0,
    const size_t        count1,
    const float         eps)
{
    if (count0 != count1) {
        return false;
    }
    // TODO: write AVX2 and SSE optimized versions. (We don't expect to see half-floats, not a priority for now.)
    for (size_t i = 0; i < count0; ++i) {
        if (std::abs(input0[i] - input1[i]) > eps) {
            return false;
        }
    }
    return true;
}

//----------------------------------------------------------------------------------------------------------------------
bool compareArray(
    const int8_t* const input0,
    const int8_t* const input1,
    const size_t        count0,
    const size_t        count1)
{
    if (count0 != count1) {
        return false;
    }
#ifdef __AVX2__
    const size_t count32 = count0 & ~0x1FULL;
    size_t       i = 0;

    // check all values that can be processed in blocks of 8
    for (; i < count32; i += 32) {
        const i256 in0 = loadu8i(input0 + i);
        const i256 in1 = loadu8i(input1 + i);
        const i256 cmp = cmpeq32i8(in0, in1);
        if (~movemask32i8(cmp))
            return false;
    }

    alignas(32) uint8_t a[32] = { 0 };
    alignas(32) uint8_t b[32] = { 0 };
    for (int j = 0, n = count0 % 32; j < n; ++i, ++j) {
        a[j] = input0[i];
        b[j] = input1[i];
    }

    // use a masked load to load the last 0 -> 7 elements in each array. The unused
    // elements will be set to zero, so the if(diff > eps) test should return 0
    // in the movemask for those elements.
    const i256 in0 = load8i(a);
    const i256 in1 = load8i(b);
    const i256 cmp = cmpeq32i8(in0, in1);
    return movemask32i8(cmp) == -1;

#elif defined(__SSE__)
    const size_t count16 = count0 & ~0xFULL;
    size_t       i = 0;
    for (; i < count16; i += 16) {
        const i128 in0 = loadu4i(input0 + i);
        const i128 in1 = loadu4i(input1 + i);
        const i128 cmp = cmpeq16i8(in0, in1);
        if (0xFFFF & (~movemask16i8(cmp))) {
            return false;
        }
    }

    alignas(16) uint8_t a[16] = { 0 };
    alignas(16) uint8_t b[16] = { 0 };
    for (int j = 0; i < count0; ++i, ++j) {
        a[j] = input0[i];
        b[j] = input1[i];
    }

    // use a masked load to load the last 0 -> 7 elements in each array. The unused
    // elements will be set to zero, so the if(diff > eps) test should return 0
    // in the movemask for those elements.
    const i128 in0 = load4i(a);
    const i128 in1 = load4i(b);
    const i128 cmp = cmpeq16i8(in0, in1);
    return 0xFFFF == movemask16i8(cmp);
#else
    for (size_t i = 0; i < count0; ++i) {
        if (input0[i] != input1[i])
            return false;
    }
    return true;
#endif
}

//----------------------------------------------------------------------------------------------------------------------
bool compareArray(
    const int32_t* const input0,
    const int32_t* const input1,
    const size_t         count0,
    const size_t         count1)
{
    if (count0 != count1) {
        return false;
    }
#ifdef __AVX2__
    const size_t count8 = count0 & ~0x7ULL;
    size_t       i = 0;

    // check all values that can be processed in blocks of 8
    for (; i < count8; i += 8) {
        const i256 in0 = loadu8i(input0 + i);
        const i256 in1 = loadu8i(input1 + i);
        const i256 cmp = cmpeq8i(in0, in1);
        if (0xFF & (~movemask8i(cmp)))
            return false;
    }

    // use a masked load to load the last 0 -> 7 elements in each array. The unused
    // elements will be set to zero, so the if(diff > eps) test should return 0
    // in the movemask for those elements.
    const i256 in0 = loadmask7i(input0 + i, count0);
    const i256 in1 = loadmask7i(input1 + i, count0);
    const i256 cmp = cmpeq8i(in0, in1);
    return (0xFF & (~movemask8i(cmp))) == 0;

#elif defined(__SSE__)
    const size_t count4 = count0 & ~0x3ULL;
    size_t       i = 0;
    for (; i < count4; i += 4) {
        const i128 in0 = loadu4i(input0 + i);
        const i128 in1 = loadu4i(input1 + i);
        const i128 cmp = cmpeq4i(in0, in1);
        if (0xF & (~movemask4i(cmp)))
            return false;
    }

    // check the final 3 elements (deliberate fallthrough in switch cases)
    // using switch to make sure the compiler isn't *clever* and inserts an
    // optimised loop (clang 5.0 can't optimise the loop in this case).
    bool result = true;
    switch (count0 & 0x3) {
    case 3: result = result & (input0[i + 2] == input1[i + 2]);
    case 2: result = result & (input0[i + 1] == input1[i + 1]);
    case 1: result = result & (input0[i + 0] == input1[i + 0]);
    default: break;
    }
    return result;
#else
    for (size_t i = 0; i < count0; ++i) {
        if (input0[i] != input1[i])
            return false;
    }
    return true;
#endif
}

//----------------------------------------------------------------------------------------------------------------------
bool compareUvArray(
    const float* const u0,
    const float* const v0,
    const float* const uv1,
    const size_t       count0,
    const size_t       count1,
    const float        eps)
{
    if (count0 != count1) {
        return false;
    }

#ifdef __AVX2__

    const f256   eps8 = splat8f(eps);
    const size_t count8 = count0 & ~0x7ULL;
    size_t       i = 0, j = 0;

    // check all values that can be processed in blocks of 8
    for (; i < count8; i += 8, j += 16) {
        const f256 inu0 = loadu8f(u0 + i);
        const f256 inv0 = loadu8f(v0 + i);
        const f256 inuv1a = loadu8f(uv1 + j);
        const f256 inuv1b = loadu8f(uv1 + j + 8);

        // zip U and V arrays together
        const f256 xy0 = unpacklo8f(inu0, inv0);
        const f256 xy1 = unpackhi8f(inu0, inv0);
        const f256 inuv0a = permute128f<0, 2>(xy0, xy1);
        const f256 inuv0b = permute128f<1, 3>(xy0, xy1);

        const f256 diff0 = abs8f(sub8f(inuv0a, inuv1a));
        const f256 diff1 = abs8f(sub8f(inuv0b, inuv1b));
        const f256 cmp0 = cmpgt8f(diff0, eps8);
        const f256 cmp1 = cmpgt8f(diff1, eps8);
        if (movemask8f(cmp0) | movemask8f(cmp1))
            return false;
    }

    if (count0 != count8) {
        f256 inu0, inv0, inuv1a, inuv1b;
        if (count0 & 0x4) {
            inu0 = loadmask7f(u0 + i, count0);
            inv0 = loadmask7f(v0 + i, count0);
            inuv1a = loadu8f(uv1 + j);
            inuv1b = loadmask7f(uv1 + j + 8, count0 << 1);
        } else {
            inu0 = loadmask7f(u0 + i, count0);
            inv0 = loadmask7f(v0 + i, count0);
            inuv1a = loadmask7f(uv1 + j, count0 << 1);
            inuv1b = zero8f();
        }

        // zip U and V arrays together
        const f256 xy0 = unpacklo8f(inu0, inv0);
        const f256 xy1 = unpackhi8f(inu0, inv0);
        const f256 inuv0a = permute128f<0, 2>(xy0, xy1);
        const f256 inuv0b = permute128f<1, 3>(xy0, xy1);

        const f256 diff0 = abs8f(sub8f(inuv0a, inuv1a));
        const f256 diff1 = abs8f(sub8f(inuv0b, inuv1b));
        const f256 cmp0 = cmpgt8f(diff0, eps8);
        const f256 cmp1 = cmpgt8f(diff1, eps8);
        if (movemask8f(cmp0) | movemask8f(cmp1))
            return false;
    }

    return true;

#elif defined(__SSE__)

    const f128   eps4 = splat4f(eps);
    const size_t count4 = count0 & ~0x3ULL;
    size_t       i = 0, j = 0;

    // check all values that can be processed in blocks of 8
    for (; i < count4; i += 4, j += 8) {
        const f128 inu0 = loadu4f(u0 + i);
        const f128 inv0 = loadu4f(v0 + i);
        const f128 inuv1a = loadu4f(uv1 + j);
        const f128 inuv1b = loadu4f(uv1 + j + 4);

        // zip U and V arrays together
        const f128 inuv0a = unpacklo4f(inu0, inv0);
        const f128 inuv0b = unpackhi4f(inu0, inv0);

        const f128 diff0 = abs4f(sub4f(inuv0a, inuv1a));
        const f128 diff1 = abs4f(sub4f(inuv0b, inuv1b));
        const f128 cmp0 = cmpgt4f(diff0, eps4);
        const f128 cmp1 = cmpgt4f(diff1, eps4);
        if (movemask4f(cmp0) | movemask4f(cmp1))
            return false;
    }

    if (count0 != count4) {
        f128 inuv0a, inuv0b, inu1, inv1;
        if (count0 & 0x2) {
            inuv0a = loadu4f(uv1 + j);
            inuv0b = loadmask3f(uv1 + j + 4, count0 << 1);
            inu1 = loadmask3f(u0 + i, count0);
            inv1 = loadmask3f(v0 + i, count0);
        } else {
            inuv0a = loadmask3f(uv1 + j, count0 << 1);
            inuv0b = zero4f();
            inu1 = loadmask3f(u0 + i, count0);
            inv1 = loadmask3f(v0 + i, count0);
        }

        // zip U and V arrays together
        const f128 inuv1a = unpacklo4f(inu1, inv1);
        const f128 inuv1b = unpackhi4f(inu1, inv1);
        const f128 diff0 = abs4f(sub4f(inuv0a, inuv1a));
        const f128 diff1 = abs4f(sub4f(inuv0b, inuv1b));
        const f128 cmp0 = cmpgt4f(diff0, eps4);
        const f128 cmp1 = cmpgt4f(diff1, eps4);
        if (movemask4f(cmp0) | movemask4f(cmp1))
            return false;
    }

    return true;
#else
    for (size_t i = 0, j = 0; i < count0; ++i, j += 2) {
        if (std::abs(u0[i] - uv1[j + 0]) > eps || std::abs(v0[i] - uv1[j + 1]) > eps)
            return false;
    }
    return true;
#endif
}

//----------------------------------------------------------------------------------------------------------------------
bool compareUvArray(
    const float        u0,
    const float        v0,
    const float* const u1,
    const float* const v1,
    const size_t       count,
    const float        eps)
{
#ifdef __AVX2__
    const f256 U = splat8f(u0);
    const f256 V = splat8f(v0);

    const f256   eps8 = splat8f(eps);
    const size_t count8 = count & ~0x7ULL;
    size_t       i = 0;

    // check all values that can be processed in blocks of 4
    for (; i < count8; i += 8) {
        const f256 au1 = loadu8f(u1 + i);
        const f256 av1 = loadu8f(v1 + i);

        const f256 diffu = abs8f(sub8f(au1, U));
        const f256 diffv = abs8f(sub8f(av1, V));
        const f256 cmpu = cmpgt8f(diffu, eps8);
        const f256 cmpv = cmpgt8f(diffv, eps8);
        if (movemask8f(cmpu) || movemask8f(cmpv))
            return false;
    }

    if (count8 != count) {
        alignas(32) float utemp[8];
        alignas(32) float vtemp[8];
        storeu8f(utemp, U);
        storeu8f(vtemp, V);
        f256 inu0, inv0, inu1, inv1;
        inu0 = loadmask7f(utemp, count);
        inv0 = loadmask7f(utemp, count);
        inu1 = loadmask7f(u1 + i, count);
        inv1 = loadmask7f(v1 + i, count);

        const f256 diffu = abs8f(sub8f(inu0, inu1));
        const f256 diffv = abs8f(sub8f(inv0, inv1));
        const f256 cmpu = cmpgt8f(diffu, eps8);
        const f256 cmpv = cmpgt8f(diffv, eps8);
        if (movemask8f(cmpu) || movemask8f(cmpv))
            return false;
    }

    return true;

#elif defined(__SSE__)

    const f128 U = splat4f(u0);
    const f128 V = splat4f(v0);

    const f128   eps4 = splat4f(eps);
    const size_t count4 = count & ~0x3ULL;
    size_t       i = 0;

    // check all values that can be processed in blocks of 4
    for (; i < count4; i += 4) {
        const f128 au1 = loadu4f(u1 + i);
        const f128 av1 = loadu4f(v1 + i);

        const f128 diffu = abs4f(sub4f(au1, U));
        const f128 diffv = abs4f(sub4f(av1, V));
        const f128 cmpu = cmpgt4f(diffu, eps4);
        const f128 cmpv = cmpgt4f(diffv, eps4);
        if (movemask4f(cmpu) || movemask4f(cmpv))
            return false;
    }

    if (count4 != count) {
        bool result = true;
        switch (count & 0x3) {
        case 3: result = (std::abs(u0 - u1[i + 2]) <= eps && std::abs(v0 - v1[i + 2]) <= eps);
        case 2:
            result = result && (std::abs(u0 - u1[i + 1]) <= eps && std::abs(v0 - v1[i + 1]) <= eps);
        case 1:
            result = result && (std::abs(u0 - u1[i + 0]) <= eps && std::abs(v0 - v1[i + 0]) <= eps);
        default: break;
        }
        return result;
    }

    return true;

#else
    for (size_t i = 0; i < count; ++i) {
        if (std::abs(u0 - u1[i]) > eps || std::abs(v0 - v1[i]) > eps)
            return false;
    }
    return true;
#endif
}

//----------------------------------------------------------------------------------------------------------------------
bool compareArray3Dto4D(
    const float* const input3d,
    const float* const input4d,
    const size_t       count3d,
    const size_t       count4d,
    const float        eps)
{
    if (count3d != count4d) {
        return false;
    }

    for (size_t i = 0, j = 0, n = count3d * 3; i < n; i += 3, j += 4) {
        if (std::abs(input3d[i + 0] - input4d[j + 0]) > eps
            || std::abs(input3d[i + 1] - input4d[j + 1]) > eps
            || std::abs(input3d[i + 2] - input4d[j + 2]) > eps)
            return false;
    }
    return true;
}

//----------------------------------------------------------------------------------------------------------------------
bool compareArray3Dto4D(
    const float* const  input3d,
    const double* const input4d,
    const size_t        count3d,
    const size_t        count4d,
    const float         eps)
{
    if (count3d != count4d) {
        return false;
    }
#ifdef __AVX2__
    const f128 eps4 = splatf4f(eps);
    for (size_t i = 0; i < count3d; ++i) {
        const f128 float3d = loadmask3f(input3d + i * 3, 3);
        const d256 double4d = loadmask3d(input4d + i * 4, 3);
        const f128 float4d = cvt4d_to_4f(double4d);
        const f128 diff = abs4f(sub4f(float3d, float4d));
        const f128 cmp = cmpgt4f(diff, eps4);
        if (movemask4f(cmp))
            return false;
    }
#else
    for (size_t i = 0, j = 0, n = count3d * 3; i < n; i += 3, j += 4) {
        if (std::abs(input3d[i + 0] - input4d[j + 0]) > eps
            || std::abs(input3d[i + 1] - input4d[j + 1]) > eps
            || std::abs(input3d[i + 2] - input4d[j + 2]) > eps)
            return false;
    }
    return true;
#endif
}

//----------------------------------------------------------------------------------------------------------------------
bool compareRGBAArray(
    const float        r,
    const float        g,
    const float        b,
    const float        a,
    const float* const rgba,
    const size_t       count,
    const float        eps)
{
#ifdef __AVX2__
    const f256   colour = set8f(r, g, b, a, r, g, b, a);
    const f256   eps8 = splat8f(eps);
    const size_t count2 = count & ~0x1ULL;
    size_t       i = 0;

    // check all values that can be processed in blocks of 4
    for (; i < count2 * 4; i += 8) {
        const f256 in = loadu8f(rgba + i);
        const f256 diff = abs8f(sub8f(in, colour));
        const f256 cmp = cmpgt8f(diff, eps8);
        if (movemask8f(cmp))
            return false;
    }

    if (count & 1) {
        const f128 in = loadu4f(rgba + i);
        const f128 diff = abs4f(sub4f(in, cast4f(colour)));
        const f128 cmp = cmpgt4f(diff, cast4f(eps8));
        if (movemask4f(cmp))
            return false;
    }
#elif defined(__SSE__)
    const f128 colour = set4f(r, g, b, a);
    const f128 eps4 = splat4f(eps);

    // check all values that can be processed in blocks of 4
    for (size_t i = 0; i < count * 4; i += 4) {
        const f128 in = loadu4f(rgba + i);
        const f128 diff = abs4f(sub4f(in, colour));
        const f128 cmp = cmpgt4f(diff, eps4);
        if (movemask4f(cmp))
            return false;
    }

#else
    for (size_t i = 0; i < count * 4; i += 4) {
        if (std::abs(rgba[i + 0] - r) > eps || std::abs(rgba[i + 1] - g) > eps
            || std::abs(rgba[i + 2] - b) > eps || std::abs(rgba[i + 3] - a) > eps)
            return false;
    }
#endif
    return true;
}

} // namespace MayaUsdUtils
