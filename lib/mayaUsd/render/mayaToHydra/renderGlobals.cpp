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

#include "renderOverride.h"
#include "utils.h"

#include <pxr/imaging/hd/renderDelegate.h>
#include <pxr/imaging/hd/rendererPlugin.h>
#include <pxr/imaging/hd/rendererPluginRegistry.h>
#include <pxr/pxr.h>
#include "pxr/usd/usdRender/settings.h"

#include <maya/MFnDependencyNode.h>
#include <maya/MFnEnumAttribute.h>
#include <maya/MFnNumericAttribute.h>
#include <maya/MFnStringData.h>
#include <maya/MFnTypedAttribute.h>
#include <maya/MGlobal.h>
#include <maya/MPlug.h>
#include <maya/MSelectionList.h>
#include <maya/MStatus.h>

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
    (mtohWireframeSelectionHighlight)
    (mtohColorQuantization)
    (mtohSelectionOutline)
    (mtohMotionSampleStart)
    (mtohMotionSampleEnd)
);
// clang-format on

namespace {

// Pre-amble that all option-boxes will need
//
constexpr auto _renderOverride_PreAmble = R"mel(
global proc mtohRenderOverride_ApplySetting(string $renderer, string $attr, string $node) {
    // This exists as a global function for the difference in how it is invoked from editorTemplate/AE or option-boxes
    mtoh -r $renderer -updateRenderGlobals $attr;
    refresh -f;
}
global proc mtohRenderOverride_AddAttribute(string $renderer, string $label, string $attr, int $fromAE) {
    string $command = "mtohRenderOverride_ApplySetting " + $renderer + " " + $attr;
    if (!$fromAE) {
        $command = $command + " defaultRenderGlobals";
        attrControlGrp -label $label -attribute ("defaultRenderGlobals." + $attr) -changeCommand $command;
    } else {
        editorTemplate -label $label -adc $attr $command;
    }
}
global proc mtohRenderOverride_AddMTOHAttributes(int $fromAE) {
    mtohRenderOverride_AddAttribute("mtoh", "Motion Sample Start", "mtohMotionSampleStart", $fromAE);
    mtohRenderOverride_AddAttribute("mtoh", "Motion Samples End", "mtohMotionSampleEnd", $fromAE);
    mtohRenderOverride_AddAttribute("mtoh", "Texture Memory Per Texture (KB)", "mtohTextureMemoryPerTexture", $fromAE);
    mtohRenderOverride_AddAttribute("mtoh", "Show Wireframe on Selected Objects", "mtohWireframeSelectionHighlight", $fromAE);
    mtohRenderOverride_AddAttribute("mtoh", "Highlight Selected Objects", "mtohColorSelectionHighlight", $fromAE);
    mtohRenderOverride_AddAttribute("mtoh", "Highlight Color for Selected Objects", "mtohColorSelectionHighlightColor", $fromAE);
)mel"
#if PXR_VERSION >= 2005
                                          R"mel(
    mtohRenderOverride_AddAttribute("mtoh", "Highlight outline (in pixels, 0 to disable)", "mtohSelectionOutline", $fromAE);
)mel"
#endif
#if PXR_VERSION <= 2005
                                          R"mel(
    mtohRenderOverride_AddAttribute("mtoh", "Enable color quantization", "mtohColorQuantization", $fromAE);
)mel"
#endif

                                          R"mel(
}

global proc mtohRenderOverride_AEAttributesCallback(string $nodeName) {
    if (`nodeType $nodeName` != "renderGlobals") {
        return;
    }

    editorTemplate -beginLayout "Hydra Settings" -collapse 1;
        mtohRenderOverride_AddMTOHAttributes(1);
        for ($renderer in `mtoh -lr`) {
            string $displayName = `mtoh -getRendererDisplayName -r $renderer`;
            editorTemplate -beginLayout $displayName -collapse 1;
                string $optionsCmd = "mtohRenderOverride_" + $renderer + "Options(1);";
                eval($optionsCmd);
            editorTemplate -endLayout;
        }
    editorTemplate -endLayout;
}

// Make our attributes look nice and get sent from the AttributeEditor
callbacks -o mtoh -hook AETemplateCustomContent -addCallback mtohRenderOverride_AEAttributesCallback;
)mel";

constexpr auto _renderOverrideOptionBoxTemplate = R"mel(
global proc {{override}}OptionBox() {
    string $windowName = "{{override}}OptionsWindow";
    if (`window -exists $windowName`) {
        showWindow $windowName;
        return;
    }

    // XXX: Could have an optionVar controlling -userDefaults flag
    //
    mtoh -createRenderGlobals -r "{{hydraplugin}}" -userDefaults;

    window -title "Maya to Hydra Settings" "{{override}}OptionsWindow";
    scrollLayout;
    frameLayout -label "Hydra Settings";
    columnLayout;
    mtohRenderOverride_AddMTOHAttributes(0);
    setParent ..;
    setParent ..;

    frameLayout -label "{{hydraDisplayName}}" -collapsable true;
    columnLayout;
    {{override}}Options(0);
    setParent ..;
    setParent ..;

    setParent ..;

    showWindow $windowName;
}
)mel";

