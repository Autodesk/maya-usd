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
#include <usdUfe/ufe/MaterialUtils.h>

#include <pxr/base/tf/pyContainerConversions.h>
#include <pxr/base/tf/pyResultConversions.h>
#include <pxr_python.h>

#include <ufe/hierarchy.h>
#include <ufe/pathString.h>

#include <string>
#include <tuple>
#include <vector>

PXR_NAMESPACE_USING_DIRECTIVE
using namespace PXR_BOOST_PYTHON_NAMESPACE;

using RendererMaterialTuple = std::tuple<std::string, std::string, std::string>;

std::vector<RendererMaterialTuple> _getMaterialsFromRenderers()
{
    std::vector<RendererMaterialTuple> result;
    for (const auto& entry : UsdUfe::getMaterialsFromRenderers()) {
        result.emplace_back(entry.first, entry.second.label, entry.second.item);
    }
    return result;
}

std::vector<std::string> _getMaterialsInStage(const std::string& ufePathString)
{
    return UsdUfe::getMaterialsInStage(Ufe::PathString::path(ufePathString));
}

bool _canAssignMaterialToNodeType(const std::string& ufePathString)
{
    if (ufePathString.empty()) {
        return UsdUfe::canAssignMaterialToNodeType(nullptr);
    }

    const auto path = Ufe::PathString::path(ufePathString);
    return UsdUfe::canAssignMaterialToNodeType(Ufe::Hierarchy::createItem(path));
}

void wrapMaterialUtils()
{
    static TfPyContainerConversions::to_tuple_mapping<RendererMaterialTuple>
        _rendererMaterialTupleMapping;
    to_python_converter<
        std::vector<RendererMaterialTuple>,
        TfPySequenceToPython<std::vector<RendererMaterialTuple>>>();

    def("getMaterialsFromRenderers", &_getMaterialsFromRenderers);
    def("getMaterialsInStage", &_getMaterialsInStage);
    def("canAssignMaterialToNodeType", &_canAssignMaterialToNodeType);
}
