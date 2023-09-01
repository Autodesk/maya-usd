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

#include "mayaHydraDefaultLightDataSource.h"

#include <mayaHydraLib/sceneIndex/mayaHydraSceneIndex.h>

#include <pxr/imaging/hd/lightSchema.h>
#include <pxr/imaging/hd/retainedDataSource.h>
#include <pxr/imaging/hd/xformSchema.h>

PXR_NAMESPACE_OPEN_SCOPE

// ----------------------------------------------------------------------------

class MayaHydraSimpleLightDataSource : public HdContainerDataSource
{
public:
    HD_DECLARE_DATASOURCE(MayaHydraSimpleLightDataSource);

    MayaHydraSimpleLightDataSource(
        const SdfPath& id,
        MayaHydraSceneIndex* sceneIndex)
        : _id(id)
        , _sceneIndex(sceneIndex)
    {
    }

    TfTokenVector GetNames() override
    {
        TfTokenVector result = {
            HdTokens->filters,
            HdTokens->lightLink,
            HdTokens->shadowLink,
            HdTokens->lightFilterLink,
            HdTokens->isLight
        };
        return result;
    }

    HdDataSourceBaseHandle Get(const TfToken& name) override
    {
        VtValue v;
        if (name == HdLightTokens->params) {
            v = VtValue(_sceneIndex->GetDefaultLight());
        }
        else if (name == HdTokens->transform) {
            v = VtValue(GfMatrix4d(
                1.0)); // We don't use the transform but use the position param of the GlfsimpleLight
            // Hydra might crash when this is an empty VtValue.
        }
        else if (name == HdLightTokens->shadowCollection) {
            // Exclude lines/points primitives from casting shadows by only taking the primitives
            // whose root path belongs to LightedPrimsRootPath
            HdRprimCollection coll(HdTokens->geometry, HdReprSelector(HdReprTokens->refined));
            coll.SetRootPaths({ _sceneIndex->GetLightedPrimsRootPath() });
            v = VtValue(coll);
        }
        else if (name == HdLightTokens->shadowParams) {
            HdxShadowParams shadowParams;
            shadowParams.enabled = false;
            v = VtValue(shadowParams);
        }
        else
        {
            v = _GetLightParamValue(name);
        }

        // Wrap as data source 
        if (name == HdLightTokens->params) {
            return HdRetainedSampledDataSource::New(v);
        }
        else if (name == HdLightTokens->shadowParams) {
            return HdRetainedSampledDataSource::New(v);
        }
        else if (name == HdLightTokens->shadowCollection) {
            return HdRetainedSampledDataSource::New(v);
        }
        else {
            return HdCreateTypedRetainedDataSource(v);
        }
    }

    VtValue _GetLightParamValue(const TfToken& paramName)
    {
        if (paramName == HdLightTokens->color || paramName == HdTokens->displayColor) {
            const auto diffuse = _sceneIndex->GetDefaultLight().GetDiffuse();
            return VtValue(GfVec3f(diffuse[0], diffuse[1], diffuse[2]));
        }
        else if (paramName == HdLightTokens->intensity) {
            return VtValue(1.0f);
        }
        else if (paramName == HdLightTokens->diffuse) {
            return VtValue(1.0f);
        }
        else if (paramName == HdLightTokens->specular) {
            return VtValue(0.0f);
        }
        else if (paramName == HdLightTokens->exposure) {
            return VtValue(0.0f);
        }
        else if (paramName == HdLightTokens->normalize) {
            return VtValue(true);
        }
        else if (paramName == HdLightTokens->angle) {
            return VtValue(0.0f);
        }
        else if (paramName == HdLightTokens->shadowEnable) {
            return VtValue(false);
        }
        else if (paramName == HdLightTokens->shadowColor) {
            return VtValue(GfVec3f(0.0f, 0.0f, 0.0f));
        }
        else if (paramName == HdLightTokens->enableColorTemperature) {
            return VtValue(false);
        }
        return {};
    }

private:
    SdfPath _id;
    MayaHydraSceneIndex* _sceneIndex = nullptr;
};

// ----------------------------------------------------------------------------

MayaHydraDefaultLightDataSource::MayaHydraDefaultLightDataSource(
    const SdfPath& id,
    TfToken type,
    MayaHydraSceneIndex* sceneIndex)
    : _id(id)
    , _type(type)
    , _sceneIndex(sceneIndex)
{
}


TfTokenVector
MayaHydraDefaultLightDataSource::GetNames()
{
    TfTokenVector result = {
        HdXformSchemaTokens->xform,
        HdLightSchemaTokens->light,
    };
    return result;
}

HdDataSourceBaseHandle
MayaHydraDefaultLightDataSource::Get(const TfToken& name)
{
    if (name == HdLightSchemaTokens->light) {
         return MayaHydraSimpleLightDataSource::New(_id, _sceneIndex);
    }
    else if (name == HdXformSchemaTokens->xform) {
        auto xform = GfMatrix4d(1.0);
        return HdXformSchema::Builder()
            .SetMatrix(
                HdRetainedTypedSampledDataSource<GfMatrix4d>::New(xform))
            .Build();
    }
    return nullptr;
}

PXR_NAMESPACE_CLOSE_SCOPE
