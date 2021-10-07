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
//
// TODO: currently, only double, float and their arrays are supported.

using DiffFunc = std::function<DiffResult(const VtValue& modified, const VtValue& baseline)>;
using DiffKey = std::pair<std::type_index, std::type_index>;
using DiffFuncMap = std::map<DiffKey, DiffFunc>;

template <class T1, class T2>
DiffResult diffTwoTypes(const VtValue& modified, const VtValue& baseline)
{
    const T1& v1 = modified.Get<T1>();
    const T2& v2 = baseline.Get<T2>();
    return compareArray(&v1, &v2, 1, 1) ? DiffResult::Same : DiffResult::Differ;
}

template <class T1, class T2>
DiffResult diffTwoArrayTypes(const VtValue& modified, const VtValue& baseline)
{
    const VtArray<T1>& v1 = modified.Get<VtArray<T1>>();
    const VtArray<T2>& v2 = baseline.Get<VtArray<T2>>();
    return compareArray(v1.cdata(), v2.cdata(), modified.GetArraySize(), baseline.GetArraySize())
        ? DiffResult::Same
        : DiffResult::Differ;
}

template <class T> DiffResult diffOneType(const VtValue& modified, const VtValue& baseline)
{
    return diffTwoTypes<T, T>(modified, baseline);
}

template <class T> DiffResult diffOneArrayType(const VtValue& modified, const VtValue& baseline)
{
    return diffTwoArrayTypes<T, T>(modified, baseline);
}

DiffResult diffIncomparables(const VtValue& /*modified*/, const VtValue& /*baseline*/)
{
    return DiffResult::Differ;
}

DiffResult diffEmpties(const VtValue& /*modified*/, const VtValue& /*baseline*/)
{
    return DiffResult::Same;
}

const DiffFuncMap& getDiffFuncs()
{
    // Initializing static data inside a function makes it automatically initialization-order safe.
    static DiffFuncMap diffs = {
        { DiffKey(typeid(void), typeid(void)), diffEmpties },

        { DiffKey(typeid(double), typeid(double)), diffOneType<double> },
        { DiffKey(typeid(VtArray<double>), typeid(VtArray<double>)), diffOneArrayType<double> },

        { DiffKey(typeid(float), typeid(float)), diffOneType<float> },
        { DiffKey(typeid(VtArray<float>), typeid(VtArray<float>)), diffOneArrayType<float> },

        { DiffKey(typeid(double), typeid(float)), diffTwoTypes<double, float> },
        { DiffKey(typeid(VtArray<double>), typeid(VtArray<float>)),
          diffTwoArrayTypes<double, float> },

        { DiffKey(typeid(float), typeid(double)), diffTwoTypes<float, double> },
        { DiffKey(typeid(VtArray<float>), typeid(VtArray<double>)),
          diffTwoArrayTypes<float, double> },
    };

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
