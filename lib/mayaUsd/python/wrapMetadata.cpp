//
// Copyright 2026 Autodesk
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

#include "pxr/base/tf/pyContainerConversions.h"
#include "pxr/base/tf/pyResultConversions.h"

#include <pxr/base/vt/value.h>
#include <pxr/pxr.h>
#include <pxr/usd/usd/prim.h>
#include <pxr/usd/usd/pyConversions.h>
#include <pxr_python.h>

#include <string>

using namespace PXR_BOOST_PYTHON_NAMESPACE;

PXR_NAMESPACE_USING_DIRECTIVE

namespace {

std::string convertValue(const VtValue& value)
{
    std::stringstream ss;
    ss << value;
    return ss.str();
}

UsdMetadataValueMap getAllPrimMetadataValuesAsText(PXR_NS::UsdPrim& prim)
{
    UsdMetadataValueMap metadataText;

    if (!prim)
        return metadataText;

    UsdMetadataValueMap values = prim.GetAllMetadata();
    for (const auto& nameAndValue : values) {
        metadataText[nameAndValue.first] = convertValue(nameAndValue.second);
    }

    return metadataText;
}

} // namespace

void wrapMetadata()
{
    def("getAllPrimMetadataValuesAsText",
        getAllPrimMetadataValuesAsText,
        return_value_policy<TfPyMapToDictionary>());
}
