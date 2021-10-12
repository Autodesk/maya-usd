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

#include <pxr/base/tf/type.h>
#include <pxr/base/vt/array.h>
#include <pxr/usd/sdf/valueTypeName.h>

namespace MayaUsdUtils {

using UsdObject = PXR_NS::UsdObject;
using TfToken = PXR_NS::TfToken;
using UsdMetadataValueMap = PXR_NS::UsdMetadataValueMap;

DiffResultMap compareObjectsMetadatas(const PXR_NS::UsdObject& modified, const PXR_NS::UsdObject& baseline)
{
    DiffResultMap results;

    // Create a map of baseline metadata indexed by name to rapidly verify
    // if it exists and be able to compare metadatas.
    const UsdMetadataValueMap baselineMetadatas = baseline.GetAllAuthoredMetadata();

    // Compare the metadatas from the new object.
    // Baseline metadata map won't change from now on, so cache the end.
    {
        const auto baselineEnd = baselineMetadatas.end();
        for (const auto& nameAndValue : modified.GetAllAuthoredMetadata()) {
            const TfToken& name = nameAndValue.first;
            const auto     iter = baselineMetadatas.find(name);
            if (iter == baselineEnd) {
                results[name] = DiffResult::Created;
            } else {
                results[name] = compareValues(nameAndValue.second, iter->second);
            }
        }
    }

    // Identify metadatas that are absent in the new object.
    for (const auto& nameAndAttr : baselineMetadatas) {
        const auto& name = nameAndAttr.first;
        if (results.find(name) == results.end()) {
            results[name] = DiffResult::Absent;
        }
    }

    return results;
}

} // namespace MayaUsdUtils
