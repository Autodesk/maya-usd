//
// Copyright 2018 Luma Pictures
//
// Licensed under the Apache License, Version 2.0 (the "Apache License")
// with the following modification you may not use this file except in
// compliance with the Apache License and the following modification to it:
// Section 6. Trademarks. is deleted and replaced with:
//
// 6. Trademarks. This License does not grant permission to use the trade
//    names, trademarks, service marks, or product names of the Licensor
//    and its affiliates, except as required to comply with Section 4(c) of
//    the License and to reproduce the content of the NOTICE file.
//
// You may obtain a copy of the Apache License at
//
//     http:#www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the Apache License with the above modification is
// distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied. See the Apache License for the specific
// language governing permissions and limitations under the Apache License.
//
#include "defaultLightDelegate.h"

#include <pxr/base/gf/rotation.h>
#include <pxr/base/gf/transform.h>

#include <pxr/imaging/hd/light.h>
#include <pxr/imaging/hd/tokens.h>
#include <pxr/imaging/hdx/simpleLightTask.h>

PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_PRIVATE_TOKENS(_tokens, (DefaultMayaLight));

MtohDefaultLightDelegate::MtohDefaultLightDelegate(
    HdRenderIndex* renderIndex, const SdfPath& delegateID)
    : HdSceneDelegate(renderIndex, delegateID),
      HdMayaDelegate(),
      _lightPath(delegateID.AppendChild(_tokens->DefaultMayaLight)),
      _isSupported(false) {}

MtohDefaultLightDelegate::~MtohDefaultLightDelegate() {
    if (ARCH_UNLIKELY(!_isSupported)) { return; }
    if (GetPreferSimpleLight()) {
        GetRenderIndex().RemoveSprim(HdPrimTypeTokens->simpleLight, _lightPath);
    } else {
        GetRenderIndex().RemoveSprim(
            HdPrimTypeTokens->distantLight, _lightPath);
    }
}

void MtohDefaultLightDelegate::Populate() {
    _isSupported = GetPreferSimpleLight()
                       ? GetRenderIndex().IsSprimTypeSupported(
                             HdPrimTypeTokens->simpleLight)
                       : GetRenderIndex().IsSprimTypeSupported(
                             HdPrimTypeTokens->distantLight);
    if (ARCH_UNLIKELY(!_isSupported)) { return; }
    if (GetPreferSimpleLight()) {
        GetRenderIndex().InsertSprim(
            HdPrimTypeTokens->simpleLight, this, _lightPath);
    } else {
        GetRenderIndex().InsertSprim(
            HdPrimTypeTokens->distantLight, this, _lightPath);
    }
    GetRenderIndex().GetChangeTracker().SprimInserted(
        _lightPath, HdLight::AllDirty);
}

void MtohDefaultLightDelegate::SetDefaultLight(const GlfSimpleLight& light) {
    if (ARCH_UNLIKELY(!_isSupported)) { return; }
    if (_light != light) {
        _light = light;
        GetRenderIndex().GetChangeTracker().MarkSprimDirty(
            _lightPath, HdLight::DirtyParams | HdLight::DirtyTransform);
    }
}

GfMatrix4d MtohDefaultLightDelegate::GetTransform(const SdfPath& id) {
    TF_UNUSED(id);
    // We have to rotate the distant to match the simple light's direction
    // stored in it's position. Otherwise, the matrix needs to be an identity
    // matrix.
    if (!GetPreferSimpleLight()) {
        const auto position = _light.GetPosition();
        GfTransform transform;
        transform.SetRotation(GfRotation(
            GfVec3d(0.0, 0.0, -1.0),
            GfVec3d(-position[0], -position[1], -position[2])));
        return transform.GetMatrix();
    }
    return GfMatrix4d(1.0);
}

VtValue MtohDefaultLightDelegate::Get(const SdfPath& id, const TfToken& key) {
    TF_UNUSED(id);
    if (key == HdLightTokens->params) {
        return VtValue(_light);
    } else if (key == HdTokens->transform) {
        // return VtValue(MtohDefaultLightDelegate::GetTransform(id));
        return VtValue(GfMatrix4d(1.0));
        // Hydra might crash when this is an empty VtValue.
    } else if (key == HdLightTokens->shadowCollection) {
        HdRprimCollection coll(
            HdTokens->geometry, HdReprSelector(HdReprTokens->refined));
        coll.SetRenderTags({HdTokens->geometry});
        return VtValue(coll);
    } else if (key == HdLightTokens->shadowParams) {
        HdxShadowParams shadowParams;
        shadowParams.enabled = false;
        return VtValue(shadowParams);
    }
    return {};
}

VtValue MtohDefaultLightDelegate::GetLightParamValue(
    const SdfPath& id, const TfToken& paramName) {
    TF_UNUSED(id);
    if (paramName == HdTokens->color) {
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
    }
    return {};
}

bool MtohDefaultLightDelegate::GetVisible(const SdfPath& id) {
    TF_UNUSED(id);
    return true;
}

PXR_NAMESPACE_CLOSE_SCOPE
