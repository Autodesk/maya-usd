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
#if PXR_VERSION >= 2108
#include <mayaUsd/ufe/UsdUndoMaterialCommands.h>
#endif
#include <mayaUsd/nodes/proxyShapeStageExtraData.h>
#include <mayaUsd/ufe/SetVariantSelectionCommand.h>
#include <mayaUsd/ufe/UsdObject3d.h>
#include <mayaUsd/ufe/Utils.h>
#include <mayaUsd/utils/util.h>
#include <mayaUsd/utils/utilFileSystem.h>

#include <usdUfe/ufe/UsdSceneItem.h>
#include <usdUfe/ufe/UsdUndoAddNewPrimCommand.h>

#include <pxr/base/plug/plugin.h>
#include <pxr/base/plug/registry.h>
#include <pxr/base/tf/diagnostic.h>
#include <pxr/base/tf/stringUtils.h>
#include <pxr/pxr.h>
#include <pxr/usd/sdf/fileFormat.h>
#include <pxr/usd/sdf/path.h>
#include <pxr/usd/sdf/reference.h>
#include <pxr/usd/sdr/registry.h>
#include <pxr/usd/sdr/shaderNode.h>
#include <pxr/usd/sdr/shaderProperty.h>
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
#include <ufe/hierarchy.h>
#include <ufe/object3d.h>
#include <ufe/observableSelection.h>
#include <ufe/path.h>
#include <ufe/pathString.h>
#include <ufe/selectionUndoableCommands.h>

#include <algorithm>
#include <cassert>
#include <map>
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
#ifdef UFE_V4_FEATURES_AVAILABLE
static constexpr char kAssignNewMaterialItem[] = "Assign New Material";
static constexpr char kAssignNewMaterialLabel[] = "Assign New Material";
static constexpr char kAddNewMaterialItem[] = "Add New Material";
static constexpr char kAddNewMaterialLabel[] = "Add New Material";
static constexpr char kAssignExistingMaterialItem[] = "Assign Existing Material";
static constexpr char kAssignExistingMaterialLabel[] = "Assign Existing Material";
#endif
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

#ifdef UFE_V3_FEATURES_AVAILABLE
//! \brief Create a Prim and select it:
class UsdUndoAddNewPrimAndSelectCommand : public Ufe::CompositeUndoableCommand
{
public:
    UsdUndoAddNewPrimAndSelectCommand(const UsdUfe::UsdUndoAddNewPrimCommand::Ptr& creationCmd)
        : Ufe::CompositeUndoableCommand({ creationCmd })
    {
    }

    void execute() override
    {
        auto addPrimCmd
            = std::dynamic_pointer_cast<UsdUfe::UsdUndoAddNewPrimCommand>(cmdsList().front());
        addPrimCmd->execute();
        // Create the selection command only if the creation succeeded:
        if (!addPrimCmd->newUfePath().empty()) {
            Ufe::Selection newSelection;
            newSelection.append(Ufe::Hierarchy::createItem(addPrimCmd->newUfePath()));
            append(Ufe::SelectionReplaceWith::createAndExecute(
                Ufe::GlobalSelection::get(), newSelection));
        }
    }

#ifdef UFE_V4_FEATURES_AVAILABLE
    std::string commandString() const override { return cmdsList().front()->commandString(); }
#endif
};

//! \brief Create a working Material and select it:
class InsertChildAndSelectCommand : public Ufe::CompositeUndoableCommand
{
public:
    InsertChildAndSelectCommand(const Ufe::InsertChildCommand::Ptr& creationCmd)
        : Ufe::CompositeUndoableCommand({ creationCmd })
    {
    }

    void execute() override
    {
        auto insertChildCmd
            = std::dynamic_pointer_cast<Ufe::InsertChildCommand>(cmdsList().front());
        insertChildCmd->execute();
        // Create the selection command only if the creation succeeded:
        if (insertChildCmd->insertedChild()) {
            Ufe::Selection newSelection;
            newSelection.append(insertChildCmd->insertedChild());
            append(Ufe::SelectionReplaceWith::createAndExecute(
                Ufe::GlobalSelection::get(), newSelection));
        }
    }

#ifdef UFE_V4_FEATURES_AVAILABLE
    std::string commandString() const override { return cmdsList().front()->commandString(); }
#endif
};
#endif

//! \brief Undoable command for loading a USD prim.
class LoadUnloadBaseUndoableCommand : public Ufe::UndoableCommand
{
protected:
    LoadUnloadBaseUndoableCommand(const UsdPrim& prim, UsdLoadPolicy policy)
        : _stage(prim.GetStage())
        , _primPath(prim.GetPath())
        , _policy(policy)
    {
    }

