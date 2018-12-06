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
#include "renderGlobals.h"

#include <pxr/imaging/hd/renderDelegate.h>
#include <pxr/imaging/hdx/rendererPlugin.h>
#include <pxr/imaging/hdx/rendererPluginRegistry.h>

#include <maya/MFnDependencyNode.h>
#include <maya/MFnEnumAttribute.h>
#include <maya/MFnNumericAttribute.h>
#include <maya/MFnStringData.h>
#include <maya/MFnTypedAttribute.h>
#include <maya/MGlobal.h>
#include <maya/MPlug.h>
#include <maya/MSelectionList.h>
#include <maya/MStatus.h>

#include "utils.h"

#include <functional>

PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_PRIVATE_TOKENS(
    _tokens, (defaultRenderGlobals)(mtohRenderer)(mtohTextureMemoryPerTexture)(
                 mtohMaximumShadowMapResolution));

namespace {

std::unordered_map<TfToken, HdRenderSettingDescriptorList, TfToken::HashFunctor>
    _rendererAttributes;

void _CreateEnumAttribute(
    MFnDependencyNode& node, const TfToken& attrName,
    const TfTokenVector& values, const TfToken& defValue) {
    const auto attr = node.attribute(MString(attrName.GetText()));
    if (!attr.isNull()) {
        if ([&attr, &values]() -> bool { // Meaning: Can return?
                MStatus status;
                MFnEnumAttribute eAttr(attr, &status);
                if (!status) { return true; }
                short id = 0;
                for (const auto& v : values) {
                    if (eAttr.fieldName(id++) != v.GetText()) { return false; }
                }
                return true;
            }()) {
            return;
        } else {
            node.removeAttribute(attr);
        }
    }
    MFnEnumAttribute eAttr;
    auto o = eAttr.create(attrName.GetText(), attrName.GetText());
    short id = 0;
    for (const auto& v : values) { eAttr.addField(v.GetText(), id++); }
    eAttr.setDefault(defValue.GetText());
    node.addAttribute(o);
}

void _CreateTypedAttribute(
    MFnDependencyNode& node, const TfToken& attrName, MFnData::Type type,
    const std::function<MObject()>& creator) {
    const auto attr = node.attribute(attrName.GetText());
    if (!attr.isNull()) {
        MStatus status;
        MFnTypedAttribute tAttr(attr, &status);
        if (status && tAttr.attrType() == type) { return; }
        node.removeAttribute(attr);
    }
    node.addAttribute(creator());
}

void _CreateNumericAttribute(
    MFnDependencyNode& node, const TfToken& attrName, MFnNumericData::Type type,
    const std::function<MObject()>& creator) {
    const auto attr = node.attribute(attrName.GetText());
    if (!attr.isNull()) {
        MStatus status;
        MFnNumericAttribute nAttr(attr, &status);
        if (status && nAttr.unitType() == type) { return; }
        node.removeAttribute(attr);
    }
    node.addAttribute(creator());
}

void _SetToken(
    const MFnDependencyNode& node, const TfToken& attrName, TfToken& out) {
    const auto plug = node.findPlug(attrName.GetText());
    if (!plug.isNull()) { out = TfToken(plug.asString().asChar()); }
}

void _SetEnum(
    const MFnDependencyNode& node, const TfToken& attrName, TfToken& out) {
    const auto plug = node.findPlug(attrName.GetText());
    if (plug.isNull()) { return; }
    MStatus status;
    MFnEnumAttribute eAttr(plug.attribute(), &status);
    if (!status) { return; }
    out = TfToken(eAttr.fieldName(plug.asShort()).asChar());
}

template <typename T>
void _SetFromPlug(const MPlug& plug, T& out) {
    static_assert(true, "Specialisation not implemented for SetFromPlug");
}

template <>
void _SetFromPlug<int>(const MPlug& plug, int& out) {
    out = plug.asInt();
}

template <typename T>
bool _SetNumericAttribute(
    const MFnDependencyNode& node, const TfToken& attrName, T& out) {
    const auto plug = node.findPlug(attrName.GetText());
    if (plug.isNull()) { return false; }
    _SetFromPlug<T>(plug, out);
    return true;
}

constexpr auto _renderOverrideOptionBoxCommand = R"mel(
global proc hydraViewportOverrideOptionBox() {
    string $windowName = "hydraViewportOverrideOptionsWindow";
    if (`window -exists $windowName`) {
        showWindow $windowName;
        return;
    }
    string $cc = "mtoh -updateRenderGlobals; refresh -f";

    mtoh -createRenderGlobals;

    window -title "Maya to Hydra Settings" "hydraViewportOverrideOptionsWindow";
    scrollLayout;
    frameLayout -label "Hydra Settings";
    columnLayout;
    attrControlGrp -label "Renderer Name" -attribute "defaultRenderGlobals.mtohRenderer" -changeCommand $cc;
    attrControlGrp -label "Texture Memory Per Texture (KB)" -attribute "defaultRenderGlobals.mtohTextureMemoryPerTexture" -changeCommand $cc;
    attrControlGrp -label "Maximum Shadow Map Resolution" -attribute "defaultRenderGlobals.mtohMaximumShadowMapResolution" -changeCommand $cc;
    setParent ..;
    setParent ..;
    setParent ..;

    showWindow $windowName;
}
)mel";
} // namespace

