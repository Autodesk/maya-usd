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
#include <pxr/usd/sdf/path.h>
#include <pxr/usd/usd/attribute.h>
#include <pxr/usd/usd/prim.h>
#include <pxr/usd/usd/relationship.h>
#include <pxr/usd/usd/timeCode.h>

#include <map>
#include <unordered_set>

namespace MayaUsdUtils {

//----------------------------------------------------------------------------------------------------------------------
/// The possible results from the comparison of single particular item (property, relationship,
/// etc).
enum class DiffResult
{
    Same,      // The item is identical to the baseline.
    Absent,    // The item no longer exist compared to the baseline.
    Created,   // The item does not exist in the baseline.
    Prepended, // The item is prepended to the baseline.
    Appended,  // The item is appended to the baseline.
    Subset,    // The item is a subset of the baseline item.
    Superset,  // The item is a superset of the baseline item.
    Differ     // The item differs from the baseline in a more complex way.
};

//----------------------------------------------------------------------------------------------------------------------
/// The set of differences for each token. For example:
///    - For each property that were compared between two prims.
///    - For each metadata that were compared between two objects.
using DiffResultPerToken = std::map<PXR_NS::TfToken, DiffResult>;

//----------------------------------------------------------------------------------------------------------------------
/// The set of differences for each path. For example:
///    - For each target path that were compared between two relationships.
using DiffResultPerPath = std::map<PXR_NS::SdfPath, DiffResult>;

//----------------------------------------------------------------------------------------------------------------------
/// The set of differences for each path for each token. For example:
///    - For each relationship that were compared between two prims.
using DiffResultPerPathPerToken = std::map<PXR_NS::TfToken, DiffResultPerPath>;

//----------------------------------------------------------------------------------------------------------------------
/// \brief  analyzes all the sub-results to compte an overall result.
/// \param  subResults the sub-results to analyze.
/// \return the overall result, all results are possible.
//----------------------------------------------------------------------------------------------------------------------
template <class MAP> MAYA_USD_UTILS_PUBLIC DiffResult computeOverallResult(const MAP& subResults);

//----------------------------------------------------------------------------------------------------------------------
/// \brief  compares all the attributes of a modified prim to a baseline one.
/// \param  modified the potentially modified prim that is compared.
/// \param  baseline the prim that is used as the baseline for the comparison.
/// \return the map of attribute names to the result of comparison of that attribute.
/// Currently Subset and Superset are never returned.
//----------------------------------------------------------------------------------------------------------------------
MAYA_USD_UTILS_PUBLIC
DiffResultPerToken
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
/// \brief  compares all the relationships of a modified prim to a baseline one.
/// \param  modified the potentially modified prim that is compared.
/// \param  baseline the prim that is used as the baseline for the comparison.
/// \return the map of relationship names to the result of comparison of that relationship.
/// Currently only Same, Absent, Prepended or Appended are returned.
//----------------------------------------------------------------------------------------------------------------------
MAYA_USD_UTILS_PUBLIC
DiffResultPerPathPerToken
comparePrimsRelationships(const PXR_NS::UsdPrim& modified, const PXR_NS::UsdPrim& baseline);

//----------------------------------------------------------------------------------------------------------------------
/// \brief  compares all the targets of a modified relationship to a baseline one.
/// \param  modified the potentially modified relationship that is compared.
/// \param  baseline the relationship that is used as the baseline for the comparison.
/// \return the map of target paths to the result of comparison of that target.
/// Currently only Same, Absent, Prepended or Appended are returned.
//----------------------------------------------------------------------------------------------------------------------
MAYA_USD_UTILS_PUBLIC
DiffResultPerPath compareRelationships(
    const PXR_NS::UsdRelationship& modified,
    const PXR_NS::UsdRelationship& baseline);

//----------------------------------------------------------------------------------------------------------------------
/// \brief  compares all the metadatas of a modified object to a baseline one.
/// \param  modified the potentially modified object that is compared.
/// \param  baseline the object that is used as the baseline for the comparison.
/// \return the map of metadata names to the result of comparison of that metadata.
/// Currently Subset and Superset are never returned.
//----------------------------------------------------------------------------------------------------------------------
MAYA_USD_UTILS_PUBLIC
DiffResultPerToken
compareObjectsMetadatas(const PXR_NS::UsdObject& modified, const PXR_NS::UsdObject& baseline);

//----------------------------------------------------------------------------------------------------------------------
/// \brief  retrieves the list of metadata ignored during comparisons.
/// \return the setp of metadata names that should be ignored.
//----------------------------------------------------------------------------------------------------------------------
MAYA_USD_UTILS_PUBLIC
std::unordered_set<PXR_NS::TfToken, PXR_NS::TfToken::HashFunctor>& getIgnoredMetadatas();

//----------------------------------------------------------------------------------------------------------------------
/// \brief  compares a modified value to a baseline value.
/// \param  modified the potentially modified value that is compared.
/// \param  baseline the value that is used as the baseline for the comparison.
/// \return the result of the comparison of that modified value.
/// Currently Subset and Superset are never returned.
//----------------------------------------------------------------------------------------------------------------------
MAYA_USD_UTILS_PUBLIC
DiffResult compareValues(const PXR_NS::VtValue& modified, const PXR_NS::VtValue& baseline);

//----------------------------------------------------------------------------------------------------------------------
template <class MAP> inline DiffResult computeOverallResult(const MAP& subResults)
{
    // Single pass over items to find what type of sub-results we have.
    bool hasAbsent = false;
    bool hasCreated = false;
    bool hasPrepended = false;
    bool hasAppended = false;
    bool hasSame = false;

    for (const auto& keyAndResult : subResults) {
        switch (keyAndResult.second) {
        case DiffResult::Same: hasSame = true; break;
        case DiffResult::Absent: hasAbsent = true; break;
        case DiffResult::Created: hasCreated = true; break;
        case DiffResult::Prepended: hasPrepended = true; break;
        case DiffResult::Appended: hasAppended = true; break;

        // As soon as we find a Differ result, we can return.
        // Note: superset and subset at a lower-level is not superset or subset at a higher level.
        case DiffResult::Subset: return DiffResult::Differ;
        case DiffResult::Superset: return DiffResult::Differ;
        case DiffResult::Differ: return DiffResult::Differ;
        }
    }

    // Analyze combination of results.
    //
    //     - All were same: overall is same.
    //     - No absent, all same or prepended: overall is prepended.
    //     - No absent, all same or appended: overall is appended.
    //     - No absent, no same: overall is created.
    //     - No absent, some same: overall is superset.
    //     - Some absent, some created, appended or prepended: differ.
    //     - All absent or same: overall is subset.
    //     - All absent, no same: overall is absent.

    if (!hasAbsent) {
        if (!hasCreated && !hasPrepended && !hasAppended)
            return DiffResult::Same;

        if (!hasSame)
            return DiffResult::Created;

        if (!hasCreated && hasPrepended && !hasAppended)
            return DiffResult::Prepended;

        if (!hasCreated && !hasPrepended && hasAppended)
            return DiffResult::Appended;

        return DiffResult::Superset;
    } else {
        if (hasCreated || hasPrepended || hasAppended)
            return DiffResult::Differ;

        if (hasSame)
            return DiffResult::Subset;

        return DiffResult::Absent;
    }
}

} // namespace MayaUsdUtils
