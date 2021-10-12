//
// Copyright 2021 Autodesk
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
#include "DiffPrims.h"

#include <pxr/base/gf/vec2d.h>
#include <pxr/base/gf/vec2f.h>
#include <pxr/base/gf/vec2h.h>
#include <pxr/base/gf/vec2i.h>
#include <pxr/base/gf/vec3d.h>
#include <pxr/base/gf/vec3f.h>
#include <pxr/base/gf/vec3h.h>
#include <pxr/base/gf/vec3i.h>
#include <pxr/base/gf/vec4d.h>
#include <pxr/base/gf/vec4f.h>
#include <pxr/base/gf/vec4h.h>
#include <pxr/base/gf/vec4i.h>
#include <pxr/base/tf/type.h>
#include <pxr/base/vt/array.h>
#include <pxr/base/vt/value.h>
#include <pxr/usd/sdf/valueTypeName.h>

namespace MayaUsdUtils {

using VtValue = PXR_NS::VtValue;
using TfType = PXR_NS::TfType;

namespace {

//----------------------------------------------------------------------------------------------------------------------
// Registry of functions that can compare values of two given types and return a DiffResult.
//
// The registry always returns a function: for incompatible types, it returns a function that
// always returns DiffResult::Differ.
//
// For a duo of empty values, it returns a function that always returns DiffResult::Same.

using DiffFunc = std::function<DiffResult(const VtValue& modified, const VtValue& baseline)>;
using DiffKey = std::pair<std::type_index, std::type_index>;
using DiffFuncMap = std::map<DiffKey, DiffFunc>;

template <class T1, class T2>
DiffResult diffTwoTypesWithEps(const VtValue& modified, const VtValue& baseline)
{
    const T1& v1 = modified.Get<T1>();
    const T2& v2 = baseline.Get<T2>();
    return compareArray(&v1, &v2, 1, 1) ? DiffResult::Same : DiffResult::Differ;
}

template <class T1, class T2>
DiffResult diffTwoArrayTypesWithEps(const VtValue& modified, const VtValue& baseline)
{
    const VtArray<T1>& v1 = modified.Get<VtArray<T1>>();
    const VtArray<T2>& v2 = baseline.Get<VtArray<T2>>();
    return compareArray(v1.cdata(), v2.cdata(), modified.GetArraySize(), baseline.GetArraySize())
        ? DiffResult::Same
        : DiffResult::Differ;
}

template <class T> DiffResult diffOneTypeWithEps(const VtValue& modified, const VtValue& baseline)
{
    return diffTwoTypesWithEps<T, T>(modified, baseline);
}

template <class T>
DiffResult diffOneArrayTypeWithEps(const VtValue& modified, const VtValue& baseline)
{
    return diffTwoArrayTypesWithEps<T, T>(modified, baseline);
}

template <class V1, class V2, int SIZE>
DiffResult diffTwoVecs(const VtValue& modified, const VtValue& baseline)
{
    const V1& v1 = modified.Get<V1>();
    const V2& v2 = baseline.Get<V2>();
    return compareArray(v1.data(), v2.data(), SIZE, SIZE) ? DiffResult::Same : DiffResult::Differ;
}

template <class V1, class V2, int SIZE>
DiffResult diffTwoVecArrays(const VtValue& modified, const VtValue& baseline)
{
    const VtArray<V1>& v1 = modified.Get<VtArray<V1>>();
    const VtArray<V2>& v2 = baseline.Get<VtArray<V2>>();
    using V1ValueType = typename V1::ScalarType;
    using V2ValueType = typename V2::ScalarType;
    return compareArray(
               reinterpret_cast<const V1ValueType*>(v1.cdata()),
               reinterpret_cast<const V2ValueType*>(v2.cdata()),
               modified.GetArraySize() * SIZE,
               baseline.GetArraySize() * SIZE)
        ? DiffResult::Same
        : DiffResult::Differ;
}

template <class V1, class V2, int SIZE>
DiffResult diffTwoQuats(const VtValue& modified, const VtValue& baseline)
{
    const V1& v1 = modified.Get<V1>();
    const V2& v2 = baseline.Get<V2>();
    using V1ValueType = typename V1::ScalarType;
    using V2ValueType = typename V2::ScalarType;
    return compareArray(
               reinterpret_cast<const V1ValueType*>(&v1),
               reinterpret_cast<const V2ValueType*>(&v2),
               SIZE,
               SIZE)
        ? DiffResult::Same
        : DiffResult::Differ;
}

template <class V1, class V2, int SIZE>
DiffResult diffTwoQuatArrays(const VtValue& modified, const VtValue& baseline)
{
    const VtArray<V1>& v1 = modified.Get<VtArray<V1>>();
    const VtArray<V2>& v2 = baseline.Get<VtArray<V2>>();
    using V1ValueType = typename V1::ScalarType;
    using V2ValueType = typename V2::ScalarType;
    return compareArray(
               reinterpret_cast<const V1ValueType*>(v1.cdata()),
               reinterpret_cast<const V2ValueType*>(v2.cdata()),
               modified.GetArraySize() * SIZE,
               baseline.GetArraySize() * SIZE)
        ? DiffResult::Same
        : DiffResult::Differ;
}

template <class T> DiffResult diffGenericValues(const VtValue& modified, const VtValue& baseline)
{
    const T& v1 = modified.Get<T>();
    const T& v2 = baseline.Get<T>();
    return v1 == v2 ? DiffResult::Same : DiffResult::Differ;
}

DiffResult diffIncomparables(const VtValue& /*modified*/, const VtValue& /*baseline*/)
{
    return DiffResult::Differ;
}

DiffResult diffEmpties(const VtValue& /*modified*/, const VtValue& /*baseline*/)
{
    return DiffResult::Same;
}

#define MAYA_USD_DIFF_FUNC_FOR_TYPE_WITH_EPS(T)                                     \
    { DiffKey(typeid(T), typeid(T)), diffOneTypeWithEps<T> },                       \
    {                                                                               \
        DiffKey(typeid(VtArray<T>), typeid(VtArray<T>)), diffOneArrayTypeWithEps<T> \
    }

#define MAYA_USD_DIFF_FUNC_FOR_TYPES_WITH_EPS(T1, T2)                                       \
    { DiffKey(typeid(T1), typeid(T2)), diffTwoTypesWithEps<T1, T2> },                       \
    {                                                                                       \
        DiffKey(typeid(VtArray<T1>), typeid(VtArray<T2>)), diffTwoArrayTypesWithEps<T1, T2> \
    }

#define MAYA_USD_DIFF_FUNC_FOR_VEC(T, SIZE)                                           \
    { DiffKey(typeid(T), typeid(T)), diffTwoVecs<T, T, SIZE> },                       \
    {                                                                                 \
        DiffKey(typeid(VtArray<T>), typeid(VtArray<T>)), diffTwoVecArrays<T, T, SIZE> \
    }

#define MAYA_USD_DIFF_FUNC_FOR_VECS(T1, T2, SIZE)                                         \
    { DiffKey(typeid(T1), typeid(T2)), diffTwoVecs<T1, T2, SIZE> },                       \
    {                                                                                     \
        DiffKey(typeid(VtArray<T1>), typeid(VtArray<T2>)), diffTwoVecArrays<T1, T2, SIZE> \
    }

#define MAYA_USD_DIFF_FUNC_FOR_QUAT(T, SIZE)                                           \
    { DiffKey(typeid(T), typeid(T)), diffTwoQuats<T, T, SIZE> },                       \
    {                                                                                  \
        DiffKey(typeid(VtArray<T>), typeid(VtArray<T>)), diffTwoQuatArrays<T, T, SIZE> \
    }

#define MAYA_USD_DIFF_FUNC_FOR_QUATS(T1, T2, SIZE)                                         \
    { DiffKey(typeid(T1), typeid(T2)), diffTwoQuats<T1, T2, SIZE> },                       \
    {                                                                                      \
        DiffKey(typeid(VtArray<T1>), typeid(VtArray<T2>)), diffTwoQuatArrays<T1, T2, SIZE> \
    }

#define MAYA_USD_DIFF_FUNC_FOR_GENERIC_TYPE(T)                                         \
    { DiffKey(typeid(T), typeid(T)), diffGenericValues<T> },                           \
    {                                                                                  \
        DiffKey(typeid(VtArray<T>), typeid(VtArray<T>)), diffGenericValues<VtArray<T>> \
    }

const DiffFuncMap& getDiffFuncs()
{

    // Initializing static data inside a function makes it automatically initialization-order safe.
    static DiffFuncMap diffs
        = { MAYA_USD_DIFF_FUNC_FOR_TYPE_WITH_EPS(GfHalf),
            MAYA_USD_DIFF_FUNC_FOR_TYPES_WITH_EPS(GfHalf, float),
            MAYA_USD_DIFF_FUNC_FOR_TYPES_WITH_EPS(GfHalf, double),

            MAYA_USD_DIFF_FUNC_FOR_TYPE_WITH_EPS(float),
            MAYA_USD_DIFF_FUNC_FOR_TYPES_WITH_EPS(float, GfHalf),
            MAYA_USD_DIFF_FUNC_FOR_TYPES_WITH_EPS(float, double),

            MAYA_USD_DIFF_FUNC_FOR_TYPE_WITH_EPS(double),
            MAYA_USD_DIFF_FUNC_FOR_TYPES_WITH_EPS(double, GfHalf),
            MAYA_USD_DIFF_FUNC_FOR_TYPES_WITH_EPS(double, float),

            MAYA_USD_DIFF_FUNC_FOR_TYPE_WITH_EPS(int8_t),
            MAYA_USD_DIFF_FUNC_FOR_TYPE_WITH_EPS(uint8_t),
            MAYA_USD_DIFF_FUNC_FOR_TYPE_WITH_EPS(int16_t),
            MAYA_USD_DIFF_FUNC_FOR_TYPE_WITH_EPS(uint16_t),
            MAYA_USD_DIFF_FUNC_FOR_TYPE_WITH_EPS(int32_t),
            MAYA_USD_DIFF_FUNC_FOR_TYPE_WITH_EPS(uint32_t),
            MAYA_USD_DIFF_FUNC_FOR_TYPE_WITH_EPS(int64_t),
            MAYA_USD_DIFF_FUNC_FOR_TYPE_WITH_EPS(uint64_t),

            MAYA_USD_DIFF_FUNC_FOR_VEC(GfVec2d, 2),
            MAYA_USD_DIFF_FUNC_FOR_VEC(GfVec2f, 2),
            MAYA_USD_DIFF_FUNC_FOR_VEC(GfVec2h, 2),
            MAYA_USD_DIFF_FUNC_FOR_VEC(GfVec2i, 2),

            MAYA_USD_DIFF_FUNC_FOR_VECS(GfVec2d, GfVec2f, 2),
            MAYA_USD_DIFF_FUNC_FOR_VECS(GfVec2d, GfVec2h, 2),
            MAYA_USD_DIFF_FUNC_FOR_VECS(GfVec2f, GfVec2d, 2),
            MAYA_USD_DIFF_FUNC_FOR_VECS(GfVec2f, GfVec2h, 2),
            MAYA_USD_DIFF_FUNC_FOR_VECS(GfVec2h, GfVec2d, 2),
            MAYA_USD_DIFF_FUNC_FOR_VECS(GfVec2h, GfVec2f, 2),

            MAYA_USD_DIFF_FUNC_FOR_VEC(GfVec3d, 3),
            MAYA_USD_DIFF_FUNC_FOR_VEC(GfVec3f, 3),
            MAYA_USD_DIFF_FUNC_FOR_VEC(GfVec3h, 3),
            MAYA_USD_DIFF_FUNC_FOR_VEC(GfVec3i, 3),

            MAYA_USD_DIFF_FUNC_FOR_VECS(GfVec3d, GfVec3f, 3),
            MAYA_USD_DIFF_FUNC_FOR_VECS(GfVec3d, GfVec3h, 3),
            MAYA_USD_DIFF_FUNC_FOR_VECS(GfVec3f, GfVec3d, 3),
            MAYA_USD_DIFF_FUNC_FOR_VECS(GfVec3f, GfVec3h, 3),
            MAYA_USD_DIFF_FUNC_FOR_VECS(GfVec3h, GfVec3d, 3),
            MAYA_USD_DIFF_FUNC_FOR_VECS(GfVec3h, GfVec3f, 3),

            MAYA_USD_DIFF_FUNC_FOR_VEC(GfVec4d, 4),
            MAYA_USD_DIFF_FUNC_FOR_VEC(GfVec4f, 4),
            MAYA_USD_DIFF_FUNC_FOR_VEC(GfVec4h, 4),
            MAYA_USD_DIFF_FUNC_FOR_VEC(GfVec4i, 4),

            MAYA_USD_DIFF_FUNC_FOR_VECS(GfVec4d, GfVec4f, 4),
            MAYA_USD_DIFF_FUNC_FOR_VECS(GfVec4d, GfVec4h, 4),
            MAYA_USD_DIFF_FUNC_FOR_VECS(GfVec4f, GfVec4d, 4),
            MAYA_USD_DIFF_FUNC_FOR_VECS(GfVec4f, GfVec4h, 4),
            MAYA_USD_DIFF_FUNC_FOR_VECS(GfVec4h, GfVec4d, 4),
            MAYA_USD_DIFF_FUNC_FOR_VECS(GfVec4h, GfVec4f, 4),

            MAYA_USD_DIFF_FUNC_FOR_VEC(GfMatrix2d, 4),
            MAYA_USD_DIFF_FUNC_FOR_VEC(GfMatrix3d, 9),
            MAYA_USD_DIFF_FUNC_FOR_VEC(GfMatrix4d, 16),

            MAYA_USD_DIFF_FUNC_FOR_QUAT(GfQuatd, 4),
            MAYA_USD_DIFF_FUNC_FOR_QUAT(GfQuatf, 4),
            MAYA_USD_DIFF_FUNC_FOR_QUAT(GfQuath, 4),

            MAYA_USD_DIFF_FUNC_FOR_QUATS(GfQuatd, GfQuatf, 4),
            MAYA_USD_DIFF_FUNC_FOR_QUATS(GfQuatd, GfQuath, 4),
            MAYA_USD_DIFF_FUNC_FOR_QUATS(GfQuatf, GfQuatd, 4),
            MAYA_USD_DIFF_FUNC_FOR_QUATS(GfQuatf, GfQuath, 4),
            MAYA_USD_DIFF_FUNC_FOR_QUATS(GfQuath, GfQuatd, 4),
            MAYA_USD_DIFF_FUNC_FOR_QUATS(GfQuath, GfQuatf, 4),

            MAYA_USD_DIFF_FUNC_FOR_GENERIC_TYPE(bool),
            MAYA_USD_DIFF_FUNC_FOR_GENERIC_TYPE(SdfTimeCode),
            MAYA_USD_DIFF_FUNC_FOR_GENERIC_TYPE(std::string),
            MAYA_USD_DIFF_FUNC_FOR_GENERIC_TYPE(TfToken),
            MAYA_USD_DIFF_FUNC_FOR_GENERIC_TYPE(SdfAssetPath),
            MAYA_USD_DIFF_FUNC_FOR_GENERIC_TYPE(SdfSpecifier),

            // TODO: separate U,V vs combined UV diff.
            // TODO: diff accross different integer types, like int_8 to int16_t.

            { DiffKey(typeid(void), typeid(void)), diffEmpties } };

    return diffs;
};

DiffFunc getDiffFunction(const VtValue& modified, const VtValue& baseline)
{
    const DiffFuncMap& diffs = getDiffFuncs();
    const DiffKey      typeKey(modified.GetTypeid(), baseline.GetTypeid());
    const auto         func = diffs.find(typeKey);
    if (func == diffs.end())
        return diffIncomparables;
    return func->second;
}

} // namespace

DiffResult compareValues(const VtValue& modified, const VtValue& baseline)
{
    DiffFunc diff = getDiffFunction(modified, baseline);
    return diff(modified, baseline);
}

} // namespace MayaUsdUtils
