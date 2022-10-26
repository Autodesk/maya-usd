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
#ifndef HDMAYA_MATERIAL_NETWORK_CONVERTER_H
#define HDMAYA_MATERIAL_NETWORK_CONVERTER_H

#include <hdMaya/api.h>

#include <pxr/base/tf/token.h>
#include <pxr/imaging/hd/material.h>
#include <pxr/usd/sdf/path.h>
#include <pxr/usd/sdf/types.h>

#include <maya/MFnDependencyNode.h>
#include <maya/MObject.h>
#include <maya/MShaderManager.h>

#include <unordered_map>

PXR_NAMESPACE_OPEN_SCOPE

struct HdMayaShaderParam
{
    TfToken name;
    VtValue fallbackValue;

    SdfValueTypeName type;

    HDMAYA_API
    HdMayaShaderParam(const TfToken& name, const VtValue& value, const SdfValueTypeName& type);
};

using HdMayaShaderParams = std::vector<HdMayaShaderParam>;

/// Class which provides basic name and value translation for an attribute.
/// Used by both HdMayaMaterialNetworkConverter (for to-usd file export
/// translation) and HdMayaMaterialAdapter (for translation to Hydra).
class HdMayaMaterialAttrConverter
{
public:
    typedef std::shared_ptr<HdMayaMaterialAttrConverter> RefPtr;

    virtual ~HdMayaMaterialAttrConverter() {};

    /// Returns the default type for this attr converter - if an
    /// implementation returns an invalid type, this indicates the attr
    /// converter's type is undefined / variable.
    virtual SdfValueTypeName GetType() = 0;

    /// If there is a simple, one-to-one mapping from the usd/hydra attribute
    /// we are trying to "get", and a corresponding maya plug, AND the value
    /// can be used "directly", then this should return the name of the maya
    /// plug. Otherwise it should return an empty token.
    /// By returning an empty token, we indicate that we want to set a value,
    /// but that we don't wish to set up any network connections (ie, textures,
    /// etc.)
    HDMAYA_API
    virtual TfToken GetPlugName(const TfToken& usdName) = 0;

    /// Returns the value computed from maya for the usd/hydra attribute
    //    HDMAYA_API
    //    virtual VtValue GetValue(
    //        const HdMayaShaderParam& destParam, MFnDependencyNode& node,
    //        MPlug* outPlug = nullptr) = 0;

    HDMAYA_API
    virtual VtValue GetValue(
        MFnDependencyNode&      node,
        const TfToken&          paramName,
        const SdfValueTypeName& type,
        const VtValue*          fallback = nullptr,
        MPlug*                  outPlug = nullptr)
        = 0;
};

/// Class which provides basic name and value translation for a maya node
/// type. Used by both HdMayaMaterialNetworkConverter (for to-usd file
/// export translation) and HdMayaMaterialAdapter (for translation to Hydra).
class HdMayaMaterialNodeConverter
{
public:
    typedef std::unordered_map<TfToken, HdMayaMaterialAttrConverter::RefPtr, TfToken::HashFunctor>
        NameToAttrConverterMap;

    HDMAYA_API
    HdMayaMaterialNodeConverter(
        const TfToken&                identifier,
        const NameToAttrConverterMap& attrConverters);

    inline TfToken GetIdentifier() { return _identifier; }

    /// Try to find the correct attribute converter to use for the given
    /// param; if nothing is found, will usually return a generic converter,
    /// that will look for an attribute on the maya node with the same name, and
    /// use that if possible.
    HDMAYA_API
    HdMayaMaterialAttrConverter::RefPtr GetAttrConverter(const TfToken& paramName);

    inline NameToAttrConverterMap& GetAttrConverters() { return _attrConverters; }

    HDMAYA_API
    static HdMayaMaterialNodeConverter* GetNodeConverter(const TfToken& nodeType);

private:
    NameToAttrConverterMap _attrConverters;
    mutable TfToken        _identifier;
};

class HdMayaMaterialNetworkConverter
{
public:
    typedef std::unordered_map<SdfPath, MObject, SdfPath::Hash> PathToMobjMap;

    HDMAYA_API
    HdMayaMaterialNetworkConverter(
        HdMaterialNetwork& network,
        const SdfPath&     prefix,
        PathToMobjMap*     pathToMobj = nullptr);

    HDMAYA_API
    HdMaterialNode* GetMaterial(const MObject& mayaNode);

    HDMAYA_API
    void AddPrimvar(const TfToken& primvar);

    HDMAYA_API
    void ConvertParameter(
        MFnDependencyNode&           node,
        HdMayaMaterialNodeConverter& nodeConverter,
        HdMaterialNode&              material,
        const TfToken&               paramName,
        const SdfValueTypeName&      type,
        const VtValue*               fallback = nullptr);

    HDMAYA_API static VtValue ConvertMayaAttrToValue(
        MFnDependencyNode&      node,
        const MString&          plugName,
        const SdfValueTypeName& type,
        const VtValue*          fallback = nullptr,
        MPlug*                  outPlug = nullptr);

    HDMAYA_API static VtValue ConvertMayaAttrToScaledValue(
        MFnDependencyNode&      node,
        const MString&          plugName,
        const MString&          scaleName,
        const SdfValueTypeName& type,
        const VtValue*          fallback = nullptr,
        MPlug*                  outPlug = nullptr);

    HDMAYA_API
    static void initialize();

    HDMAYA_API
    static VtValue ConvertPlugToValue(
        const MPlug&            plug,
        const SdfValueTypeName& type,
        const VtValue*          fallback = nullptr);

    HDMAYA_API
    static const HdMayaShaderParams& GetPreviewShaderParams();

	HDMAYA_API
	static const HdMayaShaderParams& GetShaderParams(const TfToken& shaderIdentifier);

private:
    HdMaterialNetwork& _network;
    const SdfPath&     _prefix;
    PathToMobjMap*     _pathToMobj;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // HDMAYA_MATERIAL_NETWORK_CONVERTER_H
