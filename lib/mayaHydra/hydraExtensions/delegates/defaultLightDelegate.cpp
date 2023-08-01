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
#include "defaultLightDelegate.h"

#include <mayaHydraLib/delegates/delegateDebugCodes.h>

#include <pxr/base/gf/rotation.h>
#include <pxr/base/gf/transform.h>
#include <pxr/imaging/hd/light.h>
#include <pxr/imaging/hd/tokens.h>
#include <pxr/imaging/hdx/simpleLightTask.h>

PXR_NAMESPACE_OPEN_SCOPE

namespace {
bool AreLightsParamsWeUseDifferent(const GlfSimpleLight& light1, const GlfSimpleLight& light2)
{
    // We only update 3 parameters in the default light : position, diffuse and specular. We don't
    // use the primitive's transform.
    return (light1.GetPosition() != light2.GetPosition())
        || // Position (in which we actually store a direction, updated when rotating the view for
           // example)
        (light1.GetDiffuse() != light2.GetDiffuse())
        || (light1.GetSpecular() != light2.GetSpecular());
}
} // namespace

// clang-format off
TF_DEFINE_PRIVATE_TOKENS(
    _tokens,

    (DefaultMayaLight)
);
// clang-format on

// MtohDefaultLightDelegate is a separate Hydra custom scene delegate to handle the default lighting
// from Maya. We use another Hydra custom scene delegate to handle the other parts of the Maya
// scene. See sceneDelegate.h in the mayaHydraLib project. If you want to know how to add a custom
// scene index to this plug-in, then please see the registration.cpp file in the mayaHydraLib
// project.
MtohDefaultLightDelegate::MtohDefaultLightDelegate(const InitData& initData)
    : HdSceneDelegate(initData.renderIndex, initData.delegateID)
    , MayaHydraDelegate(initData)
    , _lightPath(initData.delegateID.AppendChild(_tokens->DefaultMayaLight))
{
}

MtohDefaultLightDelegate::~MtohDefaultLightDelegate() { RemovePrim(); }

void MtohDefaultLightDelegate::Populate()
{
    if (_isPopulated) {
        return;
    }
    if (!_isLightingOn) {
        return;
    }
    _isSupported = IsHdSt() ? GetRenderIndex().IsSprimTypeSupported(HdPrimTypeTokens->simpleLight)
                            : GetRenderIndex().IsSprimTypeSupported(HdPrimTypeTokens->distantLight);
    if (ARCH_UNLIKELY(!_isSupported)) {
        return;
    }
    if (IsHdSt()) {
        GetRenderIndex().InsertSprim(HdPrimTypeTokens->simpleLight, this, _lightPath);
    } else {
        GetRenderIndex().InsertSprim(HdPrimTypeTokens->distantLight, this, _lightPath);
    }
    GetRenderIndex().GetChangeTracker().SprimInserted(_lightPath, HdLight::AllDirty);
    _isPopulated = true;
}

void MtohDefaultLightDelegate::RemovePrim()
{
    if (!_isPopulated) {
        return;
    }
    if (ARCH_UNLIKELY(!_isSupported)) {
        return;
    }
    if (IsHdSt()) {
        GetRenderIndex().RemoveSprim(HdPrimTypeTokens->simpleLight, _lightPath);
    } else {
        GetRenderIndex().RemoveSprim(HdPrimTypeTokens->distantLight, _lightPath);
    }
    _isPopulated = false;
}

void MtohDefaultLightDelegate::SetDefaultLight(const GlfSimpleLight& light)
{
    if (!_isPopulated) {
        return;
    }
    if (ARCH_UNLIKELY(!_isSupported)) {
        return;
    }

    // We only update 3 parameters in the default light : position (in which we store a direction),
    // diffuse and specular
    // We don't never update the transform for the default light
    const bool lightsParamsWeUseAreDifferent = AreLightsParamsWeUseDifferent(_light, light);
    if (lightsParamsWeUseAreDifferent) {
        // Update our light
        _light.SetDiffuse(light.GetDiffuse());
        _light.SetSpecular(light.GetSpecular());
        _light.SetPosition(light.GetPosition());
        GetRenderIndex().GetChangeTracker().MarkSprimDirty(_lightPath, HdLight::DirtyParams);
    }
}