MtohRenderGlobals::MtohRenderGlobals() : renderer(MtohGetDefaultRenderer()) {}

void MtohInitializeRenderGlobals() {
    for (const auto& rendererPluginName : MtohGetRendererPlugins()) {
        auto* rendererPlugin =
            HdxRendererPluginRegistry::GetInstance().GetRendererPlugin(
                rendererPluginName);
        if (rendererPlugin == nullptr) { continue; }
        auto* renderDelegate = rendererPlugin->CreateRenderDelegate();
        if (renderDelegate == nullptr) { continue; }
        _rendererAttributes[rendererPluginName] =
            renderDelegate->GetRenderSettingDescriptors();
        delete renderDelegate;
    }
    MGlobal::executeCommand(_renderOverrideOptionBoxCommand);
}

MObject MtohCreateRenderGlobals() {
    MSelectionList slist;
    slist.add(_tokens->defaultRenderGlobals.GetText());
    MObject ret;
    if (slist.length() == 0 || !slist.getDependNode(0, ret)) { return ret; }
    MStatus status;
    MFnDependencyNode node(ret, &status);
    if (!status) { return MObject(); }
    _CreateEnumAttribute(
        node, _tokens->mtohRenderer, MtohGetRendererPlugins(),
        MtohGetDefaultRenderer());
    _CreateNumericAttribute(
        node, _tokens->mtohTextureMemoryPerTexture, MFnNumericData::kInt,
        []() -> MObject {
            MFnNumericAttribute nAttr;
            const auto o = nAttr.create(
                _tokens->mtohTextureMemoryPerTexture.GetText(),
                _tokens->mtohTextureMemoryPerTexture.GetText(),
                MFnNumericData::kInt);
            nAttr.setMin(1);
            nAttr.setMax(256 * 1024);
            nAttr.setSoftMin(1 * 1024);
            nAttr.setSoftMin(16 * 1024);
            HdMayaParams params;
            nAttr.setDefault(params.textureMemoryPerTexture / 1024);
            return o;
        });
    _CreateNumericAttribute(
        node, _tokens->mtohMaximumShadowMapResolution, MFnNumericData::kInt,
        []() -> MObject {
            MFnNumericAttribute nAttr;
            const auto o = nAttr.create(
                _tokens->mtohMaximumShadowMapResolution.GetText(),
                _tokens->mtohMaximumShadowMapResolution.GetText(),
                MFnNumericData::kInt);
            nAttr.setMin(32);
            nAttr.setMax(8192);
            HdMayaParams params;
            nAttr.setDefault(params.maximumShadowMapResolution);
            return o;
        });
    return ret;
}

MtohRenderGlobals MtohGetRenderGlobals() {
    const auto obj = MtohCreateRenderGlobals();
    MtohRenderGlobals ret{};
    if (obj.isNull()) { return ret; }
    MStatus status;
    MFnDependencyNode node(obj, &status);
    if (!status) { return ret; }
    _SetEnum(node, _tokens->mtohRenderer, ret.renderer);
    if (_SetNumericAttribute(
            node, _tokens->mtohTextureMemoryPerTexture,
            ret.delegateParams.textureMemoryPerTexture)) {
        ret.delegateParams.textureMemoryPerTexture *= 1024;
    }
    _SetNumericAttribute(
        node, _tokens->mtohMaximumShadowMapResolution,
        ret.delegateParams.maximumShadowMapResolution);
    return ret;
}

PXR_NAMESPACE_CLOSE_SCOPE