    LoadUnloadBaseUndoableCommand(const UsdPrim& prim)
        : _stage(prim.GetStage())
        , _primPath(prim.GetPath())
        , _policy(UsdLoadPolicy::UsdLoadWithoutDescendants)
    {
        if (!_stage)
            return;

        // When not provided with the load policy, we need to figure out
        // what the current policy is.
        UsdStageLoadRules loadRules = _stage->GetLoadRules();
        _policy = loadRules.GetEffectiveRuleForPath(_primPath) == UsdStageLoadRules::Rule::AllRule
            ? UsdLoadPolicy::UsdLoadWithDescendants
            : UsdLoadPolicy::UsdLoadWithoutDescendants;
    }

    void doLoad() const
    {
        if (!_stage)
            return;

        _stage->Load(_primPath, _policy);
        saveModifiedLoadRules();
    }

    void doUnload() const
    {
        if (!_stage)
            return;

        _stage->Unload(_primPath);
        saveModifiedLoadRules();
    }

    void saveModifiedLoadRules() const
    {
        // Save the load rules so that switching the stage settings will be able to preserve the
        // load rules.
        MAYAUSD_NS::MayaUsdProxyShapeStageExtraData::saveLoadRules(_stage);
    }

private:
    const UsdStageWeakPtr _stage;
    const SdfPath         _primPath;
    UsdLoadPolicy         _policy;
};

//! \brief Undoable command for loading a USD prim.
class LoadUndoableCommand : public LoadUnloadBaseUndoableCommand
{
public:
    LoadUndoableCommand(const UsdPrim& prim, UsdLoadPolicy policy)
        : LoadUnloadBaseUndoableCommand(prim, policy)
    {
    }

    void redo() override { doLoad(); }
    void undo() override { doUnload(); }
};

//! \brief Undoable command for unloading a USD prim.
class UnloadUndoableCommand : public LoadUnloadBaseUndoableCommand
{
public:
    UnloadUndoableCommand(const UsdPrim& prim)
        : LoadUnloadBaseUndoableCommand(prim)
    {
    }