GfMatrix4d MtohDefaultLightDelegate::GetTransform(const SdfPath& id)
{
    TF_UNUSED(id);

    TF_DEBUG(MAYAHYDRALIB_DELEGATE_GET_TRANSFORM)
        .Msg("MtohDefaultLightDelegate::GetTransform(%s)\n", id.GetText());

    // We have to rotate the distant to match the simple light's direction
    // stored in it's position. Otherwise, the matrix needs to be an identity
    // matrix.
    if (!IsHdSt()) {
        const auto  position = _light.GetPosition();
        GfTransform transform;
        transform.SetRotation(
            GfRotation(GfVec3d(0.0, 0.0, -1.0), GfVec3d(-position[0], -position[1], -position[2])));
        return transform.GetMatrix();
    }
    return GfMatrix4d(1.0);
}

VtValue MtohDefaultLightDelegate::Get(const SdfPath& id, const TfToken& key)
{
    TF_UNUSED(id);

    TF_DEBUG(MAYAHYDRALIB_DELEGATE_GET)
        .Msg("MtohDefaultLightDelegate::Get(%s, %s)\n", id.GetText(), key.GetText());

    if (key == HdLightTokens->params) {
        return VtValue(_light);
    } else if (key == HdTokens->transform) {
        return VtValue(GfMatrix4d(
            1.0)); // We don't use the transform but use the position param of the GlfsimpleLight
        // Hydra might crash when this is an empty VtValue.
    } else if (key == HdLightTokens->shadowCollection) {
        if (!_solidPrimitivesRootPaths.empty()) {
            // Exclude lines/points primitives from casting shadows by only taking the primitives
            // whose root path belongs to _solidPrimitivesRootPaths
            HdRprimCollection coll(HdTokens->geometry, HdReprSelector(HdReprTokens->refined));
            coll.SetRootPaths(_solidPrimitivesRootPaths);
            return VtValue(coll);
        } else {
            HdRprimCollection coll(HdTokens->geometry, HdReprSelector(HdReprTokens->refined));
            return VtValue(coll);
        }
    } else if (key == HdLightTokens->shadowParams) {
        HdxShadowParams shadowParams;
        shadowParams.enabled = false;
        return VtValue(shadowParams);
    }
    return {};
}

VtValue MtohDefaultLightDelegate::GetLightParamValue(const SdfPath& id, const TfToken& paramName)
{
    TF_UNUSED(id);

    TF_DEBUG(MAYAHYDRALIB_DELEGATE_GET_LIGHT_PARAM_VALUE)
        .Msg(
            "MtohDefaultLightDelegate::GetLightParamValue(%s, %s)\n",
            id.GetText(),
            paramName.GetText());

    if (paramName == HdLightTokens->color || paramName == HdTokens->displayColor) {
        const auto diffuse = _light.GetDiffuse();
        return VtValue(GfVec3f(diffuse[0], diffuse[1], diffuse[2]));
    } else if (paramName == HdLightTokens->intensity) {
        return VtValue(1.0f);
    } else if (paramName == HdLightTokens->diffuse) {
        return VtValue(1.0f);
    } else if (paramName == HdLightTokens->specular) {
        return VtValue(0.0f);
    } else if (paramName == HdLightTokens->exposure) {
        return VtValue(0.0f);
    } else if (paramName == HdLightTokens->normalize) {
        return VtValue(true);
    } else if (paramName == HdLightTokens->angle) {
        return VtValue(0.0f);
    } else if (paramName == HdLightTokens->shadowEnable) {
        return VtValue(false);
    } else if (paramName == HdLightTokens->shadowColor) {
        return VtValue(GfVec3f(0.0f, 0.0f, 0.0f));
    } else if (paramName == HdLightTokens->enableColorTemperature) {
        return VtValue(false);
    }
    return {};
}

bool MtohDefaultLightDelegate::GetVisible(const SdfPath& id)
{
    TF_UNUSED(id);
    TF_DEBUG(MAYAHYDRALIB_DELEGATE_GET_VISIBLE)
        .Msg("MtohDefaultLightDelegate::GetVisible(%s)\n", id.GetText());
    return true;
}

void MtohDefaultLightDelegate::SetLightingOn(bool isLightingOn)
{
    if (_isLightingOn != isLightingOn) {
        _isLightingOn = isLightingOn;

        RemovePrim();
        Populate();
    }
}

PXR_NAMESPACE_CLOSE_SCOPE
