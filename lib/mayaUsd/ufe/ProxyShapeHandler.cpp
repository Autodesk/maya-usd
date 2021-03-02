//
// Copyright 2019 Autodesk
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
#include "ProxyShapeHandler.h"

#include <mayaUsd/utils/query.h>

#include <maya/MGlobal.h>
#include <maya/MString.h>
#include <maya/MStringArray.h>

namespace MAYAUSD_NS_DEF {
namespace ufe {

//------------------------------------------------------------------------------
// Global variables
//------------------------------------------------------------------------------
const std::string ProxyShapeHandler::fMayaUsdGatewayNodeType = "mayaUsdProxyShapeBase";

//------------------------------------------------------------------------------
// ProxyShapeHandler
//------------------------------------------------------------------------------

/*static*/
const std::string& ProxyShapeHandler::gatewayNodeType() { return fMayaUsdGatewayNodeType; }

/*static*/
std::vector<std::string> ProxyShapeHandler::getAllNames()
{
    std::vector<std::string> names;
    MString                  cmd;
    MStringArray             result;
    cmd.format("ls -type ^1s -long", fMayaUsdGatewayNodeType.c_str());
    if (MS::kSuccess == MGlobal::executeCommand(cmd, result)) {
        names.reserve(result.length());
        for (MString& name : result) {
            names.push_back(name.asChar());
        }
    }
    return names;
}

/*static*/
PXR_NS::UsdStageWeakPtr ProxyShapeHandler::dagPathToStage(const std::string& dagPath)
{
    auto prim = PXR_NS::UsdMayaQuery::GetPrim(dagPath);
    return prim ? prim.GetStage() : nullptr;
}

/*static*/
std::vector<PXR_NS::UsdStageRefPtr> ProxyShapeHandler::getAllStages()
{
    // According to Pixar, the following should work:
    //   return UsdMayaStageCache::Get().GetAllStages();
    // but after a file open of a scene with one or more Pixar proxy shapes,
    // returns an empty list.  To be investigated, PPT, 28-Feb-2019.

    // When using an unmodified AL plugin, the following line crashes
    // Maya, so it requires the AL proxy shape inheritance from
    // MayaUsdProxyShapeBase.  PPT, 12-Apr-2019.
    std::vector<PXR_NS::UsdStageRefPtr> stages;
    auto                        allNames = getAllNames();
    stages.reserve(allNames.size());
    for (const auto& name : allNames) {
        PXR_NS::UsdStageWeakPtr stage = dagPathToStage(name);
        if (stage) {
            stages.push_back(stage);
        }
    }
    return stages;
}

} // namespace ufe
} // namespace MAYAUSD_NS_DEF