static constexpr const char* kMtohNSToken = "_mtohns_";
static const std::string     kMtohRendererPostFix("__");

static MString _MangleColorAttribute(const MString& attrName, unsigned i)
{
    static const MString                kMtohCmptToken("_mtohc_");
    static const std::array<MString, 4> kColorComponents = { "R", "G", "B", "A" };
    if (i < kColorComponents.size()) {
        return attrName + kMtohCmptToken + kColorComponents[i];
    }

    TF_CODING_ERROR("[mtoh] Cannot mangle component: %u", i);
    return attrName + kMtohCmptToken + MString("INVALID");
}

static MString _AlphaAttribute(const MString& attrName)
{
    return _MangleColorAttribute(attrName, 3);
}

template <typename HydraType, typename PrefType>
bool _RestoreValue(
    MFnDependencyNode& node,
    const MString&     attrName,
    PrefType (*getter)(const MString&, bool* valid))
{
    bool     valid = false;
    PrefType mayaPref = getter(attrName, &valid);
    if (valid) {
        auto plug = node.findPlug(attrName);
        plug.setValue(HydraType(mayaPref));
    }
    return valid;
}

void _CreateEnumAttribute(
    MFnDependencyNode&   node,
    const MString&       attrName,
    const TfTokenVector& values,
    const TfToken&       defValue,
    bool                 useUserOptions)
{
    const auto attr = node.attribute(attrName);
    const bool existed = !attr.isNull();
    if (existed) {
        const auto sameOrder = [&attr, &values]() -> bool {
            MStatus          status;
            MFnEnumAttribute eAttr(attr, &status);
            if (!status) {
                return false;
            }
            short id = 0;
            for (const auto& v : values) {
                if (eAttr.fieldName(id++) != v.GetText()) {
                    return false;
                }
            }
            return true;
        };
        if (sameOrder()) {
            return;
        }

        node.removeAttribute(attr);
    }

    MFnEnumAttribute eAttr;
    auto             o = eAttr.create(attrName, attrName);
    short            id = 0;
    for (const auto& v : values) {
        eAttr.addField(v.GetText(), id++);
    }
    eAttr.setDefault(defValue.GetText());
    node.addAttribute(o);

    if (existed || !useUserOptions) {
        return;
    }

    // Enums stored as string to allow re-ordering
    // Why MPlug::setValue doesn't handle this ?
    //
    bool    valid = false;
    TfToken mayaPref(MGlobal::optionVarStringValue(attrName, &valid).asChar());
    if (!valid) {
        return;
    }

    for (int i = 0, n = values.size(); i < n; ++i) {
        if (mayaPref == values[i]) {
            auto plug = node.findPlug(attrName);
            plug.setValue(i);
            return;
        }
    }
    TF_WARN("[mtoh] Cannot restore enum '%s'", mayaPref.GetText());
}

void _CreateEnumAttribute(
    MFnDependencyNode& node,
    const MString&     attrName,
    const TfEnum&      defValue,
    bool               useUserOptions)
{
    std::vector<std::string> names = TfEnum::GetAllNames(defValue);
    TfTokenVector            tokens(names.begin(), names.end());
    return _CreateEnumAttribute(
        node, attrName, tokens, TfToken(TfEnum::GetDisplayName(defValue)), useUserOptions);
}

void _CreateStringAttribute(
    MFnDependencyNode& node,
    const MString&     attrName,
    const std::string& defValue,
    bool               useUserOptions)
{

    const auto attr = node.attribute(attrName);
    const bool existed = !attr.isNull();
    if (existed) {
        MStatus           status;
        MFnTypedAttribute tAttr(attr, &status);
        if (status && tAttr.attrType() == MFnData::kString) {
            return;
        }
        node.removeAttribute(attr);
    }

    MFnTypedAttribute tAttr;
    const auto        obj = tAttr.create(attrName, attrName, MFnData::kString);
    if (!defValue.empty()) {
        MFnStringData strData;
        MObject       defObj = strData.create(defValue.c_str());
        tAttr.setDefault(defObj);
    }
    node.addAttribute(obj);

    if (!existed && useUserOptions) {
        _RestoreValue<MString>(node, attrName, MGlobal::optionVarStringValue);
    }
}

