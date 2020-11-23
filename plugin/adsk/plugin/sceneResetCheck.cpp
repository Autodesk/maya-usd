//
// Copyright 2020 Autodesk
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
#include "sceneResetCheck.h"

#include "ProxyShape.h"

#include <pxr/pxr.h>
#include <pxr/usd/usd/common.h>
#include <pxr/usd/usd/stage.h>

#include <maya/MFnDependencyNode.h>
#include <maya/MGlobal.h>
#include <maya/MItDependencyNodes.h>
#include <maya/MMessage.h>
#include <maya/MSceneMessage.h>

PXR_NAMESPACE_USING_DIRECTIVE

namespace {
MCallbackId _beforeNewCheckCallbackId = 0;
MCallbackId _beforeOpenCheckCallbackId = 0;

const char* ignoreDirtyLayersConfirmScript = R"(
global proc string MayaUsdIgnoreDirtyLayersConfirm()
{
    return `confirmDialog -title "Discard USD Edits" 
        -message "Are you sure you want to exit this session?\n\nAll edits on your USD layer(s) will be discarded."
        -button "Yes" -button "No" -defaultButton "No" -icon "warning"
        -cancelButton "No" -dismissString "No"`;

}
MayaUsdIgnoreDirtyLayersConfirm();
)";

static void _OnMayaNewOrOpenSceneCheckCallback(bool* retCode, void*)
{
    *retCode = true;

    bool atLeastOneDirty = false;

    MFnDependencyNode  fn;
    MItDependencyNodes iter(MFn::kPluginDependNode);

    for (; !iter.isDone() && !atLeastOneDirty; iter.next()) {
        MObject mobj = iter.item();
        fn.setObject(mobj);
        if (MayaUsd::ProxyShape::typeId == fn.typeId()) {
            MayaUsdProxyShapeBase* pShape = static_cast<MayaUsdProxyShapeBase*>(fn.userNode());
            UsdStageRefPtr         stage = pShape ? pShape->getUsdStage() : nullptr;
            if (!stage) {
                continue;
            }

            std::string          temp;
            SdfLayerHandleVector allLayers = stage->GetLayerStack(true);
            for (auto layer : allLayers) {
                if (layer->IsDirty()) {
                    atLeastOneDirty = true;
                }
            }
        }
    }

    if (atLeastOneDirty) {
        MString allowExit = MGlobal::executeCommandStringResult(ignoreDirtyLayersConfirmScript);
        if (MString("No") == allowExit) {
            *retCode = false;
        }
    }
}

} // namespace

namespace MAYAUSD_NS_DEF {

void SceneResetCheck::registerSceneResetCheckCallback()
{
    if (_beforeNewCheckCallbackId == 0) {
        _beforeNewCheckCallbackId = MSceneMessage::addCheckCallback(
            MSceneMessage::kBeforeNewCheck, _OnMayaNewOrOpenSceneCheckCallback);
    }

    if (_beforeOpenCheckCallbackId == 0) {
        _beforeOpenCheckCallbackId = MSceneMessage::addCheckCallback(
            MSceneMessage::kBeforeOpenCheck, _OnMayaNewOrOpenSceneCheckCallback);
    }
}

void SceneResetCheck::deregisterSceneResetCheckCallback()
{
    if (_beforeNewCheckCallbackId != 0) {
        MMessage::removeCallback(_beforeNewCheckCallbackId);
        _beforeNewCheckCallbackId = 0;
    }

    if (_beforeOpenCheckCallbackId != 0) {
        MMessage::removeCallback(_beforeOpenCheckCallbackId);
        _beforeOpenCheckCallbackId = 0;
    }
}

} // namespace MAYAUSD_NS_DEF
