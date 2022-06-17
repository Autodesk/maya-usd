//
// Copyright 2020 Autodesk
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
#include "UsdContextOps.h"

#include "private/UfeNotifGuard.h"

#ifdef UFE_V3_FEATURES_AVAILABLE
#include <mayaUsd/commands/PullPushCommands.h>
#include <mayaUsd/fileio/primUpdaterManager.h>
#endif
#include <mayaUsd/nodes/proxyShapeLoadRules.h>
#include <mayaUsd/ufe/UsdObject3d.h>
#include <mayaUsd/ufe/UsdSceneItem.h>
#include <mayaUsd/ufe/UsdUndoAddNewPrimCommand.h>
#include <mayaUsd/ufe/Utils.h>
#include <mayaUsd/utils/util.h>

#include <pxr/base/plug/plugin.h>
#include <pxr/base/plug/registry.h>
#include <pxr/base/tf/diagnostic.h>
#include <pxr/pxr.h>
#include <pxr/usd/sdf/fileFormat.h>
#include <pxr/usd/sdf/path.h>
#include <pxr/usd/sdf/reference.h>
#include <pxr/usd/usd/common.h>
#include <pxr/usd/usd/prim.h>
#include <pxr/usd/usd/references.h>
#include <pxr/usd/usd/stage.h>
#include <pxr/usd/usd/variantSets.h>
#include <pxr/usd/usdGeom/tokens.h>
#include <pxr/usd/usdShade/material.h>

#include <maya/MGlobal.h>
#include <ufe/attribute.h>
#include <ufe/attributes.h>
#include <ufe/globalSelection.h>
#include <ufe/object3d.h>
#include <ufe/observableSelection.h>
#include <ufe/path.h>
#include <ufe/pathString.h>

#include <algorithm>
#include <cassert>
#include <utility>
#include <vector>

PXR_NAMESPACE_USING_DIRECTIVE

namespace {

// Ufe::ContextItem strings
// - the "Item" describe the operation to be performed.
// - the "Label" is used in the context menu (can be localized).
// - the "Image" is used for icon in the context menu. Directly used std::string
//   for these so the emplace_back() will choose the right constructor. With char[]
//   it would convert that param to a bool and choose the wrong constructor.
#ifdef WANT_QT_BUILD
static constexpr char kUSDLayerEditorItem[] = "USD Layer Editor";
static constexpr char kUSDLayerEditorLabel[] = "USD Layer Editor...";
#endif
static const std::string kUSDLayerEditorImage { "USD_generic.png" };
static constexpr char    kUSDLoadItem[] = "Load";
static constexpr char    kUSDLoadLabel[] = "Load";
static constexpr char    kUSDLoadWithDescendantsItem[] = "Load with Descendants";
static constexpr char    kUSDLoadWithDescendantsLabel[] = "Load with Descendants";
static constexpr char    kUSDUnloadItem[] = "Unload";
static constexpr char    kUSDUnloadLabel[] = "Unload";
static constexpr char    kUSDVariantSetsItem[] = "Variant Sets";
static constexpr char    kUSDVariantSetsLabel[] = "Variant Sets";
static constexpr char    kUSDToggleVisibilityItem[] = "Toggle Visibility";
static constexpr char    kUSDMakeVisibleLabel[] = "Make Visible";
static constexpr char    kUSDMakeInvisibleLabel[] = "Make Invisible";
static constexpr char    kUSDToggleActiveStateItem[] = "Toggle Active State";
static constexpr char    kUSDActivatePrimLabel[] = "Activate Prim";
static constexpr char    kUSDDeactivatePrimLabel[] = "Deactivate Prim";
static constexpr char    kUSDToggleInstanceableStateItem[] = "Toggle Instanceable State";
static constexpr char    kUSDMarkAsInstancebaleLabel[] = "Mark as Instanceable";
static constexpr char    kUSDUnmarkAsInstanceableLabel[] = "Unmark as Instanceable";
static constexpr char    kUSDAddNewPrimItem[] = "Add New Prim";
static constexpr char    kUSDAddNewPrimLabel[] = "Add New Prim";
static constexpr char    kUSDDefPrimItem[] = "Def";
static constexpr char    kUSDDefPrimLabel[] = "Def";
static const std::string kUSDDefPrimImage { "out_USD_Def.png" };
static constexpr char    kUSDScopePrimItem[] = "Scope";
static constexpr char    kUSDScopePrimLabel[] = "Scope";
static const std::string kUSDScopePrimImage { "out_USD_Scope.png" };
static constexpr char    kUSDXformPrimItem[] = "Xform";
static constexpr char    kUSDXformPrimLabel[] = "Xform";
static const std::string kUSDXformPrimImage { "out_USD_UsdGeomXformable.png" };
static constexpr char    kUSDCapsulePrimItem[] = "Capsule";
static constexpr char    kUSDCapsulePrimLabel[] = "Capsule";
static const std::string kUSDCapsulePrimImage { "out_USD_Capsule.png" };
static constexpr char    kUSDConePrimItem[] = "Cone";
static constexpr char    kUSDConePrimLabel[] = "Cone";
static const std::string kUSDConePrimImage { "out_USD_Cone.png" };
static constexpr char    kUSDCubePrimItem[] = "Cube";
static constexpr char    kUSDCubePrimLabel[] = "Cube";
static const std::string kUSDCubePrimImage { "out_USD_Cube.png" };
static constexpr char    kUSDCylinderPrimItem[] = "Cylinder";
static constexpr char    kUSDCylinderPrimLabel[] = "Cylinder";
static const std::string kUSDCylinderPrimImage { "out_USD_Cylinder.png" };
static constexpr char    kUSDSpherePrimItem[] = "Sphere";
static constexpr char    kUSDSpherePrimLabel[] = "Sphere";
static const std::string kUSDSpherePrimImage { "out_USD_Sphere.png" };
#ifdef UFE_V3_FEATURES_AVAILABLE
static constexpr char    kEditAsMayaItem[] = "Edit As Maya Data";
static constexpr char    kEditAsMayaLabel[] = "Edit As Maya Data";
static const std::string kEditAsMayaImage { "edit_as_Maya.png" };
static constexpr char    kDuplicateAsMayaItem[] = "Duplicate As Maya Data";
static constexpr char    kDuplicateAsMayaLabel[] = "Duplicate As Maya Data";
static constexpr char    kAddMayaReferenceItem[] = "Add Maya Reference";
static constexpr char    kAddMayaReferenceLabel[] = "Add Maya Reference...";
#endif
#if PXR_VERSION >= 2108
static constexpr char kBindMaterialToSelectionItem[] = "Assign Material to Selection";
static constexpr char kBindMaterialToSelectionLabel[] = "Assign Material to Selection";
#endif

#if PXR_VERSION >= 2008
static constexpr char kAllRegisteredTypesItem[] = "All Registered";
static constexpr char kAllRegisteredTypesLabel[] = "All Registered";

// Grouping and name mapping for registered schema plugins
static const std::vector<std::string> kSchemaPluginNames = {
    "usdGeom",
    "usdLux",
    "mayaUsd_Schemas",
    "usdMedia",
    "usdRender",
    "usdRi",
    "usdShade",
    "usdSkel",
    "usdUI",
    "usdVol",
    "AL_USDMayaSchemasTest",
    "AL_USDMayaSchemas",
};
// clang-format off
static const std::vector<std::string> kSchemaNiceNames = {
    "Geometry",
    "Lighting",
    "Maya Reference",
    "Media",
    "Render",
    "RenderMan",
    "Shading",
    "Skeleton",
    "UI",
    "Volumes",
    "", // Skip legacy AL schemas
    "", // Skip legacy AL schemas
};
// clang-format on
#endif

//! \brief Change the cursor to wait state on construction and restore it on destruction.
struct WaitCursor
{
    WaitCursor() { MGlobal::executeCommand("waitCursor -state 1"); }
    ~WaitCursor() { MGlobal::executeCommand("waitCursor -state 0"); }
};

//! \brief Undoable command for loading a USD prim.
class LoadUnloadBaseUndoableCommand : public Ufe::UndoableCommand
{
public:
    LoadUnloadBaseUndoableCommand(const UsdPrim& prim)
        : _stage(prim.GetStage())
        , _primPath(prim.GetPath())
        , _oldLoadRules(prim.GetStage()->GetLoadRules())
    {
    }

