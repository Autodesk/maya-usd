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

using UsdRelationship = PXR_NS::UsdRelationship;
using TfToken = PXR_NS::TfToken;
using SdfPath = PXR_NS::SdfPath;

namespace {

SdfPathVector GetRelTargets(const UsdRelationship& rel)
{
    SdfPathVector targets;
    // Note: we can receive invalid relationship when comparing all relationships of prims.
    // Note: we don't check the return value because an empty relationship returns false.
    if (rel.IsValid())
        rel.GetTargets(&targets);
    return targets;
}

} // namespace

DiffResultPerPath
compareRelationships(const UsdRelationship& modified, const UsdRelationship& baseline)
{
    DiffResultPerPath results;

    // Note: doing comparisons of two lists would require a general diff algorithm to find the
    // minimal diff. On top of that, the lists in USD are composed and are built from prepend,
    // append, remove operations on top of lower-level compositions. We will assume that changes
    // will mostly be additions at the beginning or end of the list, with a central unchanged core.

    // TODO: generalize this to all USD list-op types.

    // Get lists of targets.
    const SdfPathVector baselineTargets = GetRelTargets(baseline);
    const SdfPathVector modifiedTargets = GetRelTargets(modified);

    // Target lists won't change from now on, so cache the ends.
    const auto baselineEnd = baselineTargets.end();
    const auto modifiedEnd = modifiedTargets.end();

    auto baselineIter = baselineTargets.begin();
    auto modifiedIter = modifiedTargets.begin();

    // Find all initial modified targets that don't match the baseline first target.
    // These will correspond to prepend.
    // TODO: improve by finding the first baseline target that matches, mark previous ones absent?
    for (; modifiedIter != modifiedEnd; ++modifiedIter) {
        if (baselineIter == baselineEnd) {
            results[*modifiedIter] = DiffResult::Prepended;
            continue;
        } else if (*baselineIter == *modifiedIter) {
            break;
        } else {
            results[*modifiedIter] = DiffResult::Prepended;
        }
    }

    // Find matching middle part.
    for (; modifiedIter != modifiedEnd; ++modifiedIter) {
        if (baselineIter == baselineEnd) {
            break;
        } else if (*baselineIter == *modifiedIter) {
            results[*modifiedIter] = DiffResult::Same;
            ++baselineIter;
        } else {
            break;
        }
    }

    // Add all remaining baseline targets that didn't match the modified targets.
    for (; baselineIter != baselineEnd; ++baselineIter) {
        results[*baselineIter] = DiffResult::Absent;
    }

    // Add all final modified targets that didn't match the baseline.
    // These will correspond to append.
    for (; modifiedIter != modifiedEnd; ++modifiedIter) {
        results[*modifiedIter] = DiffResult::Appended;
    }

    return results;
}

} // namespace MayaUsdUtils
