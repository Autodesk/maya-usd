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
#include <pxr/usd/sdf/valueTypeName.h>

namespace MayaUsdUtils {

using UsdAttribute = PXR_NS::UsdAttribute;
using VtValue = PXR_NS::VtValue;
using UsdTimeCode = PXR_NS::UsdTimeCode;

DiffResult compareAttributes(const UsdAttribute& modified, const UsdAttribute& baseline)
{
    // TODO: compare the set of time-code for the time samples, iterate over all time codes.
    return compareAttributes(modified, baseline, UsdTimeCode::EarliestTime());
}

DiffResult compareAttributes(
    const UsdAttribute& modified,
    const UsdAttribute& baseline,
    const UsdTimeCode&  timeCode)
{
    VtValue    modifiedValue;
    const bool hasmodifiedValue = modified.Get(&modifiedValue, timeCode);

    VtValue    baselineValue;
    const bool hasBaselineValue = baseline.Get(&baselineValue, timeCode);

    // Check absence.
    if (!hasmodifiedValue)
        return hasBaselineValue ? DiffResult::Absent : DiffResult::Same;

    // Check creation.
    if (!hasBaselineValue)
        return DiffResult::Created;

    return compareValues(modifiedValue, baselineValue);
}

} // namespace MayaUsdUtils
