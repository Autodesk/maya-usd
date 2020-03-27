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
#include <pxr/imaging/hd/rendererPlugin.h>
#include <pxr/imaging/hd/rendererPluginRegistry.h>

#include <maya/MFnDependencyNode.h>
#include <maya/MFnEnumAttribute.h>
#include <maya/MFnNumericAttribute.h>
#include <maya/MFnStringData.h>
#include <maya/MFnTypedAttribute.h>
#include <maya/MPlug.h>
#include <maya/MSelectionList.h>
#include <maya/MStatus.h>

#include "utils.h"

#include <functional>
#include <sstream>

PXR_NAMESPACE_OPEN_SCOPE

// clang-format off
TF_DEFINE_PRIVATE_TOKENS(
    _tokens,
    (defaultRenderGlobals)
    (mtohTextureMemoryPerTexture)
    (mtohColorSelectionHighlight)
    (mtohColorSelectionHighlightColor)
    (mtohColorSelectionHighlightColorA)
    (mtohWireframeSelectionHighlight)
    (mtohSelectionOverlay)
    (mtohEnableMotionSamples)
    );
// clang-format on

namespace {

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

void _CreateEnumAttribute(MFnDependencyNode& node, const TfToken& attrName,
    const TfEnum& defValue) {
    std::vector<std::string> names = TfEnum::GetAllNames(defValue);
    TfTokenVector tokens(names.begin(), names.end());
    return _CreateEnumAttribute(node, attrName, tokens,
        TfToken(TfEnum::GetDisplayName(defValue)));
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

template <>
void _GetFromPlug<float>(const MPlug& plug, float& out) {
    out = plug.asFloat();
}

template <>
void _GetFromPlug<std::string>(const MPlug& plug, std::string& out) {
    out = plug.asString().asChar();
}

template <>
void _GetFromPlug<TfEnum>(const MPlug& plug, TfEnum& out) {
    out = TfEnum(out.GetType(), plug.asInt());
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


} // namespace

MtohRenderGlobals::MtohRenderGlobals() {}

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
        node, MtohTokens->mtohMaximumShadowMapResolution, MFnNumericData::kInt,
        []() -> MObject {
            MFnNumericAttribute nAttr;
            const auto o = nAttr.create(
                MtohTokens->mtohMaximumShadowMapResolution.GetText(),
                MtohTokens->mtohMaximumShadowMapResolution.GetText(),
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
    for (const auto& rit : MtohGetRendererSettings()) {
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
            } else if (attr.defaultValue.IsHolding<TfEnum>()) {
                _CreateEnumAttribute(
                    node, attrName,
                    attr.defaultValue.UncheckedGet<TfEnum>());
            }
        }
    }
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
        node, MtohTokens->mtohMaximumShadowMapResolution,
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
    for (const auto& rit : MtohGetRendererSettings()) {
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
            } else if (attr.defaultValue.IsHolding<TfEnum>()) {
                auto v = attr.defaultValue.UncheckedGet<TfEnum>();
                _GetAttribute(node, attrName, v);
                settings.emplace_back(attr.key, v);
            }
        }
    }
    return ret;
}

PXR_NAMESPACE_CLOSE_SCOPE