template <typename T, typename MayaType>
void _CreateNumericAttribute(
    MFnDependencyNode&                                      node,
    const MString&                                          attrName,
    MFnNumericData::Type                                    type,
    typename std::enable_if<std::is_pod<T>::value, T>::type defValue,
    bool                                                    useUserOptions,
    MayaType (*getter)(const MString&, bool* valid),
    std::function<void(MFnNumericAttribute& nAttr)> postCreate = {})
{

    const auto attr = node.attribute(attrName);
    const bool existed = !attr.isNull();
    if (existed) {
        MStatus             status;
        MFnNumericAttribute nAttr(attr, &status);
        if (status && nAttr.unitType() == type) {
            return;
        }
        node.removeAttribute(attr);
    }

    MFnNumericAttribute nAttr;
    const auto          obj = nAttr.create(attrName, attrName, type);
    nAttr.setDefault(defValue);
    if (postCreate) {
        postCreate(nAttr);
    }
    node.addAttribute(obj);

    if (!existed && useUserOptions) {
        _RestoreValue<T, MayaType>(node, attrName, getter);
    }
}

template <typename T>
void _CreateColorAttribute(
    MFnDependencyNode&                                             node,
    const MString&                                                 attrName,
    T                                                              defValue,
    bool                                                           useUserOptions,
    std::function<bool(MFnNumericAttribute& nAttr, bool doCreate)> alphaOp = {})
{
    const auto attr = node.attribute(attrName);
    if (!attr.isNull()) {
        MStatus             status;
        MFnNumericAttribute nAttr(attr, &status);
        if (status && nAttr.isUsedAsColor() && (!alphaOp || alphaOp(nAttr, false))) {
            return;
        }
        node.removeAttribute(attr);
    }
    MFnNumericAttribute nAttr;
    const auto          o = nAttr.createColor(attrName, attrName);
    nAttr.setDefault(defValue[0], defValue[1], defValue[2]);
    node.addAttribute(o);

    if (alphaOp) {
        alphaOp(nAttr, true);
    }

    if (useUserOptions) {
        for (unsigned i = 0; i < T::dimension; ++i) {
            _RestoreValue<float>(
                node, _MangleColorAttribute(attrName, i), MGlobal::optionVarDoubleValue);
        }
    }
}

void _CreateColorAttribute(
    MFnDependencyNode& node,
    const MString&     attrName,
    GfVec4f            defVec4,
    bool               useUserOptions)
{
    _CreateColorAttribute(
        node,
        attrName,
        GfVec3f(defVec4.data()),
        useUserOptions,
        [&](MFnNumericAttribute& nAttr, bool doCreate) -> bool {
            const MString attrAName = _AlphaAttribute(attrName);
            const auto    attrA = node.attribute(attrAName);
            // If we previously found the color attribute, make sure the Alpha attribute
            // is also a match (MFnNumericData::kFloat), otherwise delete it and signal to re-create
            // the color too.
            if (!doCreate) {
                if (!attrA.isNull()) {
                    MStatus             status;
                    MFnNumericAttribute nAttr(attrA, &status);
                    if (status && nAttr.unitType() == MFnNumericData::kFloat) {
                        return true;
                    }
                    node.removeAttribute(attrA);
                }
                return false;
            }

            const auto o = nAttr.create(attrAName, attrAName, MFnNumericData::kFloat);
            nAttr.setDefault(defVec4[3]);
            node.addAttribute(o);
            return true;
        });
}

void _CreateBoolAttribute(
    MFnDependencyNode& node,
    const MString&     attrName,
    bool               defValue,
    bool               useUserOptions)
{
    _CreateNumericAttribute<bool>(
        node,
        attrName,
        MFnNumericData::kBoolean,
        defValue,
        useUserOptions,
        MGlobal::optionVarIntValue);
}

void _CreateIntAttribute(
    MFnDependencyNode&                              node,
    const MString&                                  attrName,
    int                                             defValue,
    bool                                            useUserOptions,
    std::function<void(MFnNumericAttribute& nAttr)> postCreate = {})
{
    _CreateNumericAttribute<int>(
        node,
        attrName,
        MFnNumericData::kInt,
        defValue,
        useUserOptions,
        MGlobal::optionVarIntValue,
        std::move(postCreate));
}

void _CreateFloatAttribute(
    MFnDependencyNode& node,
    const MString&     attrName,
    float              defValue,
    bool               useUserOptions)
{
    _CreateNumericAttribute<float>(
        node,
        attrName,
        MFnNumericData::kFloat,
        defValue,
        useUserOptions,
        MGlobal::optionVarDoubleValue);
}

template <typename T> void _GetFromPlug(const MPlug& plug, T& out) { assert(false); }

template <> void _GetFromPlug<bool>(const MPlug& plug, bool& out) { out = plug.asBool(); }

template <> void _GetFromPlug<int>(const MPlug& plug, int& out) { out = plug.asInt(); }

template <> void _GetFromPlug<float>(const MPlug& plug, float& out) { out = plug.asFloat(); }

template <> void _GetFromPlug<std::string>(const MPlug& plug, std::string& out)
{
    out = plug.asString().asChar();
}

template <> void _GetFromPlug<TfEnum>(const MPlug& plug, TfEnum& out)
{
    out = TfEnum(out.GetType(), plug.asInt());
}

