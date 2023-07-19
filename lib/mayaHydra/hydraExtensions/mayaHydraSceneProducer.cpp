//
// Copyright 2023 Autodesk, Inc. All rights reserved.
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

#include "mayaHydraSceneProducer.h"

#include <mayaHydraLib/delegates/sceneDelegate.h>
#include <mayaHydraLib/delegates/delegateRegistry.h>

#include <pxr/base/tf/envSetting.h>

PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_ENV_SETTING(MAYA_HYDRA_ENABLE_NATIVE_SCENE_INDEX, false,
    "Enable scene index for Maya native scene.");

bool enableMayaNativeSceneIndex() {
    static bool enable = TfGetEnvSetting(MAYA_HYDRA_ENABLE_NATIVE_SCENE_INDEX);
    return enable;
}

MayaHydraSceneProducer::MayaHydraSceneProducer(
    SdfPath& id,
    MayaHydraDelegate::InitData& initData,
    bool lightEnabled)
{
    if (enableMayaNativeSceneIndex())
    {
        //_sceneIndex = new MayaHydraSceneIndex();
    }
    else
    {
        auto delegateNames = MayaHydraDelegateRegistry::GetDelegateNames();
        auto creators = MayaHydraDelegateRegistry::GetDelegateCreators();
        TF_VERIFY(delegateNames.size() == creators.size());
        for (size_t i = 0, n = creators.size(); i < n; ++i) {
            const auto& creator = creators[i];
            if (creator == nullptr) {
                continue;
            }
            initData.name = delegateNames[i];
            initData.delegateID = id.AppendChild(
                TfToken(TfStringPrintf("_Delegate_%s_%lu_%p", delegateNames[i].GetText(), i, this)));
            initData.producer = this;
            auto newDelegate = creator(initData);
            if (newDelegate) {
                // Call SetLightsEnabled before the delegate is populated
                newDelegate->SetLightsEnabled(lightEnabled);
                _sceneDelegate
                    = std::dynamic_pointer_cast<MayaHydraSceneDelegate>(newDelegate);
                if (TF_VERIFY(
                    _sceneDelegate,
                    "Maya Hydra scene delegate not found, check mayaHydra plugin installation.")) {
                    _solidPrimsRootPaths.push_back(_sceneDelegate->GetLightedPrimsRootPath());
                }
                _delegates.emplace_back(std::move(newDelegate));
            }
        }
    }
}

MayaHydraSceneProducer::~MayaHydraSceneProducer()
{
    _delegates.clear();
}

void MayaHydraSceneProducer::HandleCompleteViewportScene(const MDataServerOperation::MViewportScene& scene, MFrameContext::DisplayStyle ds)
{
    if (enableMayaNativeSceneIndex())
    {
        //return _sceneIndex->HandleCompleteViewportScene(scene, ds);
    }
    else
    {
        return _sceneDelegate->HandleCompleteViewportScene(scene, ds);
    }
}

void MayaHydraSceneProducer::Populate()
{
    if (enableMayaNativeSceneIndex())
    {
        //return _sceneIndex->Populate();
    }
    else
    {
        for (auto& it : _delegates) {
            it->Populate();
        }
    }
}

void MayaHydraSceneProducer::PopulateSelectedPaths(
    const MSelectionList& mayaSelection,
    SdfPathVector& selectedSdfPaths,
    const HdSelectionSharedPtr& selection)
{
    if (enableMayaNativeSceneIndex())
    {
        //return _sceneIndex->PopulateSelectedPaths(
        //    mayaSelection,
        //    selectedSdfPaths,
        //    selection);
    }
    else
    {
        for (auto& it : _delegates) {
            it->PopulateSelectedPaths(mayaSelection, selectedSdfPaths, selection);
        }
    }
}

SdfPath MayaHydraSceneProducer::SetCameraViewport(const MDagPath& camPath, const GfVec4d& viewport)
{
    if (enableMayaNativeSceneIndex())
    {
        //return _sceneIndex->SetCameraViewport(camPath, viewport);
        return SdfPath();
    }
    else
    {
        return _sceneDelegate->SetCameraViewport(camPath, viewport);
    }
}

void MayaHydraSceneProducer::SetLightsEnabled(const bool enabled)
{
    if (enableMayaNativeSceneIndex())
    {
        //return _sceneIndex->SetLightsEnabled(enabled);
    }
    else
    {
        return _sceneDelegate->SetLightsEnabled(enabled);
    }
}

const MayaHydraParams& MayaHydraSceneProducer::GetParams() const
{
    if (enableMayaNativeSceneIndex())
    {
        //return _sceneIndex->GetParams();
        static MayaHydraParams params;
        return params;
    }
    else
    {
        return _sceneDelegate->GetParams();
    }
}

void MayaHydraSceneProducer::SetParams(const MayaHydraParams& params)
{
    if (enableMayaNativeSceneIndex())
    {
        //return _sceneIndex->SetParams(params);
    }
    else
    {
        for (auto& it : _delegates) {
            it->SetParams(params);
        }
    }
}

bool MayaHydraSceneProducer::AddPickHitToSelectionList(
    const HdxPickHit& hit,
    const MHWRender::MSelectionInfo& selectInfo,
    MSelectionList& selectionList,
    MPointArray& worldSpaceHitPts)
{
    if (enableMayaNativeSceneIndex())
    {
        //return _sceneIndex->AddPickHitToSelectionList(
        //    hit,
        //    selectInfo,
        //    selectionList,
        //    worldSpaceHitPts);
        return false;
    }
    else
    {
        return _sceneDelegate->AddPickHitToSelectionList(
            hit,
            selectInfo,
            selectionList,
            worldSpaceHitPts);
    }
}

