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
#pragma once

#include <mayaUsdUtils/Api.h>

#include <pxr/base/tf/token.h>
#include <pxr/base/vt/value.h>
#include <pxr/usd/usd/attribute.h>
#include <pxr/usd/usd/prim.h>
#include <pxr/usd/usd/timeCode.h>

#include <map>

namespace MayaUsdUtils {

/// The possible results from the comparison of single particular property.
enum class DiffResult
{
    Same,     // The property is identical to the baseline.
    Absent,   // The property no longer exist compared to the baseline.
    Created,  // The property does not exist in the baseline.
    Subset,   // The property is a subset of the baseline property.
    Superset, // The property is a superset of the baseline property.
    Differ    // The property differs from the baseline in a more complex way.
};

/// The set of differences for each property that was compared between two prims.
using DiffPrimsResults = std::map<PXR_NS::TfToken, DiffResult>;

// TODO: add versions without timeCodes to compare animated values.

//----------------------------------------------------------------------------------------------------------------------
/// \brief  compares all the attributes of a modified prim to a baseline one.
/// \param  modified the potentially modified prim that is compared.
/// \param  baseline the prim that is used as the baseline for the comparison.
/// \param  timeCode the time code at which to compare the prims.
/// \return the map of attribute names to the result of comparison of that attribute.
/// Currently Subset and Superset are never returned.
//----------------------------------------------------------------------------------------------------------------------
MAYA_USD_UTILS_PUBLIC
DiffPrimsResults
comparePrimsAttributes(const PXR_NS::UsdPrim& modified, const PXR_NS::UsdPrim& baseline);

//----------------------------------------------------------------------------------------------------------------------
/// \brief  compares a modified attribute to a baseline one for all time samples.
/// \param  modified the potentially modified attribute that is compared.
/// \param  baseline the attribute that is used as the baseline for the comparison.
/// \return the result of the comparison of that modified attribute.
/// Currently Subset and Superset are never returned.
//----------------------------------------------------------------------------------------------------------------------
MAYA_USD_UTILS_PUBLIC
DiffResult
compareAttributes(const PXR_NS::UsdAttribute& modified, const PXR_NS::UsdAttribute& baseline);

//----------------------------------------------------------------------------------------------------------------------
/// \brief  compares a modified attribute to a baseline one at a given time code.
/// \param  modified the potentially modified attribute that is compared.
/// \param  baseline the attribute that is used as the baseline for the comparison.
/// \param  timeCode the time code at which to compare the attributes.
/// \return the result of the comparison of that modified attribute.
/// Currently Subset and Superset are never returned.
//----------------------------------------------------------------------------------------------------------------------
MAYA_USD_UTILS_PUBLIC
DiffResult compareAttributes(
    const PXR_NS::UsdAttribute& modified,
    const PXR_NS::UsdAttribute& baseline,
    const PXR_NS::UsdTimeCode&  timeCode);

//----------------------------------------------------------------------------------------------------------------------
/// \brief  compares a modified value to a baseline value.
/// \param  modified the potentially modified value that is compared.
/// \param  baseline the value that is used as the baseline for the comparison.
/// \return the result of the comparison of that modified value.
/// Currently Subset and Superset are never returned.
//----------------------------------------------------------------------------------------------------------------------
MAYA_USD_UTILS_PUBLIC
DiffResult compareValues(const PXR_NS::VtValue& modified, const PXR_NS::VtValue& baseline);

} // namespace MayaUsdUtils