template <> void _GetFromPlug<TfToken>(const MPlug& plug, TfToken& out)
{
	MObject attribute = plug.attribute();

	if (attribute.hasFn(MFn::kEnumAttribute))
	{
		MFnEnumAttribute enumAttr(attribute);
		MString value = enumAttr.fieldName(plug.asShort());
		out = TfToken(value.asChar());
	}
	else
	{
		out = TfToken(plug.asString().asChar());
	}
}


template <typename T> bool _SetOptionVar(const MString& attrName, const T& value)
{
    return MGlobal::setOptionVarValue(attrName, value);
}

bool _SetOptionVar(const MString& attrName, const bool& value)
{
    return _SetOptionVar(attrName, int(value));
}

bool _SetOptionVar(const MString& attrName, const float& value)
{
    return _SetOptionVar(attrName, double(value));
}

bool _SetOptionVar(const MString& attrName, const TfToken& value)
{
    return _SetOptionVar(attrName, MString(value.GetText()));
}

bool _SetOptionVar(const MString& attrName, const std::string& value)
{
    return _SetOptionVar(attrName, MString(value.c_str()));
}

bool _SetOptionVar(const MString& attrName, const TfEnum& value)
{
    return _SetOptionVar(attrName, TfEnum::GetDisplayName(value));
}

template <typename T> bool _SetColorOptionVar(const MString& attrName, const T& values)
{
    bool rval = true;
    for (size_t i = 0; i < T::dimension; ++i) {
        rval = _SetOptionVar(_MangleColorAttribute(attrName, i), values[i]) && rval;
    }
    return rval;
}

template <typename T>
bool _GetAttribute(
    const MFnDependencyNode& node,
    const MString&           attrName,
    T&                       out,
    bool                     storeUserSetting)
{
    const auto plug = node.findPlug(attrName, true);
    if (plug.isNull()) {
        return false;
    }
    _GetFromPlug<T>(plug, out);
    if (storeUserSetting) {
        _SetOptionVar(attrName, out);
    }
    return true;
}

void _GetColorAttribute(
    const MFnDependencyNode&            node,
    const MString&                      attrName,
    GfVec3f&                            out,
    bool                                storeUserSetting,
    std::function<void(GfVec3f&, bool)> alphaOp = {})
{
    const auto plug = node.findPlug(attrName, true);
    if (plug.isNull()) {
        return;
    }

    out[0] = plug.child(0).asFloat();
    out[1] = plug.child(1).asFloat();
    out[2] = plug.child(2).asFloat();
    if (alphaOp) {
        // if alphaOp is provided, it is responsible for storing the settings
        alphaOp(out, storeUserSetting);
    } else {
        _SetColorOptionVar(attrName, out);
    }
}

void _GetColorAttribute(
    const MFnDependencyNode& node,
    const MString&           attrName,
    GfVec4f&                 out,
    bool                     storeUserSetting)
{
    GfVec3f color3;
    _GetColorAttribute(
        node, attrName, color3, storeUserSetting, [&](GfVec3f& color3, bool storeUserSetting) {
            const auto plugA = node.findPlug(_AlphaAttribute(attrName), true);
            if (plugA.isNull()) {
                TF_WARN("[mtoh] No Alpha plug for GfVec4f");
                return;
            }
            out[0] = color3[0];
            out[1] = color3[1];
            out[2] = color3[2];
            out[3] = plugA.asFloat();
            if (storeUserSetting) {
                _SetColorOptionVar(attrName, out);
            }
        });
}

bool _IsSupportedAttribute(const VtValue& v)
{
    return v.IsHolding<bool>() || v.IsHolding<int>() || v.IsHolding<float>()
        || v.IsHolding<GfVec3f>() || v.IsHolding<GfVec4f>() || v.IsHolding<TfToken>()
        || v.IsHolding<std::string>() || v.IsHolding<TfEnum>();
}

TfToken _MangleString(
    const std::string& settingKey,
    const std::string& token,
    const std::string& replacement,
    std::string        str = {})
{
    std::size_t pos = 0;
    auto        delim = settingKey.find(token);
    while (delim != std::string::npos) {
        str += settingKey.substr(pos, delim - pos).c_str();
        str += replacement;
        pos = delim + token.size();
        delim = settingKey.find(token, pos);
    }
    str += settingKey.substr(pos, settingKey.size() - pos).c_str();
    return TfToken(str, TfToken::Immortal);
}

std::string _MangleRenderer(const TfToken& rendererName)
{
    return rendererName.IsEmpty() ? "" : (rendererName.GetString() + kMtohRendererPostFix);
}

TfToken _MangleString(const TfToken& settingKey, const TfToken& rendererName)
{
    return _MangleString(settingKey.GetString(), ":", kMtohNSToken, _MangleRenderer(rendererName));
}

TfToken _DeMangleString(const TfToken& settingKey, const TfToken& rendererName)
{
    assert(!rendererName.IsEmpty() && "No condition for this");
    return _MangleString(
        settingKey.GetString().substr(rendererName.size() + kMtohRendererPostFix.size()),
        kMtohNSToken,
        ":");
}