    void undo() override
    {
        if (!_stage) {
            return;
        }

        _stage->SetLoadRules(_oldLoadRules);
        saveModifiedLoadRules();
    }

protected:
    void saveModifiedLoadRules()
    {
        // Save the load rules so that switching the stage settings will be able to preserve the
        // load rules.
        MAYAUSD_NS::MayaUsdProxyShapeLoadRules::saveLoadRules(_stage);
    }

    const UsdStageWeakPtr   _stage;
    const SdfPath           _primPath;
    const UsdStageLoadRules _oldLoadRules;
};

//! \brief Undoable command for loading a USD prim.
class LoadUndoableCommand : public LoadUnloadBaseUndoableCommand
{
public:
    LoadUndoableCommand(const UsdPrim& prim, UsdLoadPolicy policy)
        : LoadUnloadBaseUndoableCommand(prim)
        , _policy(policy)
    {
    }

    void redo() override
    {
        if (!_stage) {
            return;
        }

        _stage->Load(_primPath, _policy);
        saveModifiedLoadRules();
    }

private:
    const UsdLoadPolicy _policy;
};

//! \brief Undoable command for unloading a USD prim.
class UnloadUndoableCommand : public LoadUnloadBaseUndoableCommand
{
public:
    UnloadUndoableCommand(const UsdPrim& prim)
        : LoadUnloadBaseUndoableCommand(prim)
    {
    }

    void redo() override
    {
        if (!_stage) {
            return;
        }

        _stage->Unload(_primPath);
        saveModifiedLoadRules();
    }
};

//! \brief Undoable command for variant selection change
class SetVariantSelectionUndoableCommand : public Ufe::UndoableCommand
{
public:
    SetVariantSelectionUndoableCommand(
        const Ufe::Path&                 path,
        const UsdPrim&                   prim,
        const Ufe::ContextOps::ItemPath& itemPath)
        : _path(path)
        , _varSet(prim.GetVariantSets().GetVariantSet(itemPath[1]))
        , _oldSelection(_varSet.GetVariantSelection())
        , _newSelection(itemPath[2])
    {
    }

    void undo() override
    {
        _varSet.SetVariantSelection(_oldSelection);
        // Restore the saved selection to the global selection.  If a saved
        // selection item started with the prim's path, re-create it.
        auto globalSn = Ufe::GlobalSelection::get();
        globalSn->replaceWith(MayaUsd::ufe::recreateDescendants(_savedSn, _path));
    }

    void redo() override
    {
        // Make a copy of the global selection, to restore it on unmute.
        auto globalSn = Ufe::GlobalSelection::get();
        _savedSn.replaceWith(*globalSn);
        // Filter the global selection, removing items below our prim.
        globalSn->replaceWith(MayaUsd::ufe::removeDescendants(_savedSn, _path));
        _varSet.SetVariantSelection(_newSelection);
    }

private:
    const Ufe::Path   _path;
    UsdVariantSet     _varSet;
    const std::string _oldSelection;
    const std::string _newSelection;
    Ufe::Selection    _savedSn; // For global selection save and restore.
};

//! \brief Undoable command for prim active state change
class ToggleActiveStateCommand : public Ufe::UndoableCommand
{
public:
    ToggleActiveStateCommand(const UsdPrim& prim)
    {
        _stage = prim.GetStage();
        _primPath = prim.GetPath();
        _active = prim.IsActive();
    }

