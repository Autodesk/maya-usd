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

#include "DiffCore.h"

namespace MayaUsdUtils {

using UsdPrim = PXR_NS::UsdPrim;
using TfToken = PXR_NS::TfToken;

DiffResultPerToken comparePrimsAttributes(const UsdPrim& modified, const UsdPrim& baseline)
{
    DiffResultPerToken results;

    // Create a map of baseline attribute indexed by name to rapidly verify
    // if it exists and be able to compare attributes.
    std::map<TfToken, UsdAttribute> baselineAttrs;
    {
        for (const UsdAttribute& attr : baseline.GetAuthoredAttributes()) {
            baselineAttrs[attr.GetName()] = attr;
        }
    }

    // Compare the attributes from the new prim.
    // Baseline attributes map won't change from now on, so cache the end.
    {
        const auto baselineEnd = baselineAttrs.end();
        for (const UsdAttribute& attr : modified.GetAuthoredAttributes()) {
            const TfToken& name = attr.GetName();
            const auto     iter = baselineAttrs.find(name);
            if (iter == baselineEnd) {
                results[name] = DiffResult::Created;
            } else {
                results[name] = compareAttributes(attr, iter->second);
            }
        }
    }

    // Identify attributes that are absent in the new prim.
    for (const auto& nameAndAttr : baselineAttrs) {
        const auto& name = nameAndAttr.first;
        if (results.find(name) == results.end()) {
            results[name] = DiffResult::Absent;
        }
    }

    return results;
}

DiffResultPerPathPerToken
comparePrimsRelationships(const PXR_NS::UsdPrim& modified, const PXR_NS::UsdPrim& baseline)
{
    DiffResultPerPathPerToken results;

    // Create a map of baseline relationship indexed by name to rapidly verify
    // if it exists and be able to compare relationships.
    std::map<TfToken, UsdRelationship> baselineRels;
    {
        for (const UsdRelationship& rel : baseline.GetAuthoredRelationships()) {
            baselineRels[rel.GetName()] = rel;
        }
    }

    // Compare the relationships from the new prim.
    // Baseline relationships map won't change from now on, so cache the end.
    {
        const auto baselineEnd = baselineRels.end();
        for (const UsdRelationship& rel : modified.GetAuthoredRelationships()) {
            const TfToken& name = rel.GetName();
            const auto     iter = baselineRels.find(name);
            if (iter == baselineEnd) {
                results[name] = compareRelationships(rel, UsdRelationship());
            } else {
                results[name] = compareRelationships(rel, iter->second);
            }
        }
    }

    // Identify relationships that are absent in the new prim.
    for (const auto& nameAndRel : baselineRels) {
        const auto& name = nameAndRel.first;
        if (results.find(name) == results.end()) {
            results[name] = compareRelationships(UsdRelationship(), nameAndRel.second);
        }
    }

    return results;
}


} // namespace MayaUsdUtils