TfToken _MangleName(const TfToken& settingKey, const TfToken& rendererName = {})
{
    assert(
        rendererName.GetString().find(':') == std::string::npos
        && "Unexpected : token in plug-in name");
    return _MangleString(settingKey, rendererName);
}

} // namespace

MtohRenderGlobals::MtohRenderGlobals() { }

// Does the attribute in 'attrName' apply to the renderer
// XXX: Not the greatest check in the world, but currently no overlap in renderer-names
bool MtohRenderGlobals::AffectsRenderer(const TfToken& mangledAttr, const TfToken& rendererName)
{
    // If no explicit renderer, the setting affects them all
    if (rendererName.IsEmpty()) {
        return true;
    }
    return mangledAttr.GetString().find(_MangleRenderer(rendererName)) == 0;
}

bool MtohRenderGlobals::ApplySettings(
    HdRenderDelegate*    delegate,
    const TfToken&       rendererName,
    const TfTokenVector& attrNames) const
{
    const auto* settings = TfMapLookupPtr(_rendererSettings, rendererName);
    if (!settings) {
        return false;
    }

    bool appliedAny = false;
    if (!attrNames.empty()) {
        for (auto& mangledAttr : attrNames) {
            if (const auto* setting = TfMapLookupPtr(*settings, mangledAttr)) {
                delegate->SetRenderSetting(_DeMangleString(mangledAttr, rendererName), *setting);
                appliedAny = true;
            }
        }
    } else {
        for (auto&& setting : *settings) {
            delegate->SetRenderSetting(
                _DeMangleString(setting.first, rendererName), setting.second);
        }
        appliedAny = true;
    }

    return appliedAny;
}

void MtohRenderGlobals::OptionsPreamble()
{
    MStatus status = MGlobal::executeCommand(_renderOverride_PreAmble);
    if (status) {
        return;
    }
    TF_WARN("[mtoh] Error executing preamble:\n%s", _renderOverride_PreAmble);
}

void MtohRenderGlobals::BuildOptionsMenu(
    const MtohRendererDescription&       rendererDesc,
    const HdRenderSettingDescriptorList& rendererSettingDescriptors)
{
    // FIXME: Horribly in-effecient, 3 parse/replace calls
    //
    auto optionBoxCommand = TfStringReplace(
        _renderOverrideOptionBoxTemplate, "{{override}}", rendererDesc.overrideName.GetText());
    optionBoxCommand
        = TfStringReplace(optionBoxCommand, "{{hydraplugin}}", rendererDesc.rendererName);
    optionBoxCommand
        = TfStringReplace(optionBoxCommand, "{{hydraDisplayName}}", rendererDesc.displayName);

    auto status = MGlobal::executeCommand(optionBoxCommand.c_str());
    if (!status) {
        TF_WARN(
            "[mtoh] Error in render override option box command function: \n%s",
            status.errorString().asChar());
    }

    auto quote = [&](std::string str, const char* strb = nullptr) -> std::string {
        if (strb)
            str += strb;
        return "\"" + str + "\"";
    };

    std::stringstream ss;
    ss << "global proc " << rendererDesc.overrideName << "Options(int $fromAE) {\n";
    for (const auto& desc : rendererSettingDescriptors) {
        if (!_IsSupportedAttribute(desc.defaultValue)) {
            continue;
        }

        ss << "\tmtohRenderOverride_AddAttribute(" << quote(rendererDesc.rendererName.GetString())
           << ',' << quote(desc.name) << ','
           << quote(_MangleName(desc.key, rendererDesc.rendererName).GetString())
           << ", $fromAE);\n";
    }
    if (rendererDesc.rendererName == MtohTokens->HdStormRendererPlugin) {
        ss << "\tmtohRenderOverride_AddAttribute(" << quote(rendererDesc.rendererName.GetString())
           << ',' << quote("Maximum shadow map size") << ','
           << quote(_MangleName(MtohTokens->mtohMaximumShadowMapResolution).GetString())
           << ", $fromAE);\n";
    }
    ss << "}\n";

    const auto optionsCommand = ss.str();
    status = MGlobal::executeCommand(optionsCommand.c_str());
    if (!status) {
        TF_WARN(
            "[mtoh] Error in render delegate options function: \n%s",
            status.errorString().asChar());
    }
}

class MtohRenderGlobals::MtohSettingFilter
{
    TfToken       _attrName;
    MString       _mayaString;
    const TfToken _inFilter;
    bool          _isAttributeFilt;

public:
    MtohSettingFilter(const GlobalParams& params)
        : _inFilter(params.filter)
        , _isAttributeFilt(!(params.filterIsRenderer || _inFilter.IsEmpty()))
    {
    }