    void undo() override
    {
        if (_stage) {
            UsdPrim prim = _stage->GetPrimAtPath(_primPath);
            if (prim.IsValid()) {
                MayaUsd::ufe::InAddOrDeleteOperation ad;
                prim.SetActive(_active);
            }
        }
    }

    void redo() override
    {
        if (_stage) {
            UsdPrim prim = _stage->GetPrimAtPath(_primPath);
            if (prim.IsValid()) {
                MayaUsd::ufe::InAddOrDeleteOperation ad;
                prim.SetActive(!_active);
            }
        }
    }

private:
    PXR_NS::UsdStageWeakPtr _stage;
    PXR_NS::SdfPath         _primPath;
    bool                    _active;
};

//! \brief Undoable command for prim instanceable state change
class ToggleInstanceableStateCommand : public Ufe::UndoableCommand
{
public:
    ToggleInstanceableStateCommand(const UsdPrim& prim)
    {
        _stage = prim.GetStage();
        _primPath = prim.GetPath();
        _instanceable = prim.IsInstanceable();
    }

    void undo() override
    {
        if (_stage) {
            UsdPrim prim = _stage->GetPrimAtPath(_primPath);
            if (prim.IsValid()) {
                prim.SetInstanceable(_instanceable);
            }
        }
    }

    void redo() override
    {
        if (_stage) {
            UsdPrim prim = _stage->GetPrimAtPath(_primPath);
            if (prim.IsValid()) {
                prim.SetInstanceable(!_instanceable);
            }
        }
    }

private:
    PXR_NS::UsdStageWeakPtr _stage;
    PXR_NS::SdfPath         _primPath;
    bool                    _instanceable;
};

const char* selectUSDFileScriptPre = R"mel(
global proc string SelectUSDFileForAddReference()
{
    string $result[] = `fileDialog2
        -fileMode 1
        -caption "Add Reference to USD Prim"
        -fileFilter "USD Files )mel";

const char* selectUSDFileScriptPost = R"mel("`;

    if (0 == size($result))
        return "";
    else
        return $result[0];
}
SelectUSDFileForAddReference();
)mel";

// Ask SDF for all supported extensions:
const char* _selectUSDFileScript()
{

    static std::string commandString;

    if (commandString.empty()) {
        // This is an interactive call from the main UI thread. No need for SMP protections.
        commandString = selectUSDFileScriptPre;

        std::string usdUiString = "(";
        std::string usdSelector = "";
        std::string otherUiString = "";
        std::string otherSelector = "";

        for (auto&& extension : SdfFileFormat::FindAllFileFormatExtensions()) {
            // Put USD first
            if (extension.rfind("usd", 0) == 0) {
                if (!usdSelector.empty()) {
                    usdUiString += " ";
                }
                usdUiString += "*." + extension;
                usdSelector += ";;*." + extension;
            } else {
                otherUiString += " *." + extension;
                otherSelector += ";;*." + extension;
            }
        }
        commandString += usdUiString + otherUiString + ")" + usdSelector + otherSelector
            + selectUSDFileScriptPost;
    }
    return commandString.c_str();
}

const char* clearAllReferencesConfirmScript = R"(
global proc string ClearAllUSDReferencesConfirm()
{
    return `confirmDialog -title "Remove All References"
        -message "Removing all references from USD prim.  Are you sure?"
        -button "Yes" -button "No" -defaultButton "Yes"
        -cancelButton "No" -dismissString "No"`;

}
ClearAllUSDReferencesConfirm();
)";

class AddUsdReferenceUndoableCommand : public Ufe::UndoableCommand
{
public:
    static const std::string commandName;

    AddUsdReferenceUndoableCommand(const UsdPrim& prim, const std::string& filePath)
        : _prim(prim)
        , _sdfRef()
        , _filePath(filePath)
    {
    }

    void undo() override
    {
        if (_prim.IsValid()) {
            UsdReferences primRefs = _prim.GetReferences();
            primRefs.RemoveReference(_sdfRef);
        }
    }

    void redo() override
    {
        if (_prim.IsValid()) {
            if (TfStringEndsWith(_filePath, ".mtlx")) {
                _sdfRef = SdfReference(_filePath, SdfPath("/MaterialX"));
            } else {
                _sdfRef = SdfReference(_filePath);
            }
            UsdReferences primRefs = _prim.GetReferences();
            primRefs.AddReference(_sdfRef);
        }
    }

private:
    UsdPrim           _prim;
    SdfReference      _sdfRef;
    const std::string _filePath;
};
const std::string AddUsdReferenceUndoableCommand::commandName("Add USD Reference...");

class ClearAllReferencesUndoableCommand : public Ufe::UndoableCommand
{
public:
    static const std::string commandName;
    static const MString     cancelRemoval;

    ClearAllReferencesUndoableCommand(const UsdPrim& prim)
        : _prim(prim)
    {
    }

    void undo() override { }

    void redo() override
    {
        if (_prim.IsValid()) {
            UsdReferences primRefs = _prim.GetReferences();
            primRefs.ClearReferences();
        }
    }

private:
    UsdPrim _prim;
};
const std::string ClearAllReferencesUndoableCommand::commandName("Clear All References");
const MString     ClearAllReferencesUndoableCommand::cancelRemoval("No");

#if PXR_VERSION >= 2108
class BindMaterialUndoableCommand : public Ufe::UndoableCommand
{
public:
    static const std::string commandName;