    void redo() override { doUnload(); }
    void undo() override { doLoad(); }
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
                UsdUfe::InAddOrDeleteOperation ad;
                prim.SetActive(_active);
            }
        }
    }

    void redo() override
    {
        if (_stage) {
            UsdPrim prim = _stage->GetPrimAtPath(_primPath);
            if (prim.IsValid()) {
                UsdUfe::InAddOrDeleteOperation ad;
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

const PXR_NS::SdfLayerHandle getCurrentTargetLayer(const UsdPrim& prim)
{
    auto stage = prim.GetStage();
    if (!stage)
        return {};

    return stage->GetEditTarget().GetLayer();
}

const std::string getTargetLayerFilePath(const UsdPrim& prim)
{
    auto layer = getCurrentTargetLayer(prim);
    if (!layer)
        return {};

    return layer->GetRealPath();
}

bool _prepareUSDReferenceTargetLayer(const UsdPrim& prim)
{
    return UsdMayaUtilFileSystem::prepareLayerSaveUILayer(getCurrentTargetLayer(prim), false);
}

// Ask SDF for all supported extensions:
const char* _selectUSDFileScript()
{
    static std::string commandString;

    if (commandString.empty()) {
        // This is an interactive call from the main UI thread. No need for SMP protections.

        // The goal of the following loop is to build a first file filter that allow any
        // USD-compatible file format, then a series of file filters, one per particular
        // file format. So for N different file formats, we will have N+1 filters.

        std::vector<std::string> usdUiStrings;
        std::vector<std::string> usdSelectors;
        std::vector<std::string> otherUiStrings;
        std::vector<std::string> otherSelectors;

        for (auto&& extension : SdfFileFormat::FindAllFileFormatExtensions()) {
            // Put USD first
            if (extension.rfind("usd", 0) == 0) {
                usdUiStrings.push_back("*." + extension);
                usdSelectors.push_back("*." + extension);
            } else {
                otherUiStrings.push_back("*." + extension);
                otherSelectors.push_back("*." + extension);
            }
        }

        usdUiStrings.insert(usdUiStrings.end(), otherUiStrings.begin(), otherUiStrings.end());
        usdSelectors.insert(usdSelectors.end(), otherSelectors.begin(), otherSelectors.end());

        const char* script = R"mel(
        global proc string SelectUSDFileForAddReference()
        {
            string $result[] = `fileDialog2
                -fileMode 1
                -caption "Add Reference to USD Prim"
                -fileFilter "USD Files (%s);;%s"
                -optionsUICreate addUSDReferenceCreateUi
                -optionsUIInit addUSDReferenceInitUi
                -optionsUICommit2 addUSDReferenceToUsdCommitUi`;

            if (0 == size($result))
                return "";
            else
                return $result[0];
        }
        SelectUSDFileForAddReference();
        )mel";

        commandString = TfStringPrintf(
            script, TfStringJoin(usdUiStrings).c_str(), TfStringJoin(usdSelectors, ";;").c_str());
    }

    return commandString.c_str();
}

std::string
makeUSDReferenceFilePathRelativeIfRequested(const std::string& filePath, const UsdPrim& prim)
{
    if (!UsdMayaUtilFileSystem::requireUsdPathsRelativeToEditTargetLayer())
        return filePath;

    const std::string layerDirPath = UsdMayaUtilFileSystem::getDir(getTargetLayerFilePath(prim));

    auto relativePathAndSuccess = UsdMayaUtilFileSystem::makePathRelativeTo(filePath, layerDirPath);

    if (!relativePathAndSuccess.second) {
        TF_WARN(
            "File name (%s) cannot be resolved as relative to the current edit target layer, "
            "using the absolute path.",
            filePath.c_str());
    }

    return relativePathAndSuccess.first;
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

bool sceneItemSupportsShading(const Ufe::SceneItem::Ptr& sceneItem)
{
#if PXR_VERSION >= 2108
    if (MayaUsd::ufe::BindMaterialUndoableCommand::CompatiblePrim(sceneItem)) {
        return true;
    }
#else
    auto usdItem = std::dynamic_pointer_cast<UsdUfe::UsdSceneItem>(sceneItem);
    if (!usdItem) {
        return false;
    }
    if (PXR_NS::UsdShadeMaterialBindingAPI(usdItem->prim())) {
        return true;
    }
#endif
    return false;
}

bool selectionSupportsShading()
{
    if (auto globalSn = Ufe::GlobalSelection::get()) {
        for (auto&& selItem : *globalSn) {
            if (sceneItemSupportsShading(selItem)) {
                return true;
            }
        }
    }
    return false;
}

#ifdef UFE_V3_FEATURES_AVAILABLE

void executeEditAsMaya(const Ufe::Path& path)
{
    MString script;
    script.format(
        "^1s \"^2s\"",
        MayaUsd::ufe::EditAsMayaCommand::commandName,
        Ufe::PathString::string(path).c_str());
    WaitCursor wait;
    MGlobal::executeCommand(script, /* display = */ true, /* undoable = */ true);
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
        if (fItem->prim().IsA<UsdShadeMaterial>() && selectionSupportsShading()) {
            items.emplace_back(kBindMaterialToSelectionItem, kBindMaterialToSelectionLabel);
            items.emplace_back(Ufe::ContextItem::kSeparator);
        }
#endif
#ifdef WANT_QT_BUILD
        // Top-level item - USD Layer editor (for all context op types).
        // Only available when building with Qt enabled.
        items.emplace_back(kUSDLayerEditorItem, kUSDLayerEditorLabel, kUSDLayerEditorImage);
#endif

#ifdef UFE_V3_FEATURES_AVAILABLE
        const bool isMayaRef = (prim().GetTypeName() == TfToken("MayaReference"));
        if (!fIsAGatewayType && PrimUpdaterManager::getInstance().canEditAsMaya(path())) {
            items.emplace_back(kEditAsMayaItem, kEditAsMayaLabel, kEditAsMayaImage);
            if (!isMayaRef) {
                items.emplace_back(kDuplicateAsMayaItem, kDuplicateAsMayaLabel);
            }
        }
        if (!isMayaRef) {
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
        if (!fIsAGatewayType) {
            items.emplace_back(
                AddUsdReferenceUndoableCommand::commandName,
                AddUsdReferenceUndoableCommand::commandName);
            items.emplace_back(
                ClearAllReferencesUndoableCommand::commandName,
                ClearAllReferencesUndoableCommand::commandName);
        }
#if PXR_VERSION >= 2108
        if (!fIsAGatewayType) {
            // Top level item - Bind/unbind existing materials
            bool materialSeparatorsAdded = false;
            int  allowMaterialFunctions = 0;
#ifdef UFE_V4_FEATURES_AVAILABLE
            MString script;
            script.format(
                "mayaUsdMaterialBindings \"^1s\" -canAssignMaterialToNodeType true",
                Ufe::PathString::string(fItem->path()).c_str());
            MGlobal::executeCommand(script, allowMaterialFunctions);

            if (allowMaterialFunctions && sceneItemSupportsShading(fItem)) {
                if (!materialSeparatorsAdded) {
                    items.emplace_back(Ufe::ContextItem::kSeparator);
                    materialSeparatorsAdded = true;
                }
                items.emplace_back(
                    kAssignNewMaterialItem,
                    kAssignNewMaterialLabel,
                    Ufe::ContextItem::kHasChildren);

                // Only show this option if we actually have materials in the stage.
                MStringArray materials;
                script.format(
                    "mayaUsdGetMaterialsInStage \"^1s\"",
                    Ufe::PathString::string(fItem->path()).c_str());
                MGlobal::executeCommand(script, materials);
                if (materials.length() > 0) {
                    items.emplace_back(
                        kAssignExistingMaterialItem,
                        kAssignExistingMaterialLabel,
                        Ufe::ContextItem::kHasChildren);
                }
            }
#endif
            if (allowMaterialFunctions && fItem->prim().HasAPI<UsdShadeMaterialBindingAPI>()) {
                UsdShadeMaterialBindingAPI bindingAPI(fItem->prim());
                // Show unbind menu item if there is a direct binding relationship:
                auto directBinding = bindingAPI.GetDirectBinding();
                if (directBinding.GetMaterial()) {
                    if (!materialSeparatorsAdded) {
                        items.emplace_back(Ufe::ContextItem::kSeparator);
                        materialSeparatorsAdded = true;
                    }
                    items.emplace_back(
                        UnbindMaterialUndoableCommand::commandName,
                        UnbindMaterialUndoableCommand::commandName);
                }
            }
#ifdef UFE_V4_FEATURES_AVAILABLE
            if (UsdUndoAddNewMaterialCommand::CompatiblePrim(fItem)) {
                if (!materialSeparatorsAdded) {
                    items.emplace_back(Ufe::ContextItem::kSeparator);
                    materialSeparatorsAdded = true;
                }
                items.emplace_back(
                    kAddNewMaterialItem, kAddNewMaterialLabel, Ufe::ContextItem::kHasChildren);
            }
#endif
        }
#endif
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
#ifdef UFE_V4_FEATURES_AVAILABLE
        } else if (itemPath[0] == kAssignNewMaterialItem || itemPath[0] == kAddNewMaterialItem) {
            std::multimap<std::string, MString> renderersAndMaterials;
            MStringArray                        materials;
            MGlobal::executeCommand("mayaUsdGetMaterialsFromRenderers", materials);

            for (const auto& materials : materials) {
                // Expects a string in the format "renderer/Material Name|Material Identifier".
                MStringArray rendererAndMaterial;
                MStatus      status = materials.split('/', rendererAndMaterial);
                if (status == MS::kSuccess && rendererAndMaterial.length() == 2) {
                    renderersAndMaterials.emplace(
                        std::string(rendererAndMaterial[0].asChar()), rendererAndMaterial[1]);
                }
            }

            if (itemPath.size() == 1u) {
                // Populate list of known renderers (first menu level).
                for (auto it = renderersAndMaterials.begin(), end = renderersAndMaterials.end();
                     it != end;
                     it = renderersAndMaterials.upper_bound(it->first)) {
                    items.emplace_back(it->first, it->first, Ufe::ContextItem::kHasChildren);
                }
            } else if (itemPath.size() == 2u) {
                // Populate list of materials for a given renderer (second menu level).
                const auto range = renderersAndMaterials.equal_range(itemPath[1]);
                for (auto it = range.first; it != range.second; ++it) {
                    MStringArray materialAndIdentifier;
                    // Expects a string in the format "Material Name|MaterialIdentifer".
                    MStatus status = it->second.split('|', materialAndIdentifier);
                    if (status == MS::kSuccess && materialAndIdentifier.length() == 2) {
                        items.emplace_back(
                            materialAndIdentifier[1].asChar(), materialAndIdentifier[0].asChar());
                    }
                }
            }
        } else if (itemPath[0] == kAssignExistingMaterialItem) {
            std::multimap<std::string, MString> pathsAndMaterials;
            MStringArray                        materials;
            MString                             script;
            script.format(
                "mayaUsdGetMaterialsInStage \"^1s\"",
                Ufe::PathString::string(fItem->path()).c_str());
            MGlobal::executeCommand(script, materials);

            for (const auto& material : materials) {
                MStringArray pathAndMaterial;
                // Expects a string in the format "/path1/path2/Material".
                const int lastSlash = material.rindex('/');
                if (lastSlash >= 0) {
                    MString pathToMaterial = material.substring(0, lastSlash);
                    pathsAndMaterials.emplace(std::string(pathToMaterial.asChar()), material);
                }
            }

            if (itemPath.size() == 1u) {
                // Populate list of paths to materials (first menu level).
                for (auto it = pathsAndMaterials.begin(), end = pathsAndMaterials.end(); it != end;
                     it = pathsAndMaterials.upper_bound(it->first)) {
                    items.emplace_back(it->first, it->first, Ufe::ContextItem::kHasChildren);
                }
            } else if (itemPath.size() == 2u) {
                // Populate list of to materials for given path (second  menu level).
                const auto range = pathsAndMaterials.equal_range(itemPath[1]);
                for (auto it = range.first; it != range.second; ++it) {
                    const int lastSlash = it->second.rindex('/');
                    if (lastSlash >= 0) {
                        MString materialName
                            = it->second.substring(lastSlash + 1, it->second.length() - 1);
                        items.emplace_back(it->second.asChar(), materialName.asChar());
                    }
                }
            }
#endif
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
        // operation. If the prim is a Maya reference with auto-edit on,
        // the edit it instead of switching to the Maya reference.
        return std::make_shared<ufe::SetVariantSelectionCommand>(
            path(), prim(), itemPath[1], itemPath[2]);
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
#ifdef UFE_V3_FEATURES_AVAILABLE
        return std::make_shared<UsdUndoAddNewPrimAndSelectCommand>(
            UsdUfe::UsdUndoAddNewPrimCommand::create(fItem, primType, primType));
#else
        return UsdUfe::UsdUndoAddNewPrimCommand::create(fItem, primType, primType);
#endif
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
        if (!_prepareUSDReferenceTargetLayer(prim()))
            return nullptr;

        MString fileRef = MGlobal::executeCommandStringResult(_selectUSDFileScript());
        if (fileRef.length() == 0)
            return nullptr;

        const std::string path
            = makeUSDReferenceFilePathRelativeIfRequested(UsdMayaUtil::convert(fileRef), prim());
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
        executeEditAsMaya(path());
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
        return std::make_shared<BindMaterialUndoableCommand>(fItem->path(), SdfPath(itemPath[1]));
    } else if (itemPath[0] == kBindMaterialToSelectionItem) {
        std::shared_ptr<Ufe::CompositeUndoableCommand> compositeCmd;
        if (auto globalSn = Ufe::GlobalSelection::get()) {
            for (auto&& selItem : *globalSn) {
                if (BindMaterialUndoableCommand::CompatiblePrim(selItem)) {
                    if (!compositeCmd) {
                        compositeCmd = std::make_shared<Ufe::CompositeUndoableCommand>();
                    }
                    compositeCmd->append(std::make_shared<BindMaterialUndoableCommand>(
                        selItem->path(), fItem->prim().GetPath()));
                }
            }
        }
        return compositeCmd;
    } else if (itemPath[0] == UnbindMaterialUndoableCommand::commandName) {
        return std::make_shared<UnbindMaterialUndoableCommand>(fItem->path());
#ifdef UFE_V4_FEATURES_AVAILABLE
    } else if (itemPath.size() == 3u && itemPath[0] == kAssignNewMaterialItem) {
        // Make a copy so that we don't change the user's original selection.
        Ufe::Selection sceneItems(*Ufe::GlobalSelection::get());
        // As per UX' wishes, we add the item that was right-clicked,
        // regardless of its selection state.
        sceneItems.append(fItem);
        if (sceneItems.size() > 0u) {
            return std::make_shared<InsertChildAndSelectCommand>(
                UsdUndoAssignNewMaterialCommand::create(sceneItems, itemPath[2]));
        }
    } else if (itemPath.size() == 3u && itemPath[0] == kAddNewMaterialItem) {
        return std::make_shared<InsertChildAndSelectCommand>(
            UsdUndoAddNewMaterialCommand::create(fItem, itemPath[2]));
    } else if (itemPath.size() == 3u && itemPath[0] == kAssignExistingMaterialItem) {
        std::shared_ptr<Ufe::CompositeUndoableCommand> compositeCmd;
        Ufe::Selection                                 sceneItems(*Ufe::GlobalSelection::get());
        sceneItems.append(fItem);
        for (auto& sceneItem : sceneItems) {
            if (BindMaterialUndoableCommand::CompatiblePrim(sceneItem)) {
                if (!compositeCmd) {
                    compositeCmd = std::make_shared<Ufe::CompositeUndoableCommand>();
                }
                compositeCmd->append(std::make_shared<BindMaterialUndoableCommand>(
                    sceneItem->path(), SdfPath(itemPath[2])));
            }
        }
        return compositeCmd;
#endif
    }
#endif
    return nullptr;
}

} // namespace ufe
} // namespace MAYAUSD_NS_DEF