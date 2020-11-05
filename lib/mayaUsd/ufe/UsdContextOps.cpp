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

#include <mayaUsd/ufe/UsdSceneItem.h>
#include <mayaUsd/ufe/UsdUndoAddNewPrimCommand.h>
#include <mayaUsd/ufe/Utils.h>
#include <mayaUsd/utils/util.h>

#include <pxr/base/tf/diagnostic.h>
#include <pxr/usd/sdf/path.h>
#include <pxr/usd/sdf/reference.h>
#include <pxr/usd/usd/common.h>
#include <pxr/usd/usd/prim.h>
#include <pxr/usd/usd/references.h>
#include <pxr/usd/usd/stage.h>
#include <pxr/usd/usd/variantSets.h>
#include <pxr/usd/usdGeom/tokens.h>

#include <maya/MGlobal.h>
#include <ufe/attribute.h>
#include <ufe/attributes.h>
#include <ufe/path.h>

#include <algorithm>
#include <cassert>
#include <utility>
#include <vector>

namespace {

// Ufe::ContextItem strings
// - the "Item" describe the operation to be performed.
// - the "Label" is used in the context menu (can be localized).
// - the "Image" is used for icon in the context menu. Directly used std::string
//   for these so the emplace_back() will choose the right constructor. With char[]
//   it would convert that param to a bool and choose the wrong constructor.
static constexpr char    kUSDLayerEditorItem[] = "USD Layer Editor";
static constexpr char    kUSDLayerEditorLabel[] = "USD Layer Editor...";
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

//! \brief Undoable command for loading a USD prim.
class LoadUndoableCommand : public Ufe::UndoableCommand
{
public:
    LoadUndoableCommand(const UsdPrim& prim, UsdLoadPolicy policy)
        : _stage(prim.GetStage())
        , _primPath(prim.GetPath())
        , _oldLoadSet(prim.GetStage()->GetLoadSet())
        , _policy(policy)
    {
    }

    void undo() override
    {
        if (!_stage) {
            return;
        }

        _stage->LoadAndUnload(_oldLoadSet, SdfPathSet({ _primPath }));
    }

    void redo() override
    {
        if (!_stage) {
            return;
        }

        _stage->Load(_primPath, _policy);
    }

private:
    const UsdStageWeakPtr _stage;
    const SdfPath         _primPath;
    const SdfPathSet      _oldLoadSet;
    const UsdLoadPolicy   _policy;
};

//! \brief Undoable command for unloading a USD prim.
class UnloadUndoableCommand : public Ufe::UndoableCommand
{
public:
    UnloadUndoableCommand(const UsdPrim& prim)
        : _stage(prim.GetStage())
        , _primPath({ prim.GetPath() })
        , _oldLoadSet(prim.GetStage()->GetLoadSet())
    {
    }

    void undo() override
    {
        if (!_stage) {
            return;
        }

        _stage->LoadAndUnload(_oldLoadSet, SdfPathSet());
    }

    void redo() override
    {
        if (!_stage) {
            return;
        }

        _stage->Unload(_primPath);
    }

private:
    const UsdStageWeakPtr _stage;
    const SdfPath         _primPath;
    const SdfPathSet      _oldLoadSet;
};

//! \brief Undoable command for variant selection change
class SetVariantSelectionUndoableCommand : public Ufe::UndoableCommand
{
public:
    SetVariantSelectionUndoableCommand(
        const UsdPrim&                   prim,
        const Ufe::ContextOps::ItemPath& itemPath)
        : fVarSet(prim.GetVariantSets().GetVariantSet(itemPath[1]))
        , fOldSelection(fVarSet.GetVariantSelection())
        , fNewSelection(itemPath[2])
    {
    }

    void undo() override { fVarSet.SetVariantSelection(fOldSelection); }

    void redo() override { fVarSet.SetVariantSelection(fNewSelection); }

private:
    UsdVariantSet     fVarSet;
    const std::string fOldSelection;
    const std::string fNewSelection;
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

const char* selectUSDFileScript = R"(
global proc string SelectUSDFileForAddReference()
{
    string $result[] = `fileDialog2 
        -fileMode 1
        -caption "Add Reference to USD Prim"
        -fileFilter "USD Files (*.usd *.usda *.usdc);;*.usd;;*.usda;;*.usdc"`;

    if (0 == size($result))
        return "";
    else
        return $result[0];
}
SelectUSDFileForAddReference();
)";

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

class AddReferenceUndoableCommand : public Ufe::UndoableCommand
{
public:
    static const std::string commandName;

    AddReferenceUndoableCommand(const UsdPrim& prim, const std::string& filePath)
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
            _sdfRef = SdfReference(_filePath);
            UsdReferences primRefs = _prim.GetReferences();
            primRefs.AddReference(_sdfRef);
        }
    }

private:
    UsdPrim           _prim;
    SdfReference      _sdfRef;
    const std::string _filePath;
};
const std::string AddReferenceUndoableCommand::commandName("Add Reference...");

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
#if USD_VERSION_NUM >= 2011
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

} // namespace

namespace MAYAUSD_NS_DEF {
namespace ufe {

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
        // Top-level item - USD Layer editor (for all context op types).
        int hasLayerEditorCmd { 0 };
        MGlobal::executeCommand("runTimeCommand -exists UsdLayerEditor", hasLayerEditorCmd);
        if (hasLayerEditorCmd) {
#if UFE_PREVIEW_VERSION_NUM >= 2023
            items.emplace_back(kUSDLayerEditorItem, kUSDLayerEditorLabel, kUSDLayerEditorImage);
#else
            items.emplace_back(kUSDLayerEditorItem, kUSDLayerEditorLabel);
#endif
            items.emplace_back(Ufe::ContextItem::kSeparator);
        }

        // Top-level items (do not add for gateway type node):
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
        }

        // Top level item - Add New Prim (for all context op types).
        items.emplace_back(kUSDAddNewPrimItem, kUSDAddNewPrimLabel, Ufe::ContextItem::kHasChildren);

        if (!fIsAGatewayType) {
            items.emplace_back(
                AddReferenceUndoableCommand::commandName, AddReferenceUndoableCommand::commandName);
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
#if UFE_PREVIEW_VERSION_NUM >= 2023
            items.emplace_back(
                kUSDDefPrimItem, kUSDDefPrimLabel, kUSDDefPrimImage); // typeless prim
            items.emplace_back(kUSDScopePrimItem, kUSDScopePrimLabel, kUSDScopePrimImage);
            items.emplace_back(kUSDXformPrimItem, kUSDXformPrimLabel, kUSDXformPrimImage);
            items.emplace_back(Ufe::ContextItem::kSeparator);
            items.emplace_back(kUSDCapsulePrimItem, kUSDCapsulePrimLabel, kUSDCapsulePrimImage);
            items.emplace_back(kUSDConePrimItem, kUSDConePrimLabel, kUSDConePrimImage);
            items.emplace_back(kUSDCubePrimItem, kUSDCubePrimLabel, kUSDCubePrimImage);
            items.emplace_back(kUSDCylinderPrimItem, kUSDCylinderPrimLabel, kUSDCylinderPrimImage);
            items.emplace_back(kUSDSpherePrimItem, kUSDSpherePrimLabel, kUSDSpherePrimImage);
#else
            items.emplace_back(kUSDDefPrimItem, kUSDDefPrimLabel); // typeless prim
            items.emplace_back(kUSDScopePrimItem, kUSDScopePrimLabel);
            items.emplace_back(kUSDXformPrimItem, kUSDXformPrimLabel);
            items.emplace_back(Ufe::ContextItem::kSeparator);
            items.emplace_back(kUSDCapsulePrimItem, kUSDCapsulePrimLabel);
            items.emplace_back(kUSDConePrimItem, kUSDConePrimLabel);
            items.emplace_back(kUSDCubePrimItem, kUSDCubePrimLabel);
            items.emplace_back(kUSDCylinderPrimItem, kUSDCylinderPrimLabel);
            items.emplace_back(kUSDSpherePrimItem, kUSDSpherePrimLabel);
#endif
        }
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
        return std::make_shared<SetVariantSelectionUndoableCommand>(prim(), itemPath);
    } // Variant sets
    else if (itemPath[0] == kUSDToggleVisibilityItem) {
        auto attributes = Ufe::Attributes::attributes(sceneItem());
        assert(attributes);
        auto visibility = std::dynamic_pointer_cast<Ufe::AttributeEnumString>(
            attributes->attribute(UsdGeomTokens->visibility));
        assert(visibility);
        auto current = visibility->get();
        return visibility->setCmd(
            current == UsdGeomTokens->invisible ? UsdGeomTokens->inherited
                                                : UsdGeomTokens->invisible);
    } // Visibility
    else if (itemPath[0] == kUSDToggleActiveStateItem) {
        return std::make_shared<ToggleActiveStateCommand>(prim());
    } // ActiveState
    else if (!itemPath.empty() && (itemPath[0] == kUSDAddNewPrimItem)) {
        // Operation is to create a new prim of the type specified.
        if (itemPath.size() != 2u) {
            TF_CODING_ERROR("Wrong number of arguments");
            return nullptr;
        }

        // At this point we know we have 2 arguments to execute the operation.
        // itemPath[1] contains the new prim type to create.
        return UsdUndoAddNewPrimCommand::create(fItem, itemPath[1], itemPath[1]);
    } else if (itemPath[0] == kUSDLayerEditorItem) {
        // Just open the editor directly and return null so we don't have undo.
        MGlobal::executeCommand("UsdLayerEditor");
        return nullptr;
    } else if (itemPath[0] == AddReferenceUndoableCommand::commandName) {
        MString fileRef = MGlobal::executeCommandStringResult(selectUSDFileScript);

        std::string path = UsdMayaUtil::convert(fileRef);
        if (path.empty())
            return nullptr;

        return std::make_shared<AddReferenceUndoableCommand>(prim(), path);
    } else if (itemPath[0] == ClearAllReferencesUndoableCommand::commandName) {
        MString confirmation = MGlobal::executeCommandStringResult(clearAllReferencesConfirmScript);
        if (ClearAllReferencesUndoableCommand::cancelRemoval == confirmation)
            return nullptr;

        return std::make_shared<ClearAllReferencesUndoableCommand>(prim());
    }

    return nullptr;
}

} // namespace ufe
} // namespace MAYAUSD_NS_DEF
