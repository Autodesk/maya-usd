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

#include "mayaHydraCameraDataSource.h"

#include <mayaHydraLib/adapters/cameraAdapter.h>
#include <mayaHydraLib/sceneIndex/mayaHydraPrimvarDataSource.h>

#include <pxr/imaging/hd/cameraSchema.h>
#include <pxr/imaging/hd/retainedDataSource.h>

PXR_NAMESPACE_OPEN_SCOPE

// ----------------------------------------------------------------------------

template <typename T>
class MayaHydraTypedCameraParamValueDataSource : public HdTypedSampledDataSource<T>
{
public:
    HD_DECLARE_DATASOURCE(MayaHydraTypedCameraParamValueDataSource<T>);

    MayaHydraTypedCameraParamValueDataSource(
        const SdfPath& id,
        const TfToken& key,
        MayaHydraCameraAdapter* adapter)
        : _id(id)
        , _key(key)
        , _adapter(adapter)
    {
    }

    bool GetContributingSampleTimesForInterval(
        HdSampledDataSource::Time startTime,
        HdSampledDataSource::Time endTime,
        std::vector<HdSampledDataSource::Time>* outSampleTimes)  override
    {
        return MayaHydraPrimvarValueDataSource::New(
            _key, _adapter)->GetContributingSampleTimesForInterval(
                startTime, endTime, outSampleTimes);
    }

    T GetTypedValue(HdSampledDataSource::Time shutterOffset) override
    {
        VtValue v;
        if (shutterOffset == 0.0f) {
            v = _adapter->GetCameraParamValue(_key);
        }
        else {
            v = MayaHydraPrimvarValueDataSource::New(
                _key, _adapter)->GetValue(shutterOffset);
        }

        if (v.IsHolding<T>()) {
            return v.UncheckedGet<T>();
        }

        return T();
    }

    VtValue GetValue(HdSampledDataSource::Time shutterOffset) override
    {
        if (shutterOffset == 0.0f) {
            return _adapter->GetCameraParamValue(_key);
        }

        return VtValue(GetTypedValue(shutterOffset));
    }

private:
    SdfPath _id;
    TfToken _key;
    MayaHydraCameraAdapter* _adapter;
};


class MayaHydraCameraParamValueDataSource : public HdSampledDataSource
{
public:
    HD_DECLARE_DATASOURCE(MayaHydraCameraParamValueDataSource);

    MayaHydraCameraParamValueDataSource(
        const SdfPath& id,
        const TfToken& key,
        MayaHydraCameraAdapter* adapter)
        : _id(id)
        , _key(key)
        , _adapter(adapter)
    {
    }

    bool GetContributingSampleTimesForInterval(
        HdSampledDataSource::Time startTime,
        HdSampledDataSource::Time endTime,
        std::vector<HdSampledDataSource::Time>* outSampleTimes)  override
    {
        return MayaHydraPrimvarValueDataSource::New(
            _key, _adapter)->GetContributingSampleTimesForInterval(
                startTime, endTime, outSampleTimes);
    }

    VtValue GetValue(HdSampledDataSource::Time shutterOffset) override
    {
        if (shutterOffset == 0.0f) {
            return _adapter->GetCameraParamValue(_key);
        }

        return MayaHydraPrimvarValueDataSource::New(
            _key, _adapter)->GetValue(shutterOffset);
    }

private:
    SdfPath _id;
    TfToken _key;
    MayaHydraCameraAdapter* _adapter;
};

// ----------------------------------------------------------------------------

MayaHydraCameraDataSource::MayaHydraCameraDataSource(
    const SdfPath& id,
    TfToken type,
    MayaHydraAdapter* adapter)
    : _id(id)
    , _type(type)
    , _adapter(adapter)
{
}


TfTokenVector
MayaHydraCameraDataSource::GetNames()
{
    TfTokenVector results;

    results.push_back(HdCameraSchemaTokens->projection);
    results.push_back(HdCameraSchemaTokens->horizontalAperture);
    results.push_back(HdCameraSchemaTokens->verticalAperture);
    results.push_back(HdCameraSchemaTokens->horizontalApertureOffset);
    results.push_back(HdCameraSchemaTokens->verticalApertureOffset);
    results.push_back(HdCameraSchemaTokens->focalLength);
    results.push_back(HdCameraSchemaTokens->clippingRange);
    results.push_back(HdCameraSchemaTokens->clippingPlanes);

    return results;

}

HdDataSourceBaseHandle
MayaHydraCameraDataSource::Get(const TfToken& name)
{
    MayaHydraCameraAdapter* camAdapter = dynamic_cast<MayaHydraCameraAdapter*>(_adapter);
    if (!camAdapter) {
        return nullptr;
    }

    if (name == HdCameraSchemaTokens->projection) {
        VtValue v = camAdapter->GetCameraParamValue(name);

        HdCamera::Projection proj = HdCamera::Perspective;
        if (v.IsHolding<HdCamera::Projection>()) {
            proj = v.UncheckedGet<HdCamera::Projection>();
        }
        return HdRetainedTypedSampledDataSource<TfToken>::New(
            proj == HdCamera::Perspective ?
            HdCameraSchemaTokens->perspective :
            HdCameraSchemaTokens->orthographic);
    }
    else if (name == HdCameraSchemaTokens->clippingRange) {
        VtValue v = camAdapter->GetCameraParamValue(name);

        GfRange1f range;
        if (v.IsHolding<GfRange1f>()) {
            range = v.UncheckedGet<GfRange1f>();
        }
        return HdRetainedTypedSampledDataSource<GfVec2f>::New(
            GfVec2f(range.GetMin(), range.GetMax()));
    }
    else if (name == HdCameraTokens->windowPolicy) {
        VtValue v = camAdapter->GetCameraParamValue(name);

        CameraUtilConformWindowPolicy wp = CameraUtilDontConform;
        if (v.IsHolding<CameraUtilConformWindowPolicy>()) {
            wp = v.UncheckedGet<CameraUtilConformWindowPolicy>();
        }
        return HdRetainedTypedSampledDataSource<
            CameraUtilConformWindowPolicy>::New(wp);
    }
    else if (name == HdCameraSchemaTokens->clippingPlanes) {
        const VtValue v = camAdapter->GetCameraParamValue(HdCameraTokens->clipPlanes);
        VtArray<GfVec4d> array;
        if (v.IsHolding<std::vector<GfVec4d>>()) {
            const std::vector<GfVec4d>& vec =
                v.UncheckedGet<std::vector<GfVec4d>>();
            array.resize(vec.size());
            for (size_t i = 0; i < vec.size(); i++) {
                array[i] = vec[i];
            }
        }
        return HdRetainedTypedSampledDataSource<VtArray<GfVec4d>>::New(
            array);
    }
    else if (std::find(HdCameraSchemaTokens->allTokens.begin(),
        HdCameraSchemaTokens->allTokens.end(), name)
        != HdCameraSchemaTokens->allTokens.end()) {
        // all remaining HdCameraSchema members are floats and should
        // be returned as a typed data source for schema conformance.
        return MayaHydraTypedCameraParamValueDataSource<float>::New(
            _id, name, camAdapter);
    }
    else {
        return MayaHydraCameraParamValueDataSource::New(
            _id, name, camAdapter);
    }
}

PXR_NAMESPACE_CLOSE_SCOPE
