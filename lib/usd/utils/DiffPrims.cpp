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
#include "DiffPrims.h"

#include <map>

namespace MayaUsdUtils {

using UsdPrim = PXR_NS::UsdPrim;
using TfToken = PXR_NS::TfToken;
using SdfPath = PXR_NS::SdfPath;
using UsdAttribute = PXR_NS::UsdAttribute;
using UsdRelationship = PXR_NS::UsdRelationship;

#define USD_MAYA_RETURN_QUICK_RESULT(result, results)  \
    do {                                               \
        if (quickDiff && result != DiffResult::Same) { \
            *quickDiff = result;                       \
            return results;                            \
        }                                              \
    } while (false)

DiffResultPerToken
comparePrimsAttributes(const UsdPrim& modified, const UsdPrim& baseline, DiffResult* quickDiff)
{
    DiffResultPerToken results;

    if (quickDiff)
        *quickDiff = DiffResult::Same;

    // Create a map of baseline attribute indexed by name to rapidly verify
    // if it exists and be able to compare attributes.
    std::map<TfToken, UsdAttribute> baselineAttrs;
    {
        for (const UsdAttribute& attr : baseline.GetAuthoredAttributes()) {
            baselineAttrs[attr.GetName()] = attr;
        }
    }

    // Compare the attributes from the modified prim.
    // Baseline attributes map won't change from now on, so cache the end.
    {
        const auto baselineEnd = baselineAttrs.end();
        for (const UsdAttribute& attr : modified.GetAuthoredAttributes()) {
            const TfToken& name = attr.GetName();
            const auto     iter = baselineAttrs.find(name);
            if (iter == baselineEnd) {
                USD_MAYA_RETURN_QUICK_RESULT(DiffResult::Created, results);
                results[name] = DiffResult::Created;
            } else {
                const DiffResult result = compareAttributes(attr, iter->second, quickDiff);
                USD_MAYA_RETURN_QUICK_RESULT(result, results);
                results[name] = result;
            }
        }
    }

    // Identify attributes that are absent in the modified prim.
    for (const auto& nameAndAttr : baselineAttrs) {
        const auto& name = nameAndAttr.first;
        if (results.find(name) == results.end()) {
            USD_MAYA_RETURN_QUICK_RESULT(DiffResult::Absent, results);
            results[name] = DiffResult::Absent;
        }
    }

    return results;
}

DiffResultPerPathPerToken
comparePrimsRelationships(const UsdPrim& modified, const UsdPrim& baseline, DiffResult* quickDiff)
{
    DiffResultPerPathPerToken results;

    if (quickDiff)
        *quickDiff = DiffResult::Same;

    // Create a map of baseline relationship indexed by name to rapidly verify
    // if it exists and be able to compare relationships.
    std::map<TfToken, UsdRelationship> baselineRels;
    {
        for (const UsdRelationship& rel : baseline.GetAuthoredRelationships()) {
            baselineRels[rel.GetName()] = rel;
        }
    }

    // Compare the relationships from the modified prim.
    // Baseline relationships map won't change from now on, so cache the end.
    {
        const auto baselineEnd = baselineRels.end();
        for (const UsdRelationship& rel : modified.GetAuthoredRelationships()) {
            const TfToken& name = rel.GetName();
            const auto     iter = baselineRels.find(name);
            if (iter == baselineEnd) {
                results[name] = compareRelationships(rel, UsdRelationship(), quickDiff);
                USD_MAYA_RETURN_QUICK_RESULT(*quickDiff, results);
            } else {
                results[name] = compareRelationships(rel, iter->second, quickDiff);
                USD_MAYA_RETURN_QUICK_RESULT(*quickDiff, results);
            }
        }
    }

    // Identify relationships that are absent in the modified prim.
    for (const auto& nameAndRel : baselineRels) {
        const auto& name = nameAndRel.first;
        if (results.find(name) == results.end()) {
            results[name] = compareRelationships(UsdRelationship(), nameAndRel.second, quickDiff);
            USD_MAYA_RETURN_QUICK_RESULT(*quickDiff, results);
        }
    }

    return results;
}

DiffResultPerPath
comparePrimsChildren(const UsdPrim& modified, const UsdPrim& baseline, DiffResult* quickDiff)
{
    DiffResultPerPath results;

    if (quickDiff)
        *quickDiff = DiffResult::Same;

    // Create a map of baseline children indexed by name to rapidly verify
    // if it exists and be able to compare children.
    std::map<SdfPath, UsdPrim> baselineChildren;
    {
        for (const UsdPrim& child : baseline.GetAllChildren()) {
            baselineChildren[child.GetPath()] = child;
        }
    }

    // Compare the children from the modified prim.
    // Baseline children map won't change from now on, so cache the end.
    {
        const auto baselineEnd = baselineChildren.end();
        for (const UsdPrim& child : modified.GetAllChildren()) {
            const SdfPath& path = child.GetPath();
            const auto     iter = baselineChildren.find(path);
            if (iter == baselineEnd) {
                USD_MAYA_RETURN_QUICK_RESULT(DiffResult::Created, results);
                results[path] = DiffResult::Created;
            } else {
                results[path] = comparePrims(child, iter->second, quickDiff);
                USD_MAYA_RETURN_QUICK_RESULT(*quickDiff, results);
            }
        }
    }

    // Identify children that are absent in the modified prim.
    for (const auto& pathAndPrim : baselineChildren) {
        const auto& path = pathAndPrim.first;
        if (results.find(path) == results.end()) {
            USD_MAYA_RETURN_QUICK_RESULT(DiffResult::Absent, results);
            results[path] = DiffResult::Absent;
        }
    }

    return results;
}

static DiffResult comparePrims(
    const PXR_NS::UsdPrim& modified,
    const PXR_NS::UsdPrim& baseline,
    bool                   compareChildren,
    DiffResult*            quickDiff)
{
    if (quickDiff)
        *quickDiff = DiffResult::Same;

    // If either is invalid, just compare validity.
    if (!modified.IsValid() || !baseline.IsValid()) {
        const DiffResult result
            = (modified.IsValid() == baseline.IsValid()) ? DiffResult::Same : DiffResult::Differ;
        if (quickDiff)
            *quickDiff = result;
        return result;
    }

    // We need a map to passs to computeOverallResult(), so we create one indexed by some simple
    // arbitrary thing.
    std::map<int, DiffResult> subResults;
    int                       resultIndex = 0;

    // Note: we will short-cut to DifResult::Differ as soon as we detect one such result.

    {
        const auto attrDiffs = comparePrimsAttributes(modified, baseline, quickDiff);
        USD_MAYA_RETURN_QUICK_RESULT(*quickDiff, *quickDiff);

        // Note: no need to quick result when computing overall result as it would already have
        // returned.
        const DiffResult overall = computeOverallResult(attrDiffs);
        if (overall == DiffResult::Differ)
            return DiffResult::Differ;

        subResults[resultIndex++] = overall;
    }

    {
        const auto relDiffs = comparePrimsRelationships(modified, baseline, quickDiff);
        USD_MAYA_RETURN_QUICK_RESULT(*quickDiff, *quickDiff);

        // Note: no need to quick result when computing overall result as it would already have
        // returned.
        for (const auto& tokenAndResults : relDiffs) {
            const DiffResult overall = computeOverallResult(tokenAndResults.second);
            if (overall == DiffResult::Differ)
                return DiffResult::Differ;

            subResults[resultIndex++] = overall;
        }
    }

    if (compareChildren) {
        const auto childrenDiffs = comparePrimsChildren(modified, baseline, quickDiff);
        USD_MAYA_RETURN_QUICK_RESULT(*quickDiff, *quickDiff);

        // Note: no need to quick result when computing overall result as it would already have
        // returned.
        const DiffResult overall = computeOverallResult(childrenDiffs);
        if (overall == DiffResult::Differ)
            return DiffResult::Differ;
        subResults[resultIndex++] = overall;
    }

    return computeOverallResult(subResults);
}

DiffResult comparePrims(
    const PXR_NS::UsdPrim& modified,
    const PXR_NS::UsdPrim& baseline,
    DiffResult*            quickDiff)
{
    return comparePrims(modified, baseline, true, quickDiff);
}

DiffResult comparePrimsOnly(
    const PXR_NS::UsdPrim& modified,
    const PXR_NS::UsdPrim& baseline,
    DiffResult*            quickDiff)
{
    return comparePrims(modified, baseline, false, quickDiff);
}

} // namespace MayaUsdUtils
