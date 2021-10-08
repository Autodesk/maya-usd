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

namespace MayaUsdUtils {

using UsdAttribute = PXR_NS::UsdAttribute;
using VtValue = PXR_NS::VtValue;
using UsdTimeCode = PXR_NS::UsdTimeCode;

DiffResult compareAttributes(const UsdAttribute& modified, const UsdAttribute& baseline)
{
    // We will not compare the set of point-in-times themselves but the overall result
    // of the animated values. This takes csare of tyring to match time-samples: we
    // instead only care that the output result are the same.
    //
    // Note that the UsdAttribute API to get value automatically interpolates values
    // where samples are missing when queried.
    std::vector<double> times;
    if (!UsdAttribute::GetUnionedTimeSamples({ modified, baseline }, &times))
        return DiffResult::Differ;

    // If there are no time samples at all in both attributes, we will compare the default values
    // instead.
    if (times.size() <= 0) {
        return compareAttributes(modified, baseline, UsdTimeCode::Default());
    }

    // The algorithm returns the common result if there is one. Stop as soon as we reach Differ.
    DiffResult overallResult = DiffResult::Same;
    for (const double time : times) {
        const DiffResult sampleResult = compareAttributes(modified, baseline, UsdTimeCode(time));
        if (sampleResult == DiffResult::Same) {
            continue;
        } else if (sampleResult == overallResult) {
            continue;
        } else if (overallResult == DiffResult::Same) {
            overallResult = sampleResult;
            continue;
        } else {
            overallResult = DiffResult::Differ;
            break;
        }
    }

    return overallResult;
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