    // Create the mangled key, and convert it to a Maya string if needed
    bool operator()(const TfToken& attr, const TfToken& renderer = {})
    {
        _attrName = _MangleName(attr, renderer);
        if (attributeFilter()) {
            if (_inFilter != _attrName) {
                return false;
            }
        } else if (renderFilter()) {
            // Allow everything for all renderers through
            if (!renderer.IsEmpty() && renderer != _inFilter) {
                return false;
            }
        }
        _mayaString = MString(_attrName.GetText());
        return true;
    }
    bool attributeFilter() const { return _isAttributeFilt; }
    bool renderFilter() const { return !_isAttributeFilt && !_inFilter.IsEmpty(); }
    bool affectsRenderer(const TfToken& renderer)
    {
        // If there's no filter, then the filter DOES affect this renderer
        if (_inFilter.IsEmpty()) {
            return true;
        }
        // If it's an attribute-filter, test with mangling, otherwise the renderer-names should
        // match
        return _isAttributeFilt ? AffectsRenderer(_inFilter, renderer) : renderer == _inFilter;
    }
    const TfToken& attrName() const { return _attrName; }
    const MString& mayaString() const { return _mayaString; }
};

// TODO : MtohRenderGlobals::CreateNode && MtohRenderGlobals::GetInstance
//        are extrmely redundant in logic, with most divergance occurring
//        at the leaf operation (_CreatXXXAttr vs. _GetAttribute)
//
MObject MtohRenderGlobals::CreateAttributes(const GlobalParams& params)
{
    MSelectionList slist;
    slist.add(_tokens->defaultRenderGlobals.GetText());

    MObject mayaObject;
    if (slist.length() == 0 || !slist.getDependNode(0, mayaObject)) {
        return mayaObject;
    }

    MStatus           status;
    MFnDependencyNode node(mayaObject, &status);
    if (!status) {
        return MObject();
    }

    // This static is used to setup the default values
    //
    static const MtohRenderGlobals defGlobals;

    MtohSettingFilter filter(params);
    const bool        userDefaults = params.fallbackToUserDefaults;

    if (filter(_tokens->mtohMotionSampleStart)) {
        _CreateFloatAttribute(
            node, filter.mayaString(), defGlobals.delegateParams.motionSampleStart, userDefaults);
        if (filter.attributeFilter()) {
            return mayaObject;
        }
    }
    if (filter(_tokens->mtohMotionSampleEnd)) {
        _CreateFloatAttribute(
            node, filter.mayaString(), defGlobals.delegateParams.motionSampleEnd, userDefaults);
        if (filter.attributeFilter()) {
            return mayaObject;
        }
    }
    if (filter(_tokens->mtohTextureMemoryPerTexture)) {
        _CreateIntAttribute(
            node,
            filter.mayaString(),
            defGlobals.delegateParams.textureMemoryPerTexture / 1024,
            userDefaults,
            [](MFnNumericAttribute& nAttr) {
                nAttr.setMin(1);
                nAttr.setMax(256 * 1024);
                nAttr.setSoftMin(1 * 1024);
                nAttr.setSoftMin(16 * 1024);
            });
        if (filter.attributeFilter()) {
            return mayaObject;
        }
    }
    if (filter(MtohTokens->mtohMaximumShadowMapResolution)) {
        _CreateIntAttribute(
            node,
            filter.mayaString(),
            defGlobals.delegateParams.maximumShadowMapResolution,
            userDefaults,
            [](MFnNumericAttribute& nAttr) {
                nAttr.setMin(32);
                nAttr.setMax(8192);
            });
        if (filter.attributeFilter()) {
            return mayaObject;
        }
    }
    if (filter(_tokens->mtohWireframeSelectionHighlight)) {
        _CreateBoolAttribute(
            node, filter.mayaString(), defGlobals.wireframeSelectionHighlight, userDefaults);
        if (filter.attributeFilter()) {
            return mayaObject;
        }
    }
    if (filter(_tokens->mtohColorSelectionHighlight)) {
        _CreateBoolAttribute(
            node, filter.mayaString(), defGlobals.colorSelectionHighlight, userDefaults);
        if (filter.attributeFilter()) {
            return mayaObject;
        }
    }
    if (filter(_tokens->mtohColorSelectionHighlightColor)) {
        _CreateColorAttribute(
            node, filter.mayaString(), defGlobals.colorSelectionHighlightColor, userDefaults);
        if (filter.attributeFilter()) {
            return mayaObject;
        }
    }
#if PXR_VERSION >= 2005
    if (filter(_tokens->mtohSelectionOutline)) {
        _CreateFloatAttribute(
            node, filter.mayaString(), defGlobals.outlineSelectionWidth, userDefaults);
    }
#endif
#if PXR_VERSION <= 2005
    if (filter(_tokens->mtohColorQuantization)) {
        _CreateBoolAttribute(
            node, filter.mayaString(), defGlobals.enableColorQuantization, userDefaults);
        if (filter.attributeFilter()) {
            return mayaObject;
        }
    }
#endif
    // TODO: Move this to an external function and add support for more types,
    //  and improve code quality/reuse.
    for (const auto& rit : MtohGetRendererSettings()) {
        const auto rendererName = rit.first;
        // Skip over all the settings for this renderer if it doesn't match
        if (!filter.affectsRenderer(rendererName)) {
            continue;
        }

        for (const auto& attr : rit.second) {
            if (!filter(attr.key, rendererName)) {
                continue;
            }

            if (attr.defaultValue.IsHolding<bool>()) {
                _CreateBoolAttribute(
                    node,
                    filter.mayaString(),
                    attr.defaultValue.UncheckedGet<bool>(),
                    userDefaults);
            } else if (attr.defaultValue.IsHolding<int>()) {
                _CreateIntAttribute(
                    node, filter.mayaString(), attr.defaultValue.UncheckedGet<int>(), userDefaults);
            } else if (attr.defaultValue.IsHolding<float>()) {
                _CreateFloatAttribute(
                    node,
                    filter.mayaString(),
                    attr.defaultValue.UncheckedGet<float>(),
                    userDefaults);
            } else if (attr.defaultValue.IsHolding<GfVec3f>()) {
                _CreateColorAttribute(
                    node,
                    filter.mayaString(),
                    attr.defaultValue.UncheckedGet<GfVec3f>(),
                    userDefaults);
            } else if (attr.defaultValue.IsHolding<GfVec4f>()) {
                _CreateColorAttribute(
                    node,
                    filter.mayaString(),
                    attr.defaultValue.UncheckedGet<GfVec4f>(),
                    userDefaults);
            } else if (attr.defaultValue.IsHolding<TfToken>()) {
				// If this attribute type has AllowedTokens set, we treat it as an enum instead, so that only valid values are available.```
				bool createdAsEnum = false;
				if (auto primDef = UsdRenderSettings().GetSchemaClassPrimDefinition()) {
					VtTokenArray allowedTokens;

					if (primDef->GetPropertyMetadata(TfToken(attr.key), SdfFieldKeys->AllowedTokens, &allowedTokens)) {
						// Generate dropdown from allowedTokens
						TfTokenVector tokens(allowedTokens.begin(), allowedTokens.end());
						_CreateEnumAttribute(node, filter.mayaString(), tokens, attr.defaultValue.UncheckedGet<TfToken>(), userDefaults);
						createdAsEnum = true;
					}
				}

				if (!createdAsEnum) {
					_CreateStringAttribute(
						node,
						filter.mayaString(),
						attr.defaultValue.UncheckedGet<TfToken>().GetString(),
						userDefaults);
				}
            } else if (attr.defaultValue.IsHolding<std::string>()) {
                _CreateStringAttribute(
                    node,
                    filter.mayaString(),
                    attr.defaultValue.UncheckedGet<std::string>(),
                    userDefaults);
            } else if (attr.defaultValue.IsHolding<TfEnum>()) {
                _CreateEnumAttribute(
                    node,
                    filter.mayaString(),
                    attr.defaultValue.UncheckedGet<TfEnum>(),
                    userDefaults);
            } else {
                assert(
                    !_IsSupportedAttribute(attr.defaultValue)
                    && "_IsSupportedAttribute out of synch");

                TF_WARN(
                    "[mtoh] Ignoring setting: '%s' for %s",
                    attr.key.GetText(),
                    rendererName.GetText());
            }
            if (filter.attributeFilter()) {
                break;
            }
        }
    }
    return mayaObject;
}

