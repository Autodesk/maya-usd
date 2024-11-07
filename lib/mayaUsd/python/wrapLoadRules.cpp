//
// Copyright 2023 Autodesk
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
#include <mayaUsd/utils/loadRules.h>
#include <mayaUsd/utils/util.h>

#include <pxr/base/tf/pyResultConversions.h>
#include <pxr_python.h>

using namespace PXR_BOOST_PYTHON_NAMESPACE;

namespace {

bool setLoadRules(const std::string& shapeName, bool loadAllPayloads)
{
    MObject shapeObj;
    UsdMayaUtil::GetMObjectByName(shapeName, shapeObj);
    return MayaUsd::setLoadRulesAttribute(shapeObj, loadAllPayloads);
}

bool isLoadingAll(const std::string& shapeName)
{
    MObject shapeObj;
    UsdMayaUtil::GetMObjectByName(shapeName, shapeObj);

    PXR_NS::UsdStageLoadRules rules;
    MStatus                   status = MayaUsd::getLoadRulesFromAttribute(shapeObj, rules);

    // Note: when there are no load rules set, we load all payloads.
    if (!status)
        return true;

    return rules.IsLoadedWithAllDescendants(PXR_NS::SdfPath("/"));
}

} // namespace

void wrapLoadRules()
{
    def("setLoadRulesAttribute", setLoadRules);
    def("isLoadingAllPaylaods", isLoadingAll);
}
