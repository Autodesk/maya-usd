//
// Copyright 2019 Luma Pictures
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
#ifndef MAYAHYDRALIB_DELEGATE_H
#define MAYAHYDRALIB_DELEGATE_H

#include <mayaHydraLib/api.h>
#include <mayaHydraLib/delegates/params.h>

#include <pxr/base/arch/hints.h>
#include <pxr/base/gf/interval.h>
#include <pxr/imaging/hd/engine.h>
#include <pxr/imaging/hd/renderIndex.h>
#include <pxr/imaging/hd/rendererPlugin.h>
#include <pxr/imaging/hd/selection.h>
#include <pxr/imaging/hdx/pickTask.h>
#include <pxr/imaging/hdx/taskController.h>
#include <pxr/pxr.h>
#include <pxr/usd/sdf/path.h>

#include <maya/MAnimControl.h>
#include <maya/MDGContextGuard.h>
#include <maya/MDrawContext.h>
#include <maya/MPointArray.h>
#include <maya/MSelectionContext.h>
#include <maya/MSelectionList.h>

#include <memory>

PXR_NAMESPACE_OPEN_SCOPE

class MayaHydraSceneProducer;

/**
 * \brief MayaHydraDelegate is the base class for delegate classes.
 */
class MayaHydraDelegate
{
public:
    /// Structure passed to initialize this class
    struct InitData
    {
        inline InitData(
            TfToken            nameIn,
            HdEngine&          engineIn,
            HdRenderIndex*     renderIndexIn,
            HdRendererPlugin*  rendererPluginIn,
            HdxTaskController* taskControllerIn,
            const SdfPath&     delegateIDIn,
            bool               isHdStIn,
            MayaHydraSceneProducer* producerIn = nullptr)
            : name(nameIn)
            , engine(engineIn)
            , renderIndex(renderIndexIn)
            , rendererPlugin(rendererPluginIn)
            , taskController(taskControllerIn)
            , delegateID(delegateIDIn)
            , isHdSt(isHdStIn)
            , producer(producerIn)
        {
        }

        TfToken            name;
        HdEngine&          engine;
        HdRenderIndex*     renderIndex;
        HdRendererPlugin*  rendererPlugin;
        HdxTaskController* taskController;
        SdfPath            delegateID;
        bool               isHdSt;
        MayaHydraSceneProducer* producer;
    };

    MAYAHYDRALIB_API
    MayaHydraDelegate(const InitData& initData);
    MAYAHYDRALIB_API
    virtual ~MayaHydraDelegate() = default;

    virtual void Populate() = 0;
    virtual void PreFrame(const MHWRender::MDrawContext& context) { }
    virtual void PostFrame() { }

    MAYAHYDRALIB_API
    virtual void           SetParams(const MayaHydraParams& params);
    const MayaHydraParams& GetParams() const { return _params; }

    const SdfPath& GetMayaDelegateID() { return _mayaDelegateID; }
    TfToken        GetName() { return _name; }
    bool           IsHdSt() { return _isHdSt; }

    virtual void PopulateSelectedPaths(
        const MSelectionList&       mayaSelection,
        SdfPathVector&              selectedSdfPaths,
        const HdSelectionSharedPtr& selection)
    {
    }

    virtual bool AddPickHitToSelectionList(
        const HdxPickHit&                hit,
        const MHWRender::MSelectionInfo& selectInfo,
        MSelectionList&                  mayaSelection,
        MPointArray&                     worldSpaceHitPts)
    {
        return false;
    }

    void SetLightsEnabled(const bool enabled) { _lightsEnabled = enabled; }
    bool GetLightsEnabled() { return _lightsEnabled; }

    inline HdEngine&          GetEngine() { return _engine; }
    inline HdxTaskController* GetTaskController() { return _taskController; }

    /// Calls that mirror UsdImagingDelegate

    /// Setup for the shutter open and close to be used for motion sampling.
    MAYAHYDRALIB_API
    void SetCameraForSampling(SdfPath const& id);

    /// Returns the current interval that will be used when using the
    /// sample* API in the scene delegate.
    MAYAHYDRALIB_API
    GfInterval GetCurrentTimeSamplingInterval() const;

    /// Common function to return templated sample types
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

    MayaHydraSceneProducer* GetProducer() { return _producer; };

private:
    MayaHydraParams _params;

    // Note that because there may not be a 1-to-1 relationship between
    // a MayaHydraDelegate and a HdSceneDelegate, this may be different than
    // "the" scene delegate id.  In the case of MayaHydraSceneDelegate,
    // which inherits from HdSceneDelegate, they are the same; but for, ie,
    // MayaHydraALProxyDelegate, for which there are multiple HdSceneDelegates
    // for each MayaHydraDelegate, the _mayaDelegateID is different from each
    // HdSceneDelegate's id.
    const SdfPath      _mayaDelegateID;
    SdfPath            _cameraPathForSampling;
    TfToken            _name;
    HdEngine&          _engine;
    HdxTaskController* _taskController;
    bool               _isHdSt = false;
    bool               _lightsEnabled = true;

    MayaHydraSceneProducer* _producer = nullptr;
};

using MayaHydraDelegatePtr = std::shared_ptr<MayaHydraDelegate>;

PXR_NAMESPACE_CLOSE_SCOPE

#endif // MAYAHYDRALIB_DELEGATE_H
