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
#include <pxr/base/vt/dictionary.h>
#include <pxr/base/vt/value.h>
#include <pxr/usd/sdf/path.h>
#include <pxr/usd/usd/attribute.h>
#include <pxr/usd/usd/prim.h>
#include <pxr/usd/usd/relationship.h>
#include <pxr/usd/usd/timeCode.h>

#include <map>
#include <unordered_set>
#include <vector>

namespace MayaUsdUtils {

//----------------------------------------------------------------------------------------------------------------------
// Comparison result types.
//----------------------------------------------------------------------------------------------------------------------

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
    Reordered, // The item has changed position in a list.
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
/// The set of differences for each item in a dictionary. For example:
///    - For each key that were compared between two dictionaries.
using DiffResultPerKey = std::map<PXR_NS::VtDictionary::key_type, DiffResult>;

//----------------------------------------------------------------------------------------------------------------------
/// The set of differences for each path for each token. For example:
///    - For each relationship that were compared between two prims.
using DiffResultPerPathPerToken = std::map<PXR_NS::TfToken, DiffResultPerPath>;

//----------------------------------------------------------------------------------------------------------------------
/// \brief  analyzes all the sub-results to compute an overall result.
/// \param  subResults the sub-results to analyze.
/// \return the overall result, all results are possible.
//----------------------------------------------------------------------------------------------------------------------
template <class MAP> MAYA_USD_UTILS_PUBLIC DiffResult computeOverallResult(const MAP& subResults);

//----------------------------------------------------------------------------------------------------------------------
// Comparison of prims.
//----------------------------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------------------------
/// \brief  compares a modified prim to a baseline one, including their children.
/// Currently compares attributes, relationships and children.
/// \param  modified the potentially modified prim that is compared.
/// \param  baseline the prim that is used as the baseline for the comparison.
/// \param  quickDiff if not null, returns a result other than Same when a difference is found.
/// \return the overall result, all results are possible.
//----------------------------------------------------------------------------------------------------------------------
MAYA_USD_UTILS_PUBLIC
DiffResult comparePrims(
    const PXR_NS::UsdPrim& modified,
    const PXR_NS::UsdPrim& baseline,
    DiffResult*            quickDiff = nullptr);

//----------------------------------------------------------------------------------------------------------------------
/// \brief  compares a modified prim to a baseline one but not their children.
/// Currently compares attributes, relationships and children.
/// \param  modified the potentially modified prim that is compared.
/// \param  baseline the prim that is used as the baseline for the comparison.
/// \param  quickDiff if not null, returns a result other than Same when a difference is found.
/// \return the overall result, all results are possible.
//----------------------------------------------------------------------------------------------------------------------
MAYA_USD_UTILS_PUBLIC
DiffResult comparePrimsOnly(
    const PXR_NS::UsdPrim& modified,
    const PXR_NS::UsdPrim& baseline,
    DiffResult*            quickDiff = nullptr);

//----------------------------------------------------------------------------------------------------------------------
/// \brief  compares all the children of a modified prim to a baseline one.
/// \param  modified the potentially modified prim that is compared.
/// \param  baseline the prim that is used as the baseline for the comparison.
/// \param  quickDiff if not null, returns a result other than Same when a difference is found.
/// \return the map of children paths to the result of comparison of that child.
//----------------------------------------------------------------------------------------------------------------------
MAYA_USD_UTILS_PUBLIC
DiffResultPerPath comparePrimsChildren(
    const PXR_NS::UsdPrim& modified,
    const PXR_NS::UsdPrim& baseline,
    DiffResult*            quickDiff = nullptr);

//----------------------------------------------------------------------------------------------------------------------
/// \brief  compares all the attributes of a modified prim to a baseline one.
/// \param  modified the potentially modified prim that is compared.
/// \param  baseline the prim that is used as the baseline for the comparison.
/// \param  quickDiff if not null, returns a result other than Same when a difference is found.
/// \return the map of attribute names to the result of comparison of that attribute.
/// Currently Subset and Superset are never returned.
//----------------------------------------------------------------------------------------------------------------------
MAYA_USD_UTILS_PUBLIC
DiffResultPerToken comparePrimsAttributes(
    const PXR_NS::UsdPrim& modified,
    const PXR_NS::UsdPrim& baseline,
    DiffResult*            quickDiff = nullptr);

//----------------------------------------------------------------------------------------------------------------------
/// \brief  compares all the relationships of a modified prim to a baseline one.
/// \param  modified the potentially modified prim that is compared.
/// \param  baseline the prim that is used as the baseline for the comparison.
/// \param  quickDiff if not null, returns a result other than Same when a difference is found.
/// \return the map of relationship names to the result of comparison of that relationship.
/// Currently only Same, Absent, Reordered, Prepended or Appended are returned.
//----------------------------------------------------------------------------------------------------------------------
MAYA_USD_UTILS_PUBLIC
DiffResultPerPathPerToken comparePrimsRelationships(
    const PXR_NS::UsdPrim& modified,
    const PXR_NS::UsdPrim& baseline,
    DiffResult*            quickDiff = nullptr);

//----------------------------------------------------------------------------------------------------------------------
// Comparison of USD building blocks: attributes, relationships, etc.
//----------------------------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------------------------
/// \brief  compares a modified attribute to a baseline one for all time samples.
/// \param  modified the potentially modified attribute that is compared.
/// \param  baseline the attribute that is used as the baseline for the comparison.
/// \param  quickDiff if not null, returns a result other than Same when a difference is found.
/// \return the result of the comparison of that modified attribute.
/// Currently Subset and Superset are never returned.
//----------------------------------------------------------------------------------------------------------------------
MAYA_USD_UTILS_PUBLIC
DiffResult compareAttributes(
    const PXR_NS::UsdAttribute& modified,
    const PXR_NS::UsdAttribute& baseline,
    DiffResult*                 quickDiff = nullptr);

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
/// \brief  compares all the targets of a modified relationship to a baseline one.
/// \param  modified the potentially modified relationship that is compared.
/// \param  baseline the relationship that is used as the baseline for the comparison.
/// \param  quickDiff if not null, returns a result other than Same when a difference is found.
/// \return the map of target paths to the result of comparison of that target.
/// Currently only Same, Absent, Prepended or Appended are returned.
//----------------------------------------------------------------------------------------------------------------------
MAYA_USD_UTILS_PUBLIC
DiffResultPerPath compareRelationships(
    const PXR_NS::UsdRelationship& modified,
    const PXR_NS::UsdRelationship& baseline,
    DiffResult*                    quickDiff = nullptr);

//----------------------------------------------------------------------------------------------------------------------
/// \brief  compares all the metadatas of a modified object to a baseline one.
/// \param  modified the potentially modified object that is compared.
/// \param  baseline the object that is used as the baseline for the comparison.
/// \param  quickDiff if not null, returns a result other than Same when a difference is found.
/// \return the map of metadata names to the result of comparison of that metadata.
/// Currently Subset and Superset are never returned.
//----------------------------------------------------------------------------------------------------------------------
MAYA_USD_UTILS_PUBLIC
DiffResultPerToken compareObjectsMetadatas(
    const PXR_NS::UsdObject& modified,
    const PXR_NS::UsdObject& baseline,
    DiffResult*              quickDiff = nullptr);

//----------------------------------------------------------------------------------------------------------------------
/// \brief  compares the given metadata of a modified object to a baseline one.
/// \param  modified the potentially modified object that is compared.
/// \param  baseline the object that is used as the baseline for the comparison.
/// \param  metadata the name of the metadata to compare.
/// \return the result of comparison of that metadata.
//----------------------------------------------------------------------------------------------------------------------
MAYA_USD_UTILS_PUBLIC
DiffResult compareMetadatas(
    const PXR_NS::UsdObject& modified,
    const PXR_NS::UsdObject& baseline,
    const PXR_NS::TfToken&   metadata);

//----------------------------------------------------------------------------------------------------------------------
/// \brief  retrieves the list of metadata ignored during comparisons.
/// These are the structural USD metadata that are not authored by the user.
/// For example, the fact that a prim is a "def" or an "over" or that an attribute can or cannot be
/// animated. \return the set of metadata names that should be ignored.
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
/// \brief  compares a modified list of items to a baseline list.
/// \param  modified the potentially modified list of items that is compared.
/// \param  baseline the list of items that is used as the baseline for the comparison.
/// \param  quickDiff if not null, returns a result other than Same when a difference is found.
/// \return the result of the comparison for each item in that modified list.
/// Currently only Same, Absent, Reordered, Prepended or Appended are returned.
///
/// Currently instantiated for the types used in list-op: int, unsigned int, int64_t, uint64_t,
/// TfToken, std::string, SdfPath, SdfReference and SdfPayload.
//----------------------------------------------------------------------------------------------------------------------
template <class ITEM>
MAYA_USD_UTILS_PUBLIC std::map<ITEM, DiffResult> compareLists(
    const std::vector<ITEM>& modified,
    const std::vector<ITEM>& baseline,
    DiffResult*              quickDiff = nullptr);

//----------------------------------------------------------------------------------------------------------------------
/// \brief  compares a modified dictionary of items to a baseline dictionary.
/// \param  modified the potentially modified dictionary of values that is compared.
/// \param  baseline the dictionary of values that is used as the baseline for the comparison.
/// \param  quickDiff if not null, returns a result other than Same when a difference is found.
/// \return the result of the comparison for each value in that modified dictionary.
/// Currently only Same, Differ, Absent and Created are returned.
//----------------------------------------------------------------------------------------------------------------------
MAYA_USD_UTILS_PUBLIC
DiffResultPerKey compareDictionaries(
    const PXR_NS::VtDictionary& modified,
    const PXR_NS::VtDictionary& baseline,
    DiffResult*                 quickDiff = nullptr);

//----------------------------------------------------------------------------------------------------------------------
// Implementation for overall result computation.
//----------------------------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------------------------
template <class MAP> inline DiffResult computeOverallResult(const MAP& subResults)
{
    // Single pass over items to find what type of sub-results we have.
    bool hasSame = false;
    bool hasAbsent = false;
    bool hasCreated = false;
    bool hasPrepended = false;
    bool hasAppended = false;
    bool hasReordered = false;

    for (const auto& keyAndResult : subResults) {
        switch (keyAndResult.second) {
        case DiffResult::Same: hasSame = true; break;
        case DiffResult::Absent: hasAbsent = true; break;
        case DiffResult::Created: hasCreated = true; break;
        case DiffResult::Prepended: hasPrepended = true; break;
        case DiffResult::Appended: hasAppended = true; break;
        case DiffResult::Reordered: hasReordered = true; break;

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
    //     - All were same or reordered: overall is reordered.
    //
    //     - No absent, some created, appended or prepended and some reordered: overall differ.
    //     - No absent, no same: overall is created.
    //     - No absent, all same or prepended: overall is prepended.
    //     - No absent, all same or appended: overall is appended.
    //     - No absent, some same: overall is superset.
    //
    //     - Some absent, some created, appended or prepended: differ.
    //     - All absent or same or reordered: overall is subset.
    //     - All absent, no same: overall is absent.

    if (!hasAbsent) {
        if (!hasCreated && !hasPrepended && !hasAppended) {
            if (hasReordered) {
                return DiffResult::Reordered;
            } else {
                return DiffResult::Same;
            }
        }

        if (hasReordered)
            return DiffResult::Differ;

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

        if (hasSame || hasReordered)
            return DiffResult::Subset;

        return DiffResult::Absent;
    }
}

} // namespace MayaUsdUtils
