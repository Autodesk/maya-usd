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

#ifndef MAYAHYDRASCENEPRODUCER_H
#define MAYAHYDRASCENEPRODUCER_H

#include <maya/MDagPath.h>
#include <maya/MFrameContext.h>
#include <maya/MObject.h>
#include <maya/MSelectionList.h>
#include <maya/MViewport2Renderer.h>

#include <mayaHydraLib/api.h>
#include <mayaHydraLib/delegates/params.h>
#include <mayaHydraLib/delegates/delegate.h>
#include <mayaHydraLib/sceneIndex/mayaHydraSceneIndex.h>

#include <pxr/usd/sdf/path.h>
#include <pxr/imaging/hdx/pickTask.h>
#include <pxr/imaging/hd/changeTracker.h>
#include <pxr/imaging/hd/selection.h>
#include <pxr/imaging/hd/driver.h>
#include <pxr/imaging/hd/engine.h>
#include <pxr/imaging/hd/renderIndex.h>
#include <pxr/imaging/hd/rendererPlugin.h>
#include <pxr/imaging/hdx/taskController.h>

PXR_NAMESPACE_OPEN_SCOPE

class MayaHydraSceneDelegate;
class MayaHydraAdapter;

/**
 * \brief MayaHydraSceneProducer is used to produce the hydra scene from Maya native scene.
 * Under the hood, the work is delegated to MayaHydraSceneIndex or MayaHydraSceneDelegate, depends on
 * if MAYA_HYDRA_ENABLE_NATIVE_SCENE_INDEX is enabled or not. 
 * Note that MayaHydraSceneDelegate could be depreciated in the future.
 */
class MAYAHYDRALIB_API MayaHydraSceneProducer
{
public:
    MayaHydraSceneProducer(
        SdfPath& id,
        MayaHydraDelegate::InitData& initData,
        bool lightEnabled);
    ~MayaHydraSceneProducer();

    // Propogate scene changes from Maya to Hydra
    void HandleCompleteViewportScene(const MDataServerOperation::MViewportScene& scene, MFrameContext::DisplayStyle ds);

    // Populate primitives from Maya
    void Populate();

    // Populate selected paths from Maya
    void PopulateSelectedPaths(
        const MSelectionList& mayaSelection,
        SdfPathVector& selectedSdfPaths,
        const HdSelectionSharedPtr& selection);

    // Add hydra pick points and items to Maya's selection list
    bool AddPickHitToSelectionList(
        const HdxPickHit& hit,
        const MHWRender::MSelectionInfo& selectInfo,
        MSelectionList& selectionList,
        MPointArray& worldSpaceHitPts);

    // Insert a Rprim to hydra scene
    void InsertRprim(
        MayaHydraAdapter* adapter,
        const TfToken& typeId,
        const SdfPath& id,
        const SdfPath& instancerId = {});

    // Remove a Rprim from hydra scene
    void RemoveRprim(const SdfPath& id);

    // Mark a Rprim in hydra scene as dirty
    void MarkRprimDirty(const SdfPath& id, HdDirtyBits dirtyBits);

    // Insert a Sprim to hydra scene
    void InsertSprim(
        MayaHydraAdapter* adapter,
        const TfToken& typeId, 
        const SdfPath& id, 
        HdDirtyBits initialBits);

    // Remove a Sprim from hydra scene
    void RemoveSprim(const TfToken& typeId, const SdfPath& id);

    // Mark a Sprim in hydra scene as dirty
    void MarkSprimDirty(const SdfPath& id, HdDirtyBits dirtyBits);

    // Operation that's performed on rendering a frame
    void PreFrame(const MHWRender::MDrawContext& drawContext);
    void PostFrame();

    const MayaHydraParams& GetParams() const;
    void SetParams(const MayaHydraParams& params);

    // Adapter operations
    void RemoveAdapter(const SdfPath& id);
    void RecreateAdapterOnIdle(const SdfPath& id, const MObject& obj);

    // Update viewport info to camera
    SdfPath SetCameraViewport(const MDagPath& camPath, const GfVec4d& viewport);

    // Enable or disable lighting
    void SetLightsEnabled(const bool enabled);

    void AddArnoldLight(const MDagPath& dag);
    void RemoveArnoldLight(const MDagPath& dag);

    bool IsHdSt() const;

    bool GetPlaybackRunning() const;

    SdfPath GetPrimPath(const MDagPath& dg, bool isSprim);

    HdRenderIndex& GetRenderIndex();

    SdfPath GetLightedPrimsRootPath() const;

    GfInterval GetCurrentTimeSamplingInterval() const;

    // Return the id of underlying delegate by name (MayaHydraSceneIndex or MayaHydraSceneDelegate)
    SdfPath GetDelegateID(TfToken name);
    SdfPathVector GetSolidPrimsRootPaths() { return _solidPrimsRootPaths; }

    void MaterialTagChanged(const SdfPath& id);

    // Common function to return templated sample types
    template <typename T, typename Getter>
    size_t SampleValues(size_t maxSampleCount, float* times, T* samples, Getter getValue)
    {
        if (ARCH_UNLIKELY(maxSampleCount == 0)) {
            return 0;
        }
        // Fast path 1 sample at current-frame
        if (maxSampleCount == 1
            || (!GetParams().motionSamplesEnabled() && GetParams().motionSampleStart == 0)) {
            times[0] = 0.0f;
            samples[0] = getValue();
            return 1;
        }

        const GfInterval shutter = GetCurrentTimeSamplingInterval();
        // Shutter for [-1, 1] (size 2) should have a step of 2 for 2 samples, and 1 for 3 samples
        // For sample size of 1 tStep is unused and we match USD and to provide t=shutterOpen
        // sample.
        const double tStep = maxSampleCount > 1 ? (shutter.GetSize() / (maxSampleCount - 1)) : 0;
        const MTime  mayaTime = MAnimControl::currentTime();
        size_t       nSamples = 0;
        double       relTime = shutter.GetMin();

        for (size_t i = 0; i < maxSampleCount; ++i) {
            T sample;
            {
                MDGContextGuard guard(mayaTime + relTime);
                sample = getValue();
            }
            // We compare the sample to the previous in order to reduce sample count on output.
            // Goal is to reduce the amount of samples/keyframes the Hydra delegate has to absorb.
            if (!nSamples || sample != samples[nSamples - 1]) {
                samples[nSamples] = std::move(sample);
                times[nSamples] = relTime;
                ++nSamples;
            }
            relTime += tStep;
        }
        return nSamples;
    }

private:
    SdfPathVector _solidPrimsRootPaths;

    //
    // Delegates, depends on if MAYA_HYDRA_ENABLE_NATIVE_SCENE_INDEX is enabled or not.
    // 
    // SceneDelegate
    std::shared_ptr<MayaHydraSceneDelegate> _sceneDelegate;
    std::vector<MayaHydraDelegatePtr> _delegates;
    // SceneIndex
    MayaHydraSceneIndexRefPtr _sceneIndex;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // MAYAHYDRASCENEPRODUCER_H