    static UsdPrim CompatiblePrim(const Ufe::SceneItem::Ptr& item)
    {
        auto usdItem = std::dynamic_pointer_cast<const MAYAUSD_NS::ufe::UsdSceneItem>(item);
        if (!usdItem) {
            return {};
        }
        UsdPrim usdPrim = usdItem->prim();
        if (UsdShadeNodeGraph(usdPrim) || UsdShadeShader(usdPrim)) {
            // The binding schema can be applied anywhere, but it makes no sense on a
            // material or a shader.
            return {};
        }
        if (PXR_NS::UsdShadeMaterialBindingAPI::CanApply(usdPrim)) {
            return usdPrim;
        }
        return {};
    }

    BindMaterialUndoableCommand(const UsdPrim& prim, const SdfPath& materialPath)
        : _stage(prim.GetStage())
        , _primPath(prim.GetPath())
        , _materialPath(materialPath)
    {
    }

    void undo() override
    {
        if (!_stage || _primPath.IsEmpty() || _materialPath.IsEmpty()) {
            return;
        }

        UsdPrim prim = _stage->GetPrimAtPath(_primPath);
        if (prim.IsValid()) {
            auto bindingAPI = UsdShadeMaterialBindingAPI(prim);
            if (bindingAPI) {
                if (_previousMaterialPath.IsEmpty()) {
                    bindingAPI.UnbindDirectBinding();
                } else {
                    UsdShadeMaterial material(_stage->GetPrimAtPath(_previousMaterialPath));
                    bindingAPI.Bind(material);
                }
            }
            if (_appliedBindingAPI) {
                prim.RemoveAPI<UsdShadeMaterialBindingAPI>();
            }
        }
    }

    void redo() override
    {
        if (!_stage || _primPath.IsEmpty() || _materialPath.IsEmpty()) {
            return;
        }

        UsdPrim          prim = _stage->GetPrimAtPath(_primPath);
        UsdShadeMaterial material(_stage->GetPrimAtPath(_materialPath));
        if (prim.IsValid() && material) {
            UsdShadeMaterialBindingAPI bindingAPI;
            if (prim.HasAPI<UsdShadeMaterialBindingAPI>()) {
                bindingAPI = UsdShadeMaterialBindingAPI(prim);
                _previousMaterialPath = bindingAPI.GetDirectBinding().GetMaterialPath();
            } else {
                bindingAPI = UsdShadeMaterialBindingAPI::Apply(prim);
                _appliedBindingAPI = true;
            }
            bindingAPI.Bind(material);
        }
    }

private:
    UsdStageWeakPtr _stage;
    SdfPath         _primPath;
    SdfPath         _materialPath;
    SdfPath         _previousMaterialPath;
    bool            _appliedBindingAPI = false;
};
const std::string BindMaterialUndoableCommand::commandName("Bind Material");

class UnbindMaterialUndoableCommand : public Ufe::UndoableCommand
{
public:
    static const std::string commandName;

    UnbindMaterialUndoableCommand(const UsdPrim& prim)
        : _stage(prim.GetStage())
        , _primPath(prim.GetPath())
    {
    }

    void undo() override
    {
        if (!_stage || _primPath.IsEmpty() || _materialPath.IsEmpty()) {
            return;
        }

        UsdPrim          prim = _stage->GetPrimAtPath(_primPath);
        UsdShadeMaterial material(_stage->GetPrimAtPath(_materialPath));
        if (prim.IsValid() && material) {
            // BindingAPI is still there since we did not remove it.
            auto             bindingAPI = UsdShadeMaterialBindingAPI(prim);
            UsdShadeMaterial material(_stage->GetPrimAtPath(_materialPath));
            if (bindingAPI && material) {
                bindingAPI.Bind(material);
            }
        }
    }

    void redo() override
    {
        if (!_stage || _primPath.IsEmpty()) {
            return;
        }

        UsdPrim prim = _stage->GetPrimAtPath(_primPath);
        if (prim.IsValid()) {
            auto bindingAPI = UsdShadeMaterialBindingAPI(prim);
            if (bindingAPI) {
                auto materialBinding = bindingAPI.GetDirectBinding();
                _materialPath = materialBinding.GetMaterialPath();
                if (!_materialPath.IsEmpty()) {
                    bindingAPI.UnbindDirectBinding();
                    // TODO: Can we remove the BindingAPI at this point?
                    //       Not easy to know for sure.
                }
            }
        }
    }

private:
    UsdStageWeakPtr _stage;
    SdfPath         _primPath;
    SdfPath         _materialPath;
};
const std::string UnbindMaterialUndoableCommand::commandName("Unbind Material");
#endif

