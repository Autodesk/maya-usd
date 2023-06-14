//
// Copyright 2023 Autodesk
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
#ifndef PXRUSDMAYA_ENHANCED_SPARSE_VALUE_WRITER_H
#define PXRUSDMAYA_ENHANCED_SPARSE_VALUE_WRITER_H

#include <mayaUsd/base/api.h>

#include <pxr/base/vt/value.h>
#include <pxr/pxr.h>
#include <pxr/usd/usd/attribute.h>
#include <pxr/usd/usd/timeCode.h>
#include <pxr/usd/usdUtils/sparseValueWriter.h>

PXR_NAMESPACE_OPEN_SCOPE

/// Enhanced spare value writer.
///
/// The enhancement is that we can force to write default values at the default time.
/// This is necessary in some cases, for example to author a layer that will override
/// a value back to its default. Another example is during edit-as-Maya / merge-to-USD
/// where we need to author default values in case the original value was not the default.
class MAYAUSD_CORE_PUBLIC EnhancedSparseValueWriter
{
public:
    /// Constructor taking a flag to decide if default values at default time should be written.
    EnhancedSparseValueWriter(bool writeDefaults = true);

    EnhancedSparseValueWriter(const EnhancedSparseValueWriter&) = delete;
    EnhancedSparseValueWriter& operator=(const EnhancedSparseValueWriter&) = delete;

    /// Sets the value of \p attr to \p value at time \p time. The value
    /// is written sparsely, i.e., the default value is authored only if
    /// it is different from the fallback value or the existing default value,
    /// and any redundant time-samples are skipped when the attribute value does
    /// not change significantly between consecutive time-samples.
    bool SetAttribute(
        const UsdAttribute& attr,
        const VtValue&      value,
        const UsdTimeCode   time = UsdTimeCode::Default());

    /// \overload
    /// For efficiency, this function swaps out the given \p value, leaving
    /// it empty. The value will be held in memory at least until the next
    /// time-sample is written or until the SparseAttrValueWriter instance is
    /// destroyed.
    bool SetAttribute(
        const UsdAttribute& attr,
        VtValue*            value,
        const UsdTimeCode   time = UsdTimeCode::Default());

    /// \overload
    template <typename T>
    bool SetAttribute(
        const UsdAttribute& attr,
        T&                  value,
        const UsdTimeCode   time = UsdTimeCode::Default())
    {
        VtValue val = VtValue::Take(value);
        return SetAttribute(attr, &val, time);
    }

    /// Clears the internal map, thereby releasing all the memory used by
    /// the sparse value-writers.
    void Clear() { _sparseWriter.Clear(); }

private:
    UsdUtilsSparseValueWriter _sparseWriter;
    bool                      _writeDefaults;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif /* PXRUSDMAYA_ENHANCED_SPARSE_VALUE_WRITER_H */