HdRenderIndex& MayaHydraSceneProducer::GetRenderIndex()
{
    if (enableMayaNativeSceneIndex())
    {
        //return _sceneIndex->GetRenderIndex();
        auto index = HdRenderIndex::New(nullptr, {});
        return *index;
    }
    else
    {
        return _sceneDelegate->GetRenderIndex();
    }
}

bool MayaHydraSceneProducer::IsHdSt() const
{
    if (enableMayaNativeSceneIndex())
    {
        return true;
    }
    else
    {
        return _sceneDelegate->IsHdSt();
    }
}

bool MayaHydraSceneProducer::GetPlaybackRunning() const
{
    if (enableMayaNativeSceneIndex())
    {
        return false;
    }
    else
    {
        return _sceneDelegate->GetPlaybackRunning();
    }
}

SdfPath MayaHydraSceneProducer::GetPrimPath(const MDagPath& dg, bool isSprim)
{
    if (enableMayaNativeSceneIndex())
    {
        //return _sceneIndex->GetPrimPath(dg, isSprim);
        return SdfPath();
    }
    else
    {
        return _sceneDelegate->GetPrimPath(dg, isSprim);
    }
}

void MayaHydraSceneProducer::InsertRprim(
    const TfToken& typeId,
    const SdfPath& id,
    const SdfPath& instancerId)
{
    if (enableMayaNativeSceneIndex())
    {
        //return _sceneIndex->InsertRprim();
    }
    else
    {
        return _sceneDelegate->InsertRprim(typeId, id, instancerId);
    }
}

void MayaHydraSceneProducer::RemoveRprim(const SdfPath& id)
{
    if (enableMayaNativeSceneIndex())
    {
        //_sceneIndex->RemoveRprim(id);
    }
    else
    {
        _sceneDelegate->RemoveRprim(id);
    }
}

void MayaHydraSceneProducer::InsertSprim(
    const TfToken& typeId,
    const SdfPath& id,
    HdDirtyBits initialBits)
{
    if (enableMayaNativeSceneIndex())
    {
        //_sceneIndex->InsertSprim();
    }
    else
    {
        _sceneDelegate->InsertSprim(typeId, id, initialBits);
    }
}

void MayaHydraSceneProducer::RemoveSprim(const TfToken& typeId, const SdfPath& id)
{
    if (enableMayaNativeSceneIndex())
    {
        //_sceneIndex->RemoveSprim(typeId, id);
    }
    else
    {
        _sceneDelegate->RemoveSprim(typeId, id);
    }
}

void MayaHydraSceneProducer::AddArnoldLight(const MDagPath& dag)
{
    if (enableMayaNativeSceneIndex())
    {
        //_sceneIndex->AddArnoldLight(dag);
    }
    else
    {
        _sceneDelegate->AddArnoldLight(dag);
    }
}

void MayaHydraSceneProducer::RemoveArnoldLight(const MDagPath& dag)
{
    if (enableMayaNativeSceneIndex())
    {
        //_sceneIndex->RemoveArnoldLight(dag);
    }
    else
    {
        _sceneDelegate->RemoveArnoldLight(dag);
    }
}

SdfPath MayaHydraSceneProducer::GetDelegateID(TfToken name)
{
    if (enableMayaNativeSceneIndex())
    {
        //return _sceneIndex->GetDelegateID(name);
        return SdfPath();
    }
    else
    {
        for (auto& delegate : _delegates) {
            if (delegate->GetName() == name) {
                return delegate->GetMayaDelegateID();
            }
        }
        return SdfPath();
    }
}

void MayaHydraSceneProducer::PreFrame(const MHWRender::MDrawContext& drawContext)
{
    if (enableMayaNativeSceneIndex())
    {
        //return _sceneIndex->PreFrame(drawContext);
    }
    else
    {
        for (auto& it : _delegates) {
            it->PreFrame(drawContext);
        }
    }
}

void MayaHydraSceneProducer::PostFrame()
{
    if (enableMayaNativeSceneIndex())
    {
        //return _sceneIndex->PostFrame();
    }
    else
    {
        for (auto& it : _delegates) {
            it->PostFrame();
        }
    }
}

void MayaHydraSceneProducer::RemoveAdapter(const SdfPath& id)
{
    if (enableMayaNativeSceneIndex())
    {
        //return _sceneIndex->RemoveAdapter();
    }
    else
    {
        return _sceneDelegate->RemoveAdapter(id);
    }
}

void MayaHydraSceneProducer::RecreateAdapterOnIdle(const SdfPath& id, const MObject& obj)
{
    if (enableMayaNativeSceneIndex())
    {
        //return _sceneIndex->RecreateAdapterOnIdle(id, obj);
    }
    else
    {
        return _sceneDelegate->RecreateAdapterOnIdle(id, obj);
    }
}

SdfPath MayaHydraSceneProducer::GetLightedPrimsRootPath() const
{
    if (enableMayaNativeSceneIndex())
    {
        //return _sceneIndex->GetDelegateID(name);
        return SdfPath();
    }
    else
    {
        return _sceneDelegate->GetLightedPrimsRootPath();
    }
}

void MayaHydraSceneProducer::MaterialTagChanged(const SdfPath& id)
{
    if (enableMayaNativeSceneIndex())
    {
        //return _sceneIndex->MaterialTagChanged(name);
    }
    else
    {
        _sceneDelegate->MaterialTagChanged(id);
    }
}

GfInterval MayaHydraSceneProducer::GetCurrentTimeSamplingInterval() const
{
    if (enableMayaNativeSceneIndex())
    {
        //return _sceneIndex->GetCurrentTimeSamplingInterval(name);
        return GfInterval();
    }
    else
    {
        return _sceneDelegate->GetCurrentTimeSamplingInterval();
    }
}

PXR_NAMESPACE_CLOSE_SCOPE