std::vector<std::pair<const char* const, const char* const>>
_computeLoadAndUnloadItems(const UsdPrim& prim)
{
    std::vector<std::pair<const char* const, const char* const>> itemLabelPairs;

    const bool isInPrototype =
#if PXR_VERSION >= 2011
        prim.IsInPrototype();
#else
        prim.IsInMaster();
#endif

    if (!prim.IsActive() || isInPrototype) {
        return itemLabelPairs;
    }

    UsdStageWeakPtr  stage = prim.GetStage();
    const SdfPathSet stageLoadSet = stage->GetLoadSet();
    const SdfPathSet loadableSet = stage->FindLoadable(prim.GetPath());

    // Intersect the set of what *can* be loaded at or below this prim path
    // with the set of of what *is* loaded on the stage. The resulting set will
    // contain all paths that are loaded at or below this prim path.
    SdfPathSet loadedSet;
    std::set_intersection(
        loadableSet.cbegin(),
        loadableSet.cend(),
        stageLoadSet.cbegin(),
        stageLoadSet.cend(),
        std::inserter(loadedSet, loadedSet.end()));

    // Subtract the set of what *is* loaded on the stage from the set of what
    // *can* be loaded at or below this prim path. The resulting set will
    // contain all paths that are loadable, but not currently loaded, at or
    // below this prim path.
    SdfPathSet unloadedSet;
    std::set_difference(
        loadableSet.cbegin(),
        loadableSet.cend(),
        stageLoadSet.cbegin(),
        stageLoadSet.cend(),
        std::inserter(unloadedSet, unloadedSet.end()));

    if (!unloadedSet.empty()) {
        // Loading without descendants is only meaningful for context ops when
        // the current prim has an unloaded payload.
        if (prim.HasPayload() && !prim.IsLoaded()) {
            itemLabelPairs.emplace_back(std::make_pair(kUSDLoadItem, kUSDLoadLabel));
        }

        // We always add an item for loading with descendants when there are
        // unloaded paths at or below the current prim, since we may be in one
        // of the following situations:
        // - The current prim has a payload that is unloaded, and we don't know
        //   whether loading it will introduce more payloads in descendants, so
        //   we offer the choice to also load those or not.
        // - The current prim has a payload that is loaded, so there must be
        //   paths below it that are still unloaded.
        // - The current prim does not have a payload, so there must be paths
        //   below it that are still unloaded.
        itemLabelPairs.emplace_back(
            std::make_pair(kUSDLoadWithDescendantsItem, kUSDLoadWithDescendantsLabel));
    }

    // If anything is loaded at this prim path or any of its descendants, add
    // an item for unload.
    if (!loadedSet.empty()) {
        itemLabelPairs.emplace_back(std::make_pair(kUSDUnloadItem, kUSDUnloadLabel));
    }

    return itemLabelPairs;
}
#if PXR_VERSION >= 2008
//! \brief Get groups of concrete schema prim types to list dynamically in the UI
static const std::vector<MayaUsd::ufe::SchemaTypeGroup> getConcretePrimTypes(bool sorted)
{
    std::vector<MayaUsd::ufe::SchemaTypeGroup> groups;

    // Query all the available types
    PlugRegistry&    plugReg = PlugRegistry::GetInstance();
    std::set<TfType> schemaTypes;
    plugReg.GetAllDerivedTypes<UsdSchemaBase>(&schemaTypes);

    UsdSchemaRegistry& schemaReg = UsdSchemaRegistry::GetInstance();
    for (auto t : schemaTypes) {
        if (!schemaReg.IsConcrete(t)) {
            continue;
        }

        auto plugin = plugReg.GetPluginForType(t);
        if (!plugin) {
            continue;
        }

        // For every plugin we check if there's a nice name registered and use that instead
        auto plugin_name = plugin->GetName();
        auto name_itr
            = std::find(kSchemaPluginNames.begin(), kSchemaPluginNames.end(), plugin_name);
        if (name_itr != kSchemaPluginNames.end()) {
            plugin_name = kSchemaNiceNames[name_itr - kSchemaPluginNames.begin()];
        }

        // We don't list empty names. This allows hiding certain plugins too.
        if (plugin_name.empty()) {
            continue;
        }

        auto type_name = UsdSchemaRegistry::GetConcreteSchemaTypeName(t);

        // Find or create the schema group and add to it
        auto group_itr = find(begin(groups), end(groups), plugin_name);
        if (group_itr == groups.end()) {
            MayaUsd::ufe::SchemaTypeGroup group { plugin_name };
            group._types.emplace_back(type_name);
            groups.emplace_back(group);
        } else {
            groups[group_itr - groups.begin()]._types.emplace_back(type_name);
        }
    }

    if (sorted) {
        for (size_t i = 0; i < groups.size(); ++i) {
            auto group = groups[i];
            std::sort(group._types.begin(), group._types.end());
            groups[i] = group;
        }

        std::sort(groups.begin(), groups.end(), [](const auto& lhs, const auto& rhs) {
            return lhs._name < rhs._name;
        });
    }

    return groups;
}
#endif

} // namespace