const MtohRenderGlobals&
MtohRenderGlobals::GetInstance(const GlobalParams& params, bool storeUserSetting)
{
    static MtohRenderGlobals globals;
    const auto               obj = CreateAttributes(params);
    if (obj.isNull()) {
        return globals;
    }

    MStatus           status;
    MFnDependencyNode node(obj, &status);
    if (!status) {
        return globals;
    }

    MtohSettingFilter filter(params);

    if (filter(_tokens->mtohTextureMemoryPerTexture)
        && _GetAttribute(
            node,
            filter.mayaString(),
            globals.delegateParams.textureMemoryPerTexture,
            storeUserSetting)) {
        globals.delegateParams.textureMemoryPerTexture *= 1024;
        if (filter.attributeFilter()) {
            return globals;
        }
    }
    if (filter(_tokens->mtohMotionSampleStart)) {
        _GetAttribute(
            node, filter.mayaString(), globals.delegateParams.motionSampleStart, storeUserSetting);
        if (filter.attributeFilter()) {
            return globals;
        }
    }
    if (filter(_tokens->mtohMotionSampleEnd)) {
        _GetAttribute(
            node, filter.mayaString(), globals.delegateParams.motionSampleEnd, storeUserSetting);
        if (filter.attributeFilter()) {
            return globals;
        }
    }
    if (filter(MtohTokens->mtohMaximumShadowMapResolution)) {
        _GetAttribute(
            node,
            filter.mayaString(),
            globals.delegateParams.maximumShadowMapResolution,
            storeUserSetting);
        if (filter.attributeFilter()) {
            return globals;
        }
    }
    if (filter(_tokens->mtohWireframeSelectionHighlight)) {
        _GetAttribute(
            node, filter.mayaString(), globals.wireframeSelectionHighlight, storeUserSetting);
        if (filter.attributeFilter()) {
            return globals;
        }
    }
    if (filter(_tokens->mtohColorSelectionHighlight)) {
        _GetAttribute(node, filter.mayaString(), globals.colorSelectionHighlight, storeUserSetting);
        if (filter.attributeFilter()) {
            return globals;
        }
    }
    if (filter(_tokens->mtohColorSelectionHighlightColor)) {
        _GetColorAttribute(
            node, filter.mayaString(), globals.colorSelectionHighlightColor, storeUserSetting);
        if (filter.attributeFilter()) {
            return globals;
        }
    }
#if PXR_VERSION >= 2005
    if (filter(_tokens->mtohSelectionOutline)) {
        _GetAttribute(node, filter.mayaString(), globals.outlineSelectionWidth, storeUserSetting);
        if (filter.attributeFilter()) {
            return globals;
        }
    }
#endif
#if PXR_VERSION <= 2005
    if (filter(_tokens->mtohColorQuantization)) {
        _GetAttribute(node, filter.mayaString(), globals.enableColorQuantization, storeUserSetting);
        if (filter.attributeFilter()) {
            return globals;
        }
    }
#endif

    // TODO: Move this to an external function and add support for more types,
    //  and improve code quality/reuse.
    for (const auto& rit : MtohGetRendererSettings()) {
        const auto rendererName = rit.first;
        // Skip over all the settings for this renderer if it doesn't match
        if (!filter.affectsRenderer(rendererName)) {
            continue;
        }

        auto& settings = globals._rendererSettings[rendererName];
        settings.reserve(rit.second.size());
        for (const auto& attr : rit.second) {
            if (!filter(attr.key, rendererName)) {
                continue;
            }

            if (attr.defaultValue.IsHolding<bool>()) {
                auto v = attr.defaultValue.UncheckedGet<bool>();
                _GetAttribute(node, filter.mayaString(), v, storeUserSetting);
                settings[filter.attrName()] = v;
            } else if (attr.defaultValue.IsHolding<int>()) {
                auto v = attr.defaultValue.UncheckedGet<int>();
                _GetAttribute(node, filter.mayaString(), v, storeUserSetting);
                settings[filter.attrName()] = v;
            } else if (attr.defaultValue.IsHolding<float>()) {
                auto v = attr.defaultValue.UncheckedGet<float>();
                _GetAttribute(node, filter.mayaString(), v, storeUserSetting);
                settings[filter.attrName()] = v;
            } else if (attr.defaultValue.IsHolding<GfVec3f>()) {
                auto v = attr.defaultValue.UncheckedGet<GfVec3f>();
                _GetColorAttribute(node, filter.mayaString(), v, storeUserSetting);
                settings[filter.attrName()] = v;
            } else if (attr.defaultValue.IsHolding<GfVec4f>()) {
                auto v = attr.defaultValue.UncheckedGet<GfVec4f>();
                _GetColorAttribute(node, filter.mayaString(), v, storeUserSetting);
                settings[filter.attrName()] = v;
            } else if (attr.defaultValue.IsHolding<TfToken>()) {
                auto v = attr.defaultValue.UncheckedGet<TfToken>();
                _GetAttribute(node, filter.mayaString(), v, storeUserSetting);
                settings[filter.attrName()] = v;
            } else if (attr.defaultValue.IsHolding<std::string>()) {
                auto v = attr.defaultValue.UncheckedGet<std::string>();
                _GetAttribute(node, filter.mayaString(), v, storeUserSetting);
                settings[filter.attrName()] = v;
            } else if (attr.defaultValue.IsHolding<TfEnum>()) {
                auto v = attr.defaultValue.UncheckedGet<TfEnum>();
                _GetAttribute(node, filter.mayaString(), v, storeUserSetting);
                settings[filter.attrName()] = v;
            } else {
                assert(
                    !_IsSupportedAttribute(attr.defaultValue)
                    && "_IsSupportedAttribute out of synch");

                TF_WARN(
                    "[mtoh] Can't get setting: '%s' for %s",
                    attr.key.GetText(),
                    rendererName.GetText());
            }
            if (filter.attributeFilter()) {
                break;
            }
        }
    }
    return globals;
}

const MtohRenderGlobals& MtohRenderGlobals::GetInstance(bool storeUserSetting)
{
    return GetInstance(GlobalParams(), storeUserSetting);
}

const MtohRenderGlobals&
MtohRenderGlobals::GlobalChanged(const GlobalParams& params, bool storeUserSetting)
{
    return GetInstance(params, storeUserSetting);
}

PXR_NAMESPACE_CLOSE_SCOPE
