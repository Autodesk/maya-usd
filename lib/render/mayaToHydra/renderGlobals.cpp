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
#include "renderGlobals.h"

#include <pxr/imaging/hd/renderDelegate.h>
#if USD_VERSION_NUM >= 1911
#include <pxr/imaging/hd/rendererPlugin.h>
#include <pxr/imaging/hd/rendererPluginRegistry.h>
#else
#include <pxr/imaging/hdx/rendererPlugin.h>
#include <pxr/imaging/hdx/rendererPluginRegistry.h>
PXR_NAMESPACE_OPEN_SCOPE
using HdRendererPluginRegistry = HdxRendererPluginRegistry;
PXR_NAMESPACE_CLOSE_SCOPE
#endif

#include <maya/MFnDependencyNode.h>
#include <maya/MFnEnumAttribute.h>
#include <maya/MFnNumericAttribute.h>
#include <maya/MFnStringData.h>
#include <maya/MFnTypedAttribute.h>
#include <maya/MGlobal.h>
#include <maya/MPlug.h>
#include <maya/MSelectionList.h>
#include <maya/MStatus.h>

#include "tokens.h"
#include "utils.h"

#include <functional>
#include <sstream>

PXR_NAMESPACE_OPEN_SCOPE

// clang-format off
TF_DEFINE_PRIVATE_TOKENS(
    _tokens,
    (defaultRenderGlobals)
    (mtohTextureMemoryPerTexture)
    (mtohMaximumShadowMapResolution)
    (mtohColorSelectionHighlight)
    (mtohColorSelectionHighlightColor)
    (mtohColorSelectionHighlightColorA)
    (mtohWireframeSelectionHighlight)
    (mtohSelectionOverlay)
    (mtohEnableMotionSamples)
    );
// clang-format on