namespace MAYAUSD_NS_DEF {
namespace ufe {

#if PXR_VERSION >= 2008
std::vector<SchemaTypeGroup> UsdContextOps::schemaTypeGroups = {};
#endif

UsdContextOps::UsdContextOps(const UsdSceneItem::Ptr& item)
    : Ufe::ContextOps()
    , fItem(item)
{
}

UsdContextOps::~UsdContextOps() { }

/*static*/
UsdContextOps::Ptr UsdContextOps::create(const UsdSceneItem::Ptr& item)
{
    return std::make_shared<UsdContextOps>(item);
}

void UsdContextOps::setItem(const UsdSceneItem::Ptr& item) { fItem = item; }

const Ufe::Path& UsdContextOps::path() const { return fItem->path(); }

//------------------------------------------------------------------------------
// Ufe::ContextOps overrides
//------------------------------------------------------------------------------

Ufe::SceneItem::Ptr UsdContextOps::sceneItem() const { return fItem; }

Ufe::ContextOps::Items UsdContextOps::getItems(const Ufe::ContextOps::ItemPath& itemPath) const
{
    Ufe::ContextOps::Items items;
    if (itemPath.empty()) {
#if PXR_VERSION >= 2108
        if (fItem->prim().IsA<UsdShadeMaterial>()) {
            items.emplace_back(kBindMaterialToSelectionItem, kBindMaterialToSelectionLabel);
            bool enable = false;
            if (auto globalSn = Ufe::GlobalSelection::get()) {
                for (auto&& selItem : *globalSn) {
                    if (BindMaterialUndoableCommand::CompatiblePrim(selItem)) {
                        enable = true;
                        break;
                    }
                }
            }
            // I did not see any support for disabling items in Maya Ufe outliner code, but let's
            // still mark it as disabled if nothing is selected or if the selection does not contain
            // imageable primitives.
            items.back().enabled = enable;
            items.emplace_back(Ufe::ContextItem::kSeparator);
        }
#endif
#ifdef WANT_QT_BUILD
        // Top-level item - USD Layer editor (for all context op types).
        // Only available when building with Qt enabled.
        items.emplace_back(kUSDLayerEditorItem, kUSDLayerEditorLabel, kUSDLayerEditorImage);
#endif

#ifdef UFE_V3_FEATURES_AVAILABLE
        if (!fIsAGatewayType && PrimUpdaterManager::getInstance().canEditAsMaya(path())) {
            items.emplace_back(kEditAsMayaItem, kEditAsMayaLabel, kEditAsMayaImage);
            items.emplace_back(kDuplicateAsMayaItem, kDuplicateAsMayaLabel);
        }
        if (prim().GetTypeName() != TfToken("MayaReference")) {
            items.emplace_back(kAddMayaReferenceItem, kAddMayaReferenceLabel);
        }
        items.emplace_back(Ufe::ContextItem::kSeparator);
#endif

        if (!fIsAGatewayType) {
            // Working set management (load and unload):
            const auto itemLabelPairs = _computeLoadAndUnloadItems(prim());
            for (const auto& itemLabelPair : itemLabelPairs) {
                items.emplace_back(itemLabelPair.first, itemLabelPair.second);
            }

            // Variant sets:
            if (prim().HasVariantSets()) {
                items.emplace_back(
                    kUSDVariantSetsItem, kUSDVariantSetsLabel, Ufe::ContextItem::kHasChildren);
            }
            // Visibility:
            // If the item has a visibility attribute, add menu item to change visibility.
            // Note: certain prim types such as shaders & materials don't support visibility.
            auto attributes = Ufe::Attributes::attributes(sceneItem());
            if (attributes && attributes->hasAttribute(UsdGeomTokens->visibility)) {
                auto visibility = std::dynamic_pointer_cast<Ufe::AttributeEnumString>(
                    attributes->attribute(UsdGeomTokens->visibility));
                if (visibility) {
                    auto              current = visibility->get();
                    const std::string l = (current == UsdGeomTokens->invisible)
                        ? std::string(kUSDMakeVisibleLabel)
                        : std::string(kUSDMakeInvisibleLabel);
                    items.emplace_back(kUSDToggleVisibilityItem, l);
                }
            }
            // Prim active state:
            items.emplace_back(
                kUSDToggleActiveStateItem,
                prim().IsActive() ? kUSDDeactivatePrimLabel : kUSDActivatePrimLabel);

            // Instanceable:
            items.emplace_back(
                kUSDToggleInstanceableStateItem,
                prim().IsInstanceable() ? kUSDUnmarkAsInstanceableLabel
                                        : kUSDMarkAsInstancebaleLabel);
        } // !fIsAGatewayType

        // Top level item - Add New Prim (for all context op types).
        items.emplace_back(kUSDAddNewPrimItem, kUSDAddNewPrimLabel, Ufe::ContextItem::kHasChildren);

#if PXR_VERSION >= 2108
        // Top level item - Bind/unbind existing materials
        if (PXR_NS::UsdShadeMaterialBindingAPI::CanApply(fItem->prim())) {
            // Show bind menu if there is at least one bindable material in the stage.
            //
            // TODO: Show only materials that are inside of the asset's namespace otherwise there
            //       will be "refers to a path outside the scope" errors. See
            //       https://groups.google.com/g/usd-interest/c/dmjV5bQBKIo/m/LeozZ3k6BAAJ
            //       This might help restrict the stage traversal scope and improve performance.
            //
            // For completeness, and to point out that material assignments are complex:
            //
            // TODO: Introduce the "rendering purpose" concept
            // TODO: Introduce material binding via collections API
            //
            // Find materials in the global selection. Either directly selected or a direct child of
            // the selection. This way we limit how many items we traverse in search of something to
            // bind.
            bool foundMaterialItem = false;
            if (auto globalSn = Ufe::GlobalSelection::get()) {
                for (auto&& selItem : *globalSn) {
                    UsdSceneItem::Ptr usdItem = std::dynamic_pointer_cast<UsdSceneItem>(selItem);
                    if (!usdItem) {
                        continue;
                    }
                    UsdShadeMaterial material(usdItem->prim());
                    if (material) {
                        foundMaterialItem = true;
                        break;
                    }
                    for (auto&& usdChild : usdItem->prim().GetChildren()) {
                        UsdShadeMaterial material(usdChild);
                        if (material) {
                            foundMaterialItem = true;
                            break;
                        }
                    }
                    if (foundMaterialItem) {
                        break;
                    }
                }
                if (foundMaterialItem) {
                    items.emplace_back(
                        BindMaterialUndoableCommand::commandName,
                        BindMaterialUndoableCommand::commandName,
                        Ufe::ContextItem::kHasChildren);
                }
            }
        }
        if (fItem->prim().HasAPI<UsdShadeMaterialBindingAPI>()) {
            UsdShadeMaterialBindingAPI bindingAPI(fItem->prim());
            // Show unbind menu item if there is a direct binding relationship:
            auto directBinding = bindingAPI.GetDirectBinding();
            if (directBinding.GetMaterial()) {
                items.emplace_back(
                    UnbindMaterialUndoableCommand::commandName,
                    UnbindMaterialUndoableCommand::commandName);
            }
        }
#endif

        if (!fIsAGatewayType) {
            items.emplace_back(
                AddUsdReferenceUndoableCommand::commandName,
                AddUsdReferenceUndoableCommand::commandName);
            items.emplace_back(
                ClearAllReferencesUndoableCommand::commandName,
                ClearAllReferencesUndoableCommand::commandName);
        }

    } else {
        if (itemPath[0] == kUSDVariantSetsItem) {
            UsdVariantSets           varSets = prim().GetVariantSets();
            std::vector<std::string> varSetsNames;
            varSets.GetNames(&varSetsNames);

            if (itemPath.size() == 1u) {
                // Variant sets list.
                for (auto i = varSetsNames.crbegin(); i != varSetsNames.crend(); ++i) {
                    items.emplace_back(*i, *i, Ufe::ContextItem::kHasChildren);
                }
            } else {
                // Variants of a given variant set.  Second item in the path is
                // the variant set name.
                assert(itemPath.size() == 2u);

                UsdVariantSet varSet = varSets.GetVariantSet(itemPath[1]);
                auto          selected = varSet.GetVariantSelection();

                const auto varNames = varSet.GetVariantNames();
                for (const auto& vn : varNames) {
                    const bool checked(vn == selected);
                    items.emplace_back(
                        vn,
                        vn,
                        Ufe::ContextItem::kNoChildren,
                        Ufe::ContextItem::kCheckable,
                        checked,
                        Ufe::ContextItem::kExclusive);
                }
            } // Variants of a variant set
        }     // Variant sets
        else if (itemPath[0] == kUSDAddNewPrimItem) {
            if (itemPath.size() == 1u) { // Root setup
                items.emplace_back(
                    kUSDDefPrimItem, kUSDDefPrimLabel, kUSDDefPrimImage); // typeless prim
                items.emplace_back(kUSDScopePrimItem, kUSDScopePrimLabel, kUSDScopePrimImage);
                items.emplace_back(kUSDXformPrimItem, kUSDXformPrimLabel, kUSDXformPrimImage);
                items.emplace_back(Ufe::ContextItem::kSeparator);
                items.emplace_back(kUSDCapsulePrimItem, kUSDCapsulePrimLabel, kUSDCapsulePrimImage);
                items.emplace_back(kUSDConePrimItem, kUSDConePrimLabel, kUSDConePrimImage);
                items.emplace_back(kUSDCubePrimItem, kUSDCubePrimLabel, kUSDCubePrimImage);
                items.emplace_back(
                    kUSDCylinderPrimItem, kUSDCylinderPrimLabel, kUSDCylinderPrimImage);
                items.emplace_back(kUSDSpherePrimItem, kUSDSpherePrimLabel, kUSDSpherePrimImage);
#if PXR_VERSION >= 2008
                items.emplace_back(Ufe::ContextItem::kSeparator);
                items.emplace_back(
                    kAllRegisteredTypesItem,
                    kAllRegisteredTypesLabel,
                    Ufe::ContextItem::kHasChildren);
            } else if (itemPath.size() == 2u) { // Sub Menus
                if (itemPath[1] == kAllRegisteredTypesItem) {
                    // List the Registered schema plugins
                    // Load this each time the menu is called in case plugins were loaded
                    //      in between invocations.
                    // However we cache it so the submenus don't need to re-query
                    schemaTypeGroups = getConcretePrimTypes(true);
                    for (auto schema : schemaTypeGroups) {
                        items.emplace_back(
                            schema._name.c_str(),
                            schema._name.c_str(),
                            Ufe::ContextItem::kHasChildren);
                    }
                }
            } else if (itemPath.size() == 3u) {
                if (itemPath[1] == kAllRegisteredTypesItem) {
                    // List the items that belong to this schema plugin
                    for (auto schema : schemaTypeGroups) {
                        if (schema._name != itemPath[2]) {
                            continue;
                        }
                        for (auto t : schema._types) {
                            items.emplace_back(t, t);
                        }
                    }
                }
#endif
            } // If USD >= 20.08, submenus end here. Otherwise end of Root Setup
        }
#if PXR_VERSION >= 2108
        else if (itemPath[0] == BindMaterialUndoableCommand::commandName) {
            if (fItem) {
                auto prim = fItem->prim();
                if (prim) {
                    // Find materials in the global selection. Either directly selected or a direct
                    // child of the selection:
                    if (auto globalSn = Ufe::GlobalSelection::get()) {
                        // Use a set to keep names alphabetically ordered and unique.
                        std::set<std::string> foundMaterials;
                        for (auto&& selItem : *globalSn) {
                            UsdSceneItem::Ptr usdItem
                                = std::dynamic_pointer_cast<UsdSceneItem>(selItem);
                            if (!usdItem) {
                                continue;
                            }
                            UsdShadeMaterial material(usdItem->prim());
                            if (material) {
                                std::string currentPrimPath
                                    = usdItem->prim().GetPath().GetAsString();
                                foundMaterials.insert(currentPrimPath);
                            }
                            for (auto&& usdChild : usdItem->prim().GetChildren()) {
                                UsdShadeMaterial material(usdChild);
                                if (material) {
                                    std::string currentPrimPath = usdChild.GetPath().GetAsString();
                                    foundMaterials.insert(currentPrimPath);
                                }
                            }
                        }
                        for (auto&& materialPath : foundMaterials) {
                            items.emplace_back(materialPath, materialPath);
                        }
                    }
                }
            }
        }
#endif
    } // Top-level items
    return items;
}

Ufe::UndoableCommand::Ptr UsdContextOps::doOpCmd(const ItemPath& itemPath)
{
    // Empty argument means no operation was specified, error.
    if (itemPath.empty()) {
        TF_CODING_ERROR("Empty path means no operation was specified");
        return nullptr;
    }

    if (itemPath[0u] == kUSDLoadItem || itemPath[0u] == kUSDLoadWithDescendantsItem) {
        const UsdLoadPolicy policy = (itemPath[0u] == kUSDLoadWithDescendantsItem)
            ? UsdLoadWithDescendants
            : UsdLoadWithoutDescendants;

        return std::make_shared<LoadUndoableCommand>(prim(), policy);
    } else if (itemPath[0u] == kUSDUnloadItem) {
        return std::make_shared<UnloadUndoableCommand>(prim());
    } else if (itemPath[0] == kUSDVariantSetsItem) {
        // Operation is to set a variant in a variant set.  Need both the
        // variant set and the variant as arguments to the operation.
        if (itemPath.size() != 3u) {
            TF_CODING_ERROR("Wrong number of arguments");
            return nullptr;
        }

        // At this point we know we have enough arguments to execute the
        // operation.
        return std::make_shared<SetVariantSelectionUndoableCommand>(path(), prim(), itemPath);
    } // Variant sets
    else if (itemPath[0] == kUSDToggleVisibilityItem) {
        auto object3d = UsdObject3d::create(fItem);
        TF_AXIOM(object3d);
        auto current = object3d->visibility();
        return object3d->setVisibleCmd(!current);
    } // Visibility
    else if (itemPath[0] == kUSDToggleActiveStateItem) {
        return std::make_shared<ToggleActiveStateCommand>(prim());
    } // ActiveState
    else if (itemPath[0] == kUSDToggleInstanceableStateItem) {
        return std::make_shared<ToggleInstanceableStateCommand>(prim());
    } // InstanceableState
    else if (!itemPath.empty() && (itemPath[0] == kUSDAddNewPrimItem)) {
        // Operation is to create a new prim of the type specified.
        if (itemPath.size() < 2u) {
            TF_CODING_ERROR("Wrong number of arguments");
            return nullptr;
        }
        // At this point we know the last item in the itemPath is the prim type to create
        auto primType = itemPath[itemPath.size() - 1];
        return UsdUndoAddNewPrimCommand::create(fItem, primType, primType);
#ifdef WANT_QT_BUILD
        // When building without Qt there is no LayerEditor
    } else if (itemPath[0] == kUSDLayerEditorItem) {
        // Just open the editor directly and return null so we don't have undo.
        auto ufePath = ufe::stagePath(prim().GetStage());
        auto noWorld = ufePath.popHead().string();
        auto dagPath = UsdMayaUtil::nameToDagPath(noWorld);
        auto shapePath = dagPath.fullPathName();

        MString script;
        script.format("mayaUsdLayerEditorWindow -proxyShape ^1s mayaUsdLayerEditor", shapePath);
        MGlobal::executeCommand(script);
        return nullptr;
#endif
    } else if (itemPath[0] == AddUsdReferenceUndoableCommand::commandName) {
        MString fileRef = MGlobal::executeCommandStringResult(_selectUSDFileScript());

        std::string path = UsdMayaUtil::convert(fileRef);
        if (path.empty())
            return nullptr;

        return std::make_shared<AddUsdReferenceUndoableCommand>(prim(), path);
    } else if (itemPath[0] == ClearAllReferencesUndoableCommand::commandName) {
        MString confirmation = MGlobal::executeCommandStringResult(clearAllReferencesConfirmScript);
        if (ClearAllReferencesUndoableCommand::cancelRemoval == confirmation)
            return nullptr;

        return std::make_shared<ClearAllReferencesUndoableCommand>(prim());
    }
#ifdef UFE_V3_FEATURES_AVAILABLE
    else if (itemPath[0] == kEditAsMayaItem) {
        MString script;
        script.format(
            "^1s \"^2s\"", EditAsMayaCommand::commandName, Ufe::PathString::string(path()).c_str());
        WaitCursor wait;
        MGlobal::executeCommand(script, /* display = */ true, /* undoable = */ true);
    } else if (itemPath[0] == kDuplicateAsMayaItem) {
        MString script;
        script.format(
            "^1s \"^2s\" \"|world\"",
            DuplicateCommand::commandName,
            Ufe::PathString::string(path()).c_str());
        WaitCursor wait;
        MGlobal::executeCommand(script, /* display = */ true, /* undoable = */ true);
    } else if (itemPath[0] == kAddMayaReferenceItem) {
        MString script;
        script.format("addMayaReferenceToUsd \"^1s\"", Ufe::PathString::string(path()).c_str());
        MString result = MGlobal::executeCommandStringResult(
            script, /* display = */ false, /* undoable = */ true);
    }
#endif
#if PXR_VERSION >= 2108
    else if (itemPath[0] == BindMaterialUndoableCommand::commandName) {
        return std::make_shared<BindMaterialUndoableCommand>(fItem->prim(), SdfPath(itemPath[1]));
    } else if (itemPath[0] == kBindMaterialToSelectionItem) {
        std::shared_ptr<Ufe::CompositeUndoableCommand> compositeCmd;
        if (auto globalSn = Ufe::GlobalSelection::get()) {
            for (auto&& selItem : *globalSn) {
                UsdPrim compatiblePrim = BindMaterialUndoableCommand::CompatiblePrim(selItem);
                if (compatiblePrim) {
                    if (!compositeCmd) {
                        compositeCmd = std::make_shared<Ufe::CompositeUndoableCommand>();
                    }
                    compositeCmd->append(std::make_shared<BindMaterialUndoableCommand>(
                        compatiblePrim, fItem->prim().GetPath()));
                }
            }
        }
        return compositeCmd;
    } else if (itemPath[0] == UnbindMaterialUndoableCommand::commandName) {
        return std::make_shared<UnbindMaterialUndoableCommand>(fItem->prim());
    }
#endif
    return nullptr;
}

} // namespace ufe
} // namespace MAYAUSD_NS_DEF
