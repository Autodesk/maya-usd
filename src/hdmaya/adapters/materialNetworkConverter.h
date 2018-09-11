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
#ifndef __HDMAYA_MATERIAL_NETWORK_CONVERTER_H__
#define __HDMAYA_MATERIAL_NETWORK_CONVERTER_H__

#include <hdmaya/api.h>

#include <pxr/base/tf/token.h>

#include <pxr/imaging/hd/material.h>

#include <pxr/usd/sdf/path.h>
#include <pxr/usd/sdf/types.h>

#include <maya/MFnDependencyNode.h>
#include <maya/MObject.h>

PXR_NAMESPACE_OPEN_SCOPE

struct HdMayaShaderParam {
    HdMaterialParam _param;
    SdfValueTypeName _type;

    HdMayaShaderParam(
        const TfToken& name, const VtValue& value, const SdfValueTypeName& type)
        : _param(HdMaterialParam::ParamTypeFallback, name, value),
          _type(type) {}
};

using HdMayaShaderParams = std::vector<HdMayaShaderParam>;

class HdMayaMaterialNetworkConverter {
public:
    HDMAYA_API
    HdMayaMaterialNetworkConverter(
        HdMaterialNetwork& network, const SdfPath& prefix);

    HDMAYA_API
    SdfPath GetMaterial(const MObject& mayaNode);

    HDMAYA_API
    void AddPrimvar(const TfToken& primvar);

    HDMAYA_API
    void ConvertParameter(
        MFnDependencyNode& node, HdMaterialNode& material,
        const TfToken& mayaName, const TfToken& name,
        const SdfValueTypeName& type, const VtValue* fallback = nullptr);

    HDMAYA_API
    static VtValue ConvertPlugToValue(
        const MPlug& plug, const SdfValueTypeName& type);

    HDMAYA_API
    static const HdMayaShaderParams& GetPreviewShaderParams();

    HDMAYA_API
    static const HdMaterialParamVector& GetPreviewMaterialParamVector();

private:
    HdMaterialNetwork& _network;
    const SdfPath& _prefix;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // __HDMAYA_MATERIAL_NETWORK_CONVERTER_H__
