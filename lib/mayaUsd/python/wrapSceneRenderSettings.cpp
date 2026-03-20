//
// Copyright 2025 Autodesk
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

#include <mayaUsd/nodes/sceneRenderSettings.h>

#include <pxr_python.h>

#include <pxr/usd/sdf/path.h>
#include <pxr/usd/usd/prim.h>

#include <maya/MFnDagNode.h>

namespace {

// Token for the render settings prim path stage metadata key.
static const PXR_NS::TfToken kRenderSettingsPrimPathToken("renderSettingsPrimPath");

// ---------------------------------------------------------------------------
// Singleton discovery / stage access
// ---------------------------------------------------------------------------

std::string SceneRenderSettings_find()
{
    MObject obj = MayaUsd::UsdSceneRenderSettings::findInstance();
    if (obj.isNull()) {
        return {};
    }
    MFnDagNode dagFn(obj);
    return dagFn.fullPathName().asChar();
}

PXR_NS::UsdStageRefPtr SceneRenderSettings_getUsdStage()
{
    return MayaUsd::UsdSceneRenderSettings::getStage();
}

PXR_NS::UsdPrim getDefaultRenderSettingsPrim()
{
    auto stage = MayaUsd::UsdSceneRenderSettings::getStage();
    if (!stage) {
        return {};
    }
    std::string path;
    stage->GetMetadata(kRenderSettingsPrimPathToken, &path);
    if (path.empty()) {
        return {};
    }
    return stage->GetPrimAtPath(PXR_NS::SdfPath(path));
}

} // namespace

using namespace PXR_BOOST_PYTHON_NAMESPACE;

void wrapSceneRenderSettings()
{
    class_<MayaUsd::UsdSceneRenderSettings, PXR_BOOST_PYTHON_NAMESPACE::noncopyable>(
        "SceneRenderSettings", no_init)

        // Discovery / stage access
        .def("find", &SceneRenderSettings_find,
             "Find the singleton node. Returns its Maya DAG path, or empty if not found.")
        .staticmethod("find")
        .def("getUsdStage", &SceneRenderSettings_getUsdStage,
             "Get the USD stage from the singleton node.")
        .staticmethod("getUsdStage")
        .def("getDefaultRenderSettingsPrim", &getDefaultRenderSettingsPrim,
             "Get the default render settings prim from the USD stage.")
        .staticmethod("getDefaultRenderSettingsPrim")
    ;
}