namespace {

#if USD_VERSION_NUM >= 1901
std::unordered_map<TfToken, HdRenderSettingDescriptorList, TfToken::HashFunctor>
    _rendererAttributes;
#endif

void _CreateEnumAttribute(
    MFnDependencyNode& node, const TfToken& attrName,
    const TfTokenVector& values, const TfToken& defValue) {
    const auto attr = node.attribute(MString(attrName.GetText()));
    if (!attr.isNull()) {
        if ([&attr, &values]() -> bool { // Meaning: Can return?
                MStatus status;
                MFnEnumAttribute eAttr(attr, &status);
                if (!status) { return false; }
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

void _CreateColorAttribute(
    MFnDependencyNode& node, const TfToken& attrName, const TfToken& attrAName,
    const GfVec4f& defValue) {
    const auto attr = node.attribute(attrName.GetText());
    auto foundColor = false;
    if (!attr.isNull()) {
        MStatus status;
        MFnNumericAttribute nAttr(attr, &status);
        if (status && nAttr.isUsedAsColor()) {
            foundColor = true;
        } else {
            node.removeAttribute(attr);
        }
    }
    const auto attrA = node.attribute(attrAName.GetText());
    auto foundAlpha = false;
    if (!attrA.isNull()) {
        MStatus status;
        MFnNumericAttribute nAttr(attrA, &status);
        if (status && nAttr.unitType() == MFnNumericData::kFloat) {
            if (foundColor) { return; }
            foundAlpha = true;
        } else {
            node.removeAttribute(attrA);
        }
    }
    MFnNumericAttribute nAttr;
    if (!foundColor) {
        const auto o =
            nAttr.createColor(attrName.GetText(), attrName.GetText());
        nAttr.setDefault(defValue[0], defValue[1], defValue[2]);
        node.addAttribute(o);
    }
    if (!foundAlpha) {
        const auto o = nAttr.create(
            attrAName.GetText(), attrAName.GetText(), MFnNumericData::kFloat);
        nAttr.setDefault(defValue[3]);
        node.addAttribute(o);
    }
}

void _CreateBoolAttribute(
    MFnDependencyNode& node, const TfToken& attrName, bool defValue) {
    _CreateNumericAttribute(
        node, attrName, MFnNumericData::kBoolean,
        [&attrName, &defValue]() -> MObject {
    MFnNumericAttribute nAttr;
    const auto o = nAttr.create(
                attrName.GetText(), attrName.GetText(),
                MFnNumericData::kBoolean);
    nAttr.setDefault(defValue);
    return o;
        });
}

void _CreateStringAttribute(
    MFnDependencyNode& node, const TfToken& attrName,
    const std::string& defValue) {
    _CreateTypedAttribute(
        node, attrName, MFnData::kString, [&attrName, &defValue]() -> MObject {
            MFnTypedAttribute tAttr;
            const auto o = tAttr.create(
                attrName.GetText(), attrName.GetText(), MFnData::kString);
            if (!defValue.empty()) {
                MFnStringData strData;
                MObject defObj = strData.create(defValue.c_str());
                tAttr.setDefault(defObj);
            }
            return o;
        });
}

void _GetEnum(
    const MFnDependencyNode& node, const TfToken& attrName, TfToken& out) {
    const auto plug = node.findPlug(attrName.GetText(), true);
    if (plug.isNull()) { return; }
    MStatus status;
    MFnEnumAttribute eAttr(plug.attribute(), &status);
    if (!status) { return; }
    out = TfToken(eAttr.fieldName(plug.asShort()).asChar());
}

template <typename T>
void _GetFromPlug(const MPlug& plug, T& out) {
    assert(false);
}

template <>
void _GetFromPlug<bool>(const MPlug& plug, bool& out) {
    out = plug.asBool();
}

template <>
void _GetFromPlug<int>(const MPlug& plug, int& out) {
    out = plug.asInt();
}

#if USD_VERSION_NUM >= 1901
template <>
void _GetFromPlug<float>(const MPlug& plug, float& out) {
    out = plug.asFloat();
}
#endif

template <>
void _GetFromPlug<std::string>(const MPlug& plug, std::string& out) {
    out = plug.asString().asChar();
}

template <typename T>
bool _GetAttribute(
    const MFnDependencyNode& node, const TfToken& attrName, T& out) {
    const auto plug = node.findPlug(attrName.GetText(), true);
    if (plug.isNull()) { return false; }
    _GetFromPlug<T>(plug, out);
    return true;
}

void _GetColorAttribute(
    const MFnDependencyNode& node, const TfToken& attrName,
    const TfToken& attrAName, GfVec4f& out) {
    const auto plug = node.findPlug(attrName.GetText(), true);
    if (plug.isNull()) { return; }
    out[0] = plug.child(0).asFloat();
    out[1] = plug.child(1).asFloat();
    out[2] = plug.child(2).asFloat();
    const auto plugA = node.findPlug(attrAName.GetText(), true);
    if (plugA.isNull()) { return; }
    out[3] = plugA.asFloat();
}

#if USD_VERSION_NUM >= 1901
bool _IsSupportedAttribute(const VtValue& v) {
    return v.IsHolding<bool>() || v.IsHolding<int>() || v.IsHolding<float>() ||
           v.IsHolding<GfVec4f>() || v.IsHolding<std::string>();
}
#endif

constexpr auto _renderOverrideOptionBoxTemplate = R"mel(
global proc {{override}}OptionBox() {
    string $windowName = "{{override}}OptionsWindow";
    if (`window -exists $windowName`) {
        showWindow $windowName;
        return;
    }
    string $cc = "mtoh -updateRenderGlobals; refresh -f";

    mtoh -createRenderGlobals;

    window -title "Maya to Hydra Settings" "{{override}}OptionsWindow";
    scrollLayout;
    frameLayout -label "Hydra Settings";
    columnLayout;
    attrControlGrp -label "Enable Motion Samples" -attribute "defaultRenderGlobals.mtohEnableMotionSamples" -changeCommand $cc;
    attrControlGrp -label "Texture Memory Per Texture (KB)" -attribute "defaultRenderGlobals.mtohTextureMemoryPerTexture" -changeCommand $cc;
    attrControlGrp -label "OpenGL Selection Overlay" -attribute "defaultRenderGlobals.mtohSelectionOverlay" -changeCommand $cc;
    attrControlGrp -label "Show Wireframe on Selected Objects" -attribute "defaultRenderGlobals.mtohWireframeSelectionHighlight" -changeCommand $cc;
    attrControlGrp -label "Highlight Selected Objects" -attribute "defaultRenderGlobals.mtohColorSelectionHighlight" -changeCommand $cc;
    attrControlGrp -label "Highlight Color for Selected Objects" -attribute "defaultRenderGlobals.mtohColorSelectionHighlightColor" -changeCommand $cc;
    setParent ..;
    setParent ..;
    {{override}}Options();
    setParent ..;

    showWindow $windowName;
}
)mel";
} // namespace

MtohRenderGlobals::MtohRenderGlobals() : selectionOverlay(MtohTokens->UseVp2) {}

void MtohInitializeRenderGlobals() {
    const auto& rendererDescs = MtohGetRendererDescriptions();
    for (const auto& rendererDesc : rendererDescs) {
        const auto optionBoxCommand = TfStringReplace(
            _renderOverrideOptionBoxTemplate, "{{override}}",
            rendererDesc.overrideName.GetText());
        auto status = MGlobal::executeCommand(optionBoxCommand.c_str());
        if (!status) {
            TF_WARN(
                "Error in render override option box command function: \n%s",
                status.errorString().asChar());
        }
#if USD_VERSION_NUM >= 1901
        auto* rendererPlugin =
            HdRendererPluginRegistry::GetInstance().GetRendererPlugin(
                rendererDesc.rendererName);
        if (rendererPlugin == nullptr) { continue; }
        auto* renderDelegate = rendererPlugin->CreateRenderDelegate();
        if (renderDelegate == nullptr) { continue; }
        const auto rendererSettingDescriptors =
            renderDelegate->GetRenderSettingDescriptors();
        _rendererAttributes[rendererDesc.rendererName] =
            rendererSettingDescriptors;
        delete renderDelegate;

        std::stringstream ss;
        ss << "global proc " << rendererDesc.overrideName << "Options() {\n";
        ss << "\tstring $cc = \"mtoh -updateRenderGlobals; refresh -f\";\n";
        ss << "\tframeLayout -label \"" << rendererDesc.displayName
           << "Options\" -collapsable true;\n";
        ss << "\tcolumnLayout;\n";
        for (const auto& desc : rendererSettingDescriptors) {
            if (!_IsSupportedAttribute(desc.defaultValue)) { continue; }
            const auto attrName = TfStringPrintf(
                "%s%s", rendererDesc.rendererName.GetText(),
                desc.key.GetText());
            ss << "\tattrControlGrp -label \"" << desc.name
               << "\" -attribute \"defaultRenderGlobals." << attrName
               << "\" -changeCommand $cc;\n";
        }
        if (rendererDesc.rendererName == MtohTokens->HdStormRendererPlugin) {
            ss << "\tattrControlGrp -label \"Maximum shadow map size"
               << "\" -attribute \"defaultRenderGlobals."
               << _tokens->mtohMaximumShadowMapResolution.GetString()
               << "\" -changeCommand $cc;\n";
        }
        ss << "\tsetParent ..;\n";
        ss << "\tsetParent ..;\n";
        ss << "}\n";

        const auto optionsCommand = ss.str();
#else
        const auto optionsCommand = TfStringPrintf(
            "global proc %sOptions() { }", rendererDesc.overrideName.GetText());
#endif
        status = MGlobal::executeCommand(optionsCommand.c_str());
        if (!status) {
            TF_WARN(
                "Error in render delegate options function: \n%s",
                status.errorString().asChar());
        }
    }
}

MObject MtohCreateRenderGlobals() {
    MSelectionList slist;
    slist.add(_tokens->defaultRenderGlobals.GetText());
    MObject ret;
    if (slist.length() == 0 || !slist.getDependNode(0, ret)) { return ret; }
    MStatus status;
    MFnDependencyNode node(ret, &status);
    if (!status) { return MObject(); }
    static const MtohRenderGlobals defGlobals;
    _CreateBoolAttribute(
        node, _tokens->mtohEnableMotionSamples,
        defGlobals.delegateParams.enableMotionSamples);
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
            nAttr.setDefault(
                defGlobals.delegateParams.textureMemoryPerTexture / 1024);
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
            nAttr.setDefault(
                defGlobals.delegateParams.maximumShadowMapResolution);
            return o;
        });
    static const TfTokenVector selectionOverlays{MtohTokens->UseHdSt,
                                                 MtohTokens->UseVp2};
    _CreateEnumAttribute(
        node, _tokens->mtohSelectionOverlay, selectionOverlays,
        defGlobals.selectionOverlay);
    _CreateBoolAttribute(
        node, _tokens->mtohWireframeSelectionHighlight,
        defGlobals.wireframeSelectionHighlight);
    _CreateBoolAttribute(
        node, _tokens->mtohColorSelectionHighlight,
        defGlobals.colorSelectionHighlight);
    _CreateColorAttribute(
        node, _tokens->mtohColorSelectionHighlightColor,
        _tokens->mtohColorSelectionHighlightColorA,
        defGlobals.colorSelectionHighlightColor);
    // TODO: Move this to an external function and add support for more types,
    //  and improve code quality/reuse.
#if USD_VERSION_NUM >= 1901
    for (const auto& rit : _rendererAttributes) {
        const auto rendererName = rit.first;
        for (const auto& attr : rit.second) {
            const TfToken attrName(TfStringPrintf(
                "%s%s", rendererName.GetText(), attr.key.GetText()));
            if (attr.defaultValue.IsHolding<bool>()) {
                _CreateBoolAttribute(
                    node, attrName, attr.defaultValue.UncheckedGet<bool>());
            } else if (attr.defaultValue.IsHolding<int>()) {
                _CreateNumericAttribute(
                    node, attrName, MFnNumericData::kInt,
                    [&attrName, &attr]() -> MObject {
                        MFnNumericAttribute nAttr;
                        const auto o = nAttr.create(
                            attrName.GetText(), attrName.GetText(),
                            MFnNumericData::kInt);
                        nAttr.setDefault(attr.defaultValue.UncheckedGet<int>());
                        return o;
                    });
            } else if (attr.defaultValue.IsHolding<float>()) {
                _CreateNumericAttribute(
                    node, attrName, MFnNumericData::kFloat,
                    [&attrName, &attr]() -> MObject {
                        MFnNumericAttribute nAttr;
                        const auto o = nAttr.create(
                            attrName.GetText(), attrName.GetText(),
                            MFnNumericData::kFloat);
                        nAttr.setDefault(
                            attr.defaultValue.UncheckedGet<float>());
                        return o;
                    });
            } else if (attr.defaultValue.IsHolding<GfVec4f>()) {
                const TfToken attrAName(
                    TfStringPrintf("%sA", attrName.GetText()));
                _CreateColorAttribute(
                    node, attrName, attrAName,
                    attr.defaultValue.UncheckedGet<GfVec4f>());
            } else if (attr.defaultValue.IsHolding<std::string>()) {
                _CreateStringAttribute(
                    node, attrName,
                    attr.defaultValue.UncheckedGet<std::string>());
            }
        }
    }

#endif
    return ret;
}

MtohRenderGlobals MtohGetRenderGlobals() {
    const auto obj = MtohCreateRenderGlobals();
    MtohRenderGlobals ret{};
    if (obj.isNull()) { return ret; }
    MStatus status;
    MFnDependencyNode node(obj, &status);
    if (!status) { return ret; }
    if (_GetAttribute(
            node, _tokens->mtohTextureMemoryPerTexture,
            ret.delegateParams.textureMemoryPerTexture)) {
        ret.delegateParams.textureMemoryPerTexture *= 1024;
    }
    _GetAttribute(
        node, _tokens->mtohEnableMotionSamples,
        ret.delegateParams.enableMotionSamples);
    _GetAttribute(
        node, _tokens->mtohMaximumShadowMapResolution,
        ret.delegateParams.maximumShadowMapResolution);
    _GetEnum(node, _tokens->mtohSelectionOverlay, ret.selectionOverlay);
    _GetAttribute(
        node, _tokens->mtohWireframeSelectionHighlight,
        ret.wireframeSelectionHighlight);
    _GetAttribute(
        node, _tokens->mtohColorSelectionHighlight,
        ret.colorSelectionHighlight);
    _GetColorAttribute(
        node, _tokens->mtohColorSelectionHighlightColor,
        _tokens->mtohColorSelectionHighlightColorA,
        ret.colorSelectionHighlightColor);
    // TODO: Move this to an external function and add support for more types,
    //  and improve code quality/reuse.
#if USD_VERSION_NUM >= 1901
    for (const auto& rit : _rendererAttributes) {
        const auto rendererName = rit.first;
        auto& settings = ret.rendererSettings[rendererName];
        settings.reserve(rit.second.size());
        for (const auto& attr : rit.second) {
            const TfToken attrName(TfStringPrintf(
                "%s%s", rendererName.GetText(), attr.key.GetText()));
            if (attr.defaultValue.IsHolding<bool>()) {
                auto v = attr.defaultValue.UncheckedGet<bool>();
                _GetAttribute(node, attrName, v);
                settings.emplace_back(attr.key, v);
            } else if (attr.defaultValue.IsHolding<int>()) {
                auto v = attr.defaultValue.UncheckedGet<int>();
                _GetAttribute(node, attrName, v);
                settings.emplace_back(attr.key, v);
            } else if (attr.defaultValue.IsHolding<float>()) {
                auto v = attr.defaultValue.UncheckedGet<float>();
                _GetAttribute(node, attrName, v);
                settings.emplace_back(attr.key, v);
            } else if (attr.defaultValue.IsHolding<GfVec4f>()) {
                auto v = attr.defaultValue.UncheckedGet<GfVec4f>();
                const TfToken attrAName(
                    TfStringPrintf("%sA", attrName.GetText()));
                _GetColorAttribute(node, attrName, attrAName, v);
                settings.emplace_back(attr.key, v);
            } else if (attr.defaultValue.IsHolding<std::string>()) {
                auto v = attr.defaultValue.UncheckedGet<std::string>();
                _GetAttribute(node, attrName, v);
                settings.emplace_back(attr.key, v);
            }
        }
    }
#endif
    return ret;
}

PXR_NAMESPACE_CLOSE_SCOPE
