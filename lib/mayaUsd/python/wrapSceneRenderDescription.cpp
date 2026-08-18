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

#include <mayaUsd/nodes/sceneRenderDescription.h>
#include <mayaUsd/nodes/usdSettingsNode.h>

#include <pxr_python.h>

using namespace PXR_BOOST_PYTHON_NAMESPACE;

void wrapSceneRenderDescription()
{
    class_<MayaUsd::UsdSettingsNode, PXR_BOOST_PYTHON_NAMESPACE::noncopyable>(
        "UsdDefaultRenderDescription", no_init)
        .def("find", &MayaUsd::SceneRenderDescription::find)
        .staticmethod("find")
        .def("getUsdStage", &MayaUsd::SceneRenderDescription::getUsdStage)
        .staticmethod("getUsdStage")
        .def(
            "getDefaultRenderSettingsPrim",
            &MayaUsd::SceneRenderDescription::getDefaultRenderSettingsPrim)
        .staticmethod("getDefaultRenderSettingsPrim")
        .def(
            "getActiveRenderDescriptionPath",
            &MayaUsd::SceneRenderDescription::getActiveRenderDescriptionPath)
        .staticmethod("getActiveRenderDescriptionPath")
        .def(
            "setActiveRenderDescriptionPath",
            &MayaUsd::SceneRenderDescription::setActiveRenderDescriptionPath)
        .staticmethod("setActiveRenderDescriptionPath")
        .def("getCurrentRenderer", &MayaUsd::SceneRenderDescription::getCurrentRenderer)
        .staticmethod("getCurrentRenderer")
        .def("setCurrentRenderer", &MayaUsd::SceneRenderDescription::setCurrentRenderer)
        .staticmethod("setCurrentRenderer")
        .def(
            "externalCameraAttrName",
            +[]() { return MayaUsd::SceneRenderDescription::externalCameraAttrName().GetString(); })
        .staticmethod("externalCameraAttrName")
        .def("setCamera", &MayaUsd::SceneRenderDescription::setRenderSettingsCamera)
        .staticmethod("setCamera");
}
