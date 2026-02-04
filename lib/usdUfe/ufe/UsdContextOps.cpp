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

#include <usdUfe/ufe/Global.h>
#include <usdUfe/ufe/SetVariantSelectionCommand.h>
#include <usdUfe/ufe/UfeNotifGuard.h>
#include <usdUfe/ufe/UsdObject3dHandler.h>
#include <usdUfe/ufe/UsdSceneItem.h>
#include <usdUfe/ufe/UsdUndoAddNewPrimCommand.h>
#include <usdUfe/ufe/UsdUndoClearDefaultPrimCommand.h>
#include <usdUfe/ufe/UsdUndoLongDurationCommand.h>
#include <usdUfe/ufe/UsdUndoPayloadCommand.h>
#include <usdUfe/ufe/UsdUndoSelectAfterCommand.h>
#include <usdUfe/ufe/UsdUndoSetDefaultPrimCommand.h>
#include <usdUfe/ufe/UsdUndoToggleActiveCommand.h>
#include <usdUfe/ufe/UsdUndoToggleInstanceableCommand.h>
#include <usdUfe/ufe/Utils.h>

#include <pxr/base/plug/plugin.h>
#include <pxr/base/plug/registry.h>
#include <pxr/base/tf/stringUtils.h>
#include <pxr/usd/usd/variantSets.h>

#include <ufe/globalSelection.h>

#include <vector>

PXR_NAMESPACE_USING_DIRECTIVE

namespace {

// Ufe::ContextItem strings
// - the "Item" describe the operation to be performed.
// - the "Label" is used in the context menu (can be localized).
// - the "Image" is used for icon in the context menu. Directly used std::string
//   for these so the emplace_back() will choose the right constructor. With char[]
//   it would convert that param to a bool and choose the wrong constructor.
static constexpr char    kUSDLoadItem[] = "Load";
static constexpr char    kUSDLoadLabel[] = "Load";
static constexpr char    kUSDLoadWithDescendantsItem[] = "Load with Descendants";
static constexpr char    kUSDLoadWithDescendantsLabel[] = "Load with Descendants";
static constexpr char    kUSDUnloadItem[] = "Unload";
static constexpr char    kUSDUnloadLabel[] = "Unload";
static constexpr char    kUSDVariantSetsItem[] = "Variant Sets";
static constexpr char    kUSDVariantSetsLabel[] = "Variant Sets";
static constexpr char    kUSDToggleVisibilityItem[] = "Toggle Visibility";
static constexpr char    kUSDMakeVisibleItem[] = "Make Visible";
static constexpr char    kUSDMakeVisibleLabel[] = "Make Visible";
static constexpr char    kUSDMakeInvisibleItem[] = "Make Invisible";
static constexpr char    kUSDMakeInvisibleLabel[] = "Make Invisible";
static constexpr char    kUSDToggleActiveStateItem[] = "Toggle Active State";
static constexpr char    kUSDActivatePrimItem[] = "Activate Prim";
static constexpr char    kUSDActivatePrimLabel[] = "Activate Prim";
static constexpr char    kUSDDeactivatePrimItem[] = "Deactivate Prim";
static constexpr char    kUSDDeactivatePrimLabel[] = "Deactivate Prim";
static constexpr char    kUSDToggleInstanceableStateItem[] = "Toggle Instanceable State";
static constexpr char    kUSDMarkAsInstanceabaleItem[] = "Mark as Instanceable";
static constexpr char    kUSDMarkAsInstanceabaleLabel[] = "Mark as Instanceable";
static constexpr char    kUSDUnmarkAsInstanceableItem[] = "Unmark as Instanceable";
static constexpr char    kUSDUnmarkAsInstanceableLabel[] = "Unmark as Instanceable";
static constexpr char    kUSDSetAsDefaultPrim[] = "Set as Default Prim";
static constexpr char    kUSDClearDefaultPrim[] = "Clear Default Prim";
static constexpr char    kUSDAddNewPrimItem[] = "Add New Prim";
static constexpr char    kUSDAddNewPrimLabel[] = "Add New Prim";
static constexpr char    kUSDClassPrimItem[] = "Class";
static constexpr char    kUSDClassPrimLabel[] = "Class";
static const std::string kUSDClassPrimImage { "out_USD_Class.png" };
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
#if PXR_VERSION >= 2208
static constexpr char    kUSDPlanePrimItem[] = "Plane";
static constexpr char    kUSDPlanePrimLabel[] = "Plane";
static const std::string kUSDPlanePrimImage { "out_USD_Plane.png" };
#endif
static constexpr char    kUSDSpherePrimItem[] = "Sphere";
static constexpr char    kUSDSpherePrimLabel[] = "Sphere";
static const std::string kUSDSpherePrimImage { "out_USD_Sphere.png" };

static constexpr char kAllRegisteredTypesItem[] = "All Registered";
static constexpr char kAllRegisteredTypesLabel[] = "All Registered";

static constexpr char kBulkEditItem[] = "BulkEdit";
static constexpr char kBulkEditMixedTypeLabel[] = "%zu Prims Selected";
static constexpr char kBulkEditSameTypeLabel[] = "%zu %s Prims Selected";

std::vector<std::pair<const char* const, const char* const>>
_computeLoadAndUnloadItems(const UsdPrim& prim)
{
    std::vector<std::pair<const char* const, const char* const>> itemLabelPairs;

    const bool isInPrototype = prim.IsInPrototype();
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

//! \brief Get groups of concrete schema prim types to list dynamically in the UI
static const std::vector<UsdUfe::SchemaTypeGroup>
getConcretePrimTypes(bool sorted, const UsdUfe::UsdContextOps::SchemaNameMap& schemaPluginNiceNames)
{
    std::vector<UsdUfe::SchemaTypeGroup> groups;

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
        auto       plugin_name = plugin->GetName();
        const auto search = schemaPluginNiceNames.find(plugin_name);
        if (search != schemaPluginNiceNames.end()) {
            plugin_name = search->second;
        }

        // We don't list empty names. This allows hiding certain plugins too.
        if (plugin_name.empty()) {
            continue;
        }

        auto type_name = UsdSchemaRegistry::GetConcreteSchemaTypeName(t);

        // Find or create the schema group and add to it
        auto group_itr = find(begin(groups), end(groups), plugin_name);
        if (group_itr == groups.end()) {
            UsdUfe::SchemaTypeGroup group { plugin_name };
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

} // namespace

namespace USDUFE_NS_DEF {

USDUFE_VERIFY_CLASS_SETUP(Ufe::ContextOps, UsdContextOps);

std::vector<SchemaTypeGroup> UsdContextOps::schemaTypeGroups = {};

UsdContextOps::UsdContextOps(const UsdSceneItem::Ptr& item)
    : Ufe::ContextOps()
{
    setItem(item);
}

/*static*/
UsdContextOps::Ptr UsdContextOps::create(const UsdSceneItem::Ptr& item)
{
    return std::make_shared<UsdContextOps>(item);
}

void UsdContextOps::setItem(const UsdSceneItem::Ptr& item)
{
    _item = item;

    // We only support bulk editing USD prims (so not on the gateway item).
    _bulkItems.clear();
    _bulkType.clear();
    if (item->runTimeId() == getUsdRunTimeId()) {
        // Determine if this ContextOps should be in bulk edit mode.
        if (auto globalSn = Ufe::GlobalSelection::get()) {
            if (globalSn->contains(item->path())) {
                // Only keep the selected items that match the runtime of the context item.
                _bulkType = _item->nodeType();
                const auto usdId = _item->runTimeId();
                for (auto&& selItem : *globalSn) {
                    if (selItem->runTimeId() == usdId) {
                        _bulkItems.append(selItem);
                        if (selItem->nodeType() != _bulkType)
                            _bulkType.clear();
                    }
                }

                // In order to be in bulk edit mode we need at least two items.
                // Our item plus one other.
                if (_bulkItems.size() == 1) {
                    _bulkItems.clear();
                    _bulkType.clear();
                }
            }
        }
    }
}

const Ufe::Path& UsdContextOps::path() const { return _item->path(); }

//------------------------------------------------------------------------------
// Ufe::ContextOps overrides
//------------------------------------------------------------------------------

Ufe::SceneItem::Ptr UsdContextOps::sceneItem() const { return _item; }

/*! Get the context ops items for the input item path.
 *
 *   This base class will build the following context menu:
 *
 *      Load
 *      Load with Descendants
 *      Unload
 *      Variant Sets -> submenu
 *      Make Invisible / Make Visible
 *      Deactivate Prim / Activate Prim
 *      Mark as Instanceable / Unmark as Instanceable
 *      Add New Prim -> submenu
 *         Def
 *         Scope
 *         Xform
 *         -------------------------
 *         Capsule
 *         Cone
 *         Cube
 *         Cylinder
 *         Sphere
 *         -------------------------
 *         All Registered -> submenu
 */
Ufe::ContextOps::Items UsdContextOps::getItems(const Ufe::ContextOps::ItemPath& itemPath) const
{
    if (isBulkEdit())
        return getBulkItems(itemPath);

    Ufe::ContextOps::Items items;
    if (itemPath.empty()) {
        const bool isClassPrim = prim().IsAbstract();

        if (!_isAGatewayType) {
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
            // If the item has the object3d interface, add menu item to change visibility.
            // Note: certain prim types such as shaders & materials don't support visibility.
            auto object3dHndlr = UsdObject3dHandler::create();
            if (object3dHndlr) {
                auto object3d = object3dHndlr->object3d(sceneItem());
                if (object3d) {
                    // Don't actually use UsdObject3d::visibility() - it looks at the authored
                    // visibility attribute. Instead, compute the effective visibility to decide on
                    // the label to use.
                    const auto imageable = UsdGeomImageable(prim());
                    const auto visibility
                        = imageable.ComputeVisibility() != UsdGeomTokens->invisible;
                    const auto label = visibility ? std::string(kUSDMakeInvisibleLabel)
                                                  : std::string(kUSDMakeVisibleLabel);
                    items.emplace_back(kUSDToggleVisibilityItem, label);
                }
            }

            // Default Prim:
            //     - If the prim is the default prim, add clearing the default prim
            //     - Otherwise, if the prim is a root prim, add set default prim
            if (!isClassPrim) {
                if (prim().GetStage()->GetDefaultPrim() == prim()) {
                    items.emplace_back(kUSDClearDefaultPrim, kUSDClearDefaultPrim);
                } else if (prim().GetPath().IsRootPrimPath()) {
                    items.emplace_back(kUSDSetAsDefaultPrim, kUSDSetAsDefaultPrim);
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
                                        : kUSDMarkAsInstanceabaleLabel);
        } // !_isAGatewayType

        // Top level item - Add New Prim (for all context op types).
        items.emplace_back(kUSDAddNewPrimItem, kUSDAddNewPrimLabel, Ufe::ContextItem::kHasChildren);
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
                items.emplace_back(kUSDClassPrimItem, kUSDClassPrimLabel, kUSDClassPrimImage);
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
#if PXR_VERSION >= 2208
                items.emplace_back(kUSDPlanePrimItem, kUSDPlanePrimLabel, kUSDPlanePrimImage);
#endif
                items.emplace_back(kUSDSpherePrimItem, kUSDSpherePrimLabel, kUSDSpherePrimImage);
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
                    schemaTypeGroups = getConcretePrimTypes(true, getSchemaPluginNiceNames());
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
            }
        }
    } // Top-level items
    return items;
}

// Adds the special Bulk Edit header as the first item.
void UsdContextOps::addBulkEditHeader(Ufe::ContextOps::Items& items) const
{
    assert(isBulkEdit());
    std::string bulkEditLabelStr;
    if (_bulkType.empty()) {
        bulkEditLabelStr = PXR_NS::TfStringPrintf(kBulkEditMixedTypeLabel, _bulkItems.size());
    } else {
        bulkEditLabelStr
            = PXR_NS::TfStringPrintf(kBulkEditSameTypeLabel, _bulkItems.size(), _bulkType.c_str());
    }
    Ufe::ContextItem bulkEditItem(kBulkEditItem, bulkEditLabelStr);
#ifdef UFE_V5_FEATURES_AVAILABLE
    // The position doesn't matter, it will always appear at the very top of the menu.
    bulkEditItem.setMetaData("isMenuHeader", true);
    items.emplace_back(bulkEditItem);
#else
    bulkEditItem.enabled = Ufe::ContextItem::kDisabled;
    // Insert the header (and separator) at the top of the menu.
    items.emplace(items.begin(), Ufe::ContextItem::kSeparator);
    items.emplace(items.begin(), bulkEditItem);
#endif
}

/*! Called when the context ops is in bulk edit mode.
 *
 *   This base class will build the following context menu:
 *
 *      "{countOfPrimsSelected} {PrimType} Prims Selected" - disbled item has no action
 *      -----------------
 *      Unload
 *      Load with Descendants
 *      Make Visible
 *      Make Invisible
 *      Activate Prim
 *      Deactivate Prim
 *      Mark as Instanceable
 *      Unmark as Instanceable
 */
Ufe::ContextOps::Items UsdContextOps::getBulkItems(const ItemPath& itemPath) const
{
    assert(isBulkEdit());
    Ufe::ContextOps::Items items;
    if (itemPath.empty()) {
        addBulkEditHeader(items);

        // Unload
        items.emplace_back(kUSDUnloadItem, kUSDUnloadLabel);

        // Load With Descendants
        items.emplace_back(kUSDLoadWithDescendantsItem, kUSDLoadWithDescendantsLabel);

        // Visibility:
        items.emplace_back(kUSDMakeVisibleItem, kUSDMakeVisibleLabel);
        items.emplace_back(kUSDMakeInvisibleItem, kUSDMakeInvisibleLabel);

        // Prim active state:
        items.emplace_back(kUSDActivatePrimItem, kUSDActivatePrimLabel);
        items.emplace_back(kUSDDeactivatePrimItem, kUSDDeactivatePrimLabel);

        // Instanceable:
        items.emplace_back(kUSDMarkAsInstanceabaleItem, kUSDMarkAsInstanceabaleLabel);
        items.emplace_back(kUSDUnmarkAsInstanceableItem, kUSDUnmarkAsInstanceableLabel);
    }
    return items;
}

Ufe::UndoableCommand::Ptr UsdContextOps::doOpCmd(const ItemPath& itemPath)
{
    // Empty argument means no operation was specified, error.
    if (itemPath.empty()) {
        TF_CODING_ERROR("Empty path means no operation was specified");
        return nullptr;
    }

    if (isBulkEdit())
        return doBulkOpCmd(itemPath);

    if (itemPath[0u] == kUSDLoadItem || itemPath[0u] == kUSDLoadWithDescendantsItem) {
        const UsdLoadPolicy policy = (itemPath[0u] == kUSDLoadWithDescendantsItem)
            ? UsdLoadWithDescendants
            : UsdLoadWithoutDescendants;
        return UsdUndoLongDurationCommand::create(
            { std::make_shared<UsdUndoLoadPayloadCommand>(prim(), policy) });
    } else if (itemPath[0u] == kUSDUnloadItem) {
        return UsdUndoLongDurationCommand::create(
            { std::make_shared<UsdUndoUnloadPayloadCommand>(prim()) });
    } else if (itemPath[0] == kUSDVariantSetsItem) {
        // Operation is to set a variant in a variant set.  Need both the
        // variant set and the variant as arguments to the operation.
        if (itemPath.size() != 3u) {
            TF_CODING_ERROR("Wrong number of arguments");
            return nullptr;
        }

        // At this point we know we have enough arguments to execute the
        // operation. If the prim is a DCC reference with auto-edit on,
        // the edit it instead of switching to the DCC reference.
        return std::make_shared<SetVariantSelectionCommand>(
            path(), prim(), itemPath[1], itemPath[2]);
    } // Variant sets
    else if (itemPath[0] == kUSDToggleVisibilityItem) {
        // Note: can use UsdObject3d::create() directly here since we know the item
        //       supports it (because we added the menu item).
        auto object3d = UsdObject3d::create(_item);
        if (!TF_VERIFY(object3d))
            return nullptr;
        // Don't use UsdObject3d::visibility() - it looks at the authored visibility
        // attribute. Instead, compute the effective visibility, which is what we want
        // to toggle.
        const auto imageable = UsdGeomImageable(prim());
        const auto current = imageable.ComputeVisibility() != UsdGeomTokens->invisible;
        return object3d->setVisibleCmd(!current);
    } // Visibility
    else if (itemPath[0] == kUSDToggleActiveStateItem) {
        return std::make_shared<UsdUfe::UsdUndoToggleActiveCommand>(prim());
    } // ActiveState
    else if (itemPath[0] == kUSDToggleInstanceableStateItem) {
        return std::make_shared<UsdUfe::UsdUndoToggleInstanceableCommand>(prim());
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
        return UsdUfe::UsdUndoSelectAfterCommand<UsdUfe::UsdUndoAddNewPrimCommand>::create(
            _item, primType, primType);
#else
        return UsdUfe::UsdUndoAddNewPrimCommand::create(_item, primType, primType);
#endif
    } else if (itemPath[0] == kUSDSetAsDefaultPrim) {
        return std::make_shared<UsdUndoSetDefaultPrimCommand>(prim());
    } else if (itemPath[0] == kUSDClearDefaultPrim) {
        return std::make_shared<UsdUndoClearDefaultPrimCommand>(prim());
    }
    return nullptr;
}

Ufe::UndoableCommand::Ptr UsdContextOps::doBulkOpCmd(const ItemPath& itemPath)
{
    assert(isBulkEdit());

    // List for the commands created (for CompositeUndoableCommand). If list
    // is empty return nullptr instead so nothing will be executed.
    std::list<Ufe::CompositeUndoableCommand::Ptr> cmdList;

#ifdef DEBUG
    auto DEBUG_OUTPUT = [&cmdList](const Ufe::Selection& bulkItems) {
        TF_STATUS(
            "Performing bulk edit on %d prims (%d selected)", cmdList.size(), bulkItems.size());
    };
#else
#define DEBUG_OUTPUT(x) (void)0
#endif

    auto compositeCmdReturn = [&cmdList](const Ufe::Selection& bulkItems) {
        DEBUG_OUTPUT(bulkItems);
        return !cmdList.empty() ? std::make_shared<Ufe::CompositeUndoableCommand>(cmdList)
                                : nullptr;
    };

    // Unload:
    if (itemPath[0u] == kUSDUnloadItem) {
        for (auto& selItem : _bulkItems) {
            auto usdItem = downcast(selItem);
            if (usdItem) {
                auto cmd = std::make_shared<UsdUndoUnloadPayloadCommand>(usdItem->prim());
                cmdList.emplace_back(cmd);
            }
        }
        return compositeCmdReturn(_bulkItems);
    }

    // Load With Descendants:
    if (itemPath[0u] == kUSDLoadWithDescendantsItem) {
        for (auto& selItem : _bulkItems) {
            UsdSceneItem::Ptr   usdItem = downcast(selItem);
            const UsdLoadPolicy policy = (itemPath[0u] == kUSDLoadWithDescendantsItem)
                ? UsdLoadWithDescendants
                : UsdLoadWithoutDescendants;
            if (usdItem) {
                auto cmd = std::make_shared<UsdUndoLoadPayloadCommand>(usdItem->prim(), policy);
                cmdList.emplace_back(cmd);
            }
        }
        return compositeCmdReturn(_bulkItems);
    }

    // Prim Visibility:
    const bool makeVisible = itemPath[0u] == kUSDMakeVisibleItem;
    const bool makeInvisible = itemPath[0u] == kUSDMakeInvisibleItem;
    if (makeVisible || makeInvisible) {
        // We know that all the bulk items are in the Usd runtime.
        auto object3dHndlr = UsdObject3dHandler::create();
        if (object3dHndlr) {
            for (auto& selItem : _bulkItems) {
                auto usdItem = downcast(selItem);
                if (usdItem) {
                    auto object3d = object3dHndlr->object3d(usdItem);
                    if (object3d) {
                        const auto imageable = UsdGeomImageable(usdItem->prim());
                        const auto isVisible
                            = imageable.ComputeVisibility() != UsdGeomTokens->invisible;
                        Ufe::UndoableCommand::Ptr cmd;
                        try {
                            // The UsdUndoVisibleCommand constructor will throw
                            // if attribute editing is blocked.
                            if (isVisible && makeInvisible) {
                                cmd = object3d->setVisibleCmd(false);
                                cmdList.emplace_back(cmd);
                            } else if (!isVisible && makeVisible) {
                                cmd = object3d->setVisibleCmd(true);
                                cmdList.emplace_back(cmd);
                            }
                        } catch (std::exception&) {
                            // Do nothing
                        }
                    }
                }
            }
        }
        return compositeCmdReturn(_bulkItems);
    }

    // Prim Active State:
    const bool makeActive = itemPath[0u] == kUSDActivatePrimItem;
    const bool makeInactive = itemPath[0u] == kUSDDeactivatePrimItem;
    if (makeActive || makeInactive) {
        for (auto& selItem : _bulkItems) {
            auto usdItem = downcast(selItem);
            if (usdItem) {
                auto       prim = usdItem->prim();
                const bool primIsActive = prim.IsActive();
                if ((makeActive && !primIsActive) || (makeInactive && primIsActive)) {
                    auto cmd = std::make_shared<UsdUfe::UsdUndoToggleActiveCommand>(prim);
                    cmdList.emplace_back(cmd);
                }
            }
        }
        return compositeCmdReturn(_bulkItems);
    }

    // Instanceable State:
    const bool markInstanceable = itemPath[0u] == kUSDMarkAsInstanceabaleItem;
    const bool unmarkInstanceable = itemPath[0u] == kUSDUnmarkAsInstanceableItem;
    if (markInstanceable || unmarkInstanceable) {
        for (auto& selItem : _bulkItems) {
            auto usdItem = downcast(selItem);
            if (usdItem) {
                auto       prim = usdItem->prim();
                const bool primIsInstanceable = prim.IsInstanceable();
                if ((markInstanceable && !primIsInstanceable)
                    || (unmarkInstanceable && primIsInstanceable)) {
                    auto cmd = std::make_shared<UsdUfe::UsdUndoToggleInstanceableCommand>(prim);
                    cmdList.emplace_back(cmd);
                }
            }
        }
        return compositeCmdReturn(_bulkItems);
    }

    return nullptr;
}

UsdContextOps::SchemaNameMap UsdContextOps::getSchemaPluginNiceNames() const
{
    // clang-format off
    static const SchemaNameMap schemaPluginNiceNames = {
        { "usdGeom", "Geometry" },
        { "usdLux", "Lighting" },
        { "usdMedia", "Media" },
        { "usdRender", "Render" },
        { "usdRi", "RenderMan" },
        { "usdPhysics", "Physics" },
        { "usdProc", "Procedural" },
        { "usdShade", "Shading" },
        { "usdSkel", "Skeleton" },
        { "usdUI", "UI" },
        { "usdVol", "Volumes" },
        { "usdArnold", "Arnold" }
    };
    // clang-format on
    return schemaPluginNiceNames;
}

USDUFE_VERIFY_CLASS_VIRTUAL_DESTRUCTOR(Ufe::CompositeUndoableCommand);
USDUFE_VERIFY_CLASS_BASE(
    UsdBulkEditCompositeUndoableCommand::Parent,
    UsdBulkEditCompositeUndoableCommand);

void UsdBulkEditCompositeUndoableCommand::execute()
{
    // Same as base class in forward order.
    // Iterate on our internal command list and only add to base class any commands
    // which succeed (no error thrown). Thus we do not need to override undo/redo.
    for (const auto& cmd : _cmds) {
        if (cmd) {
            try {
                cmd->execute();
                Parent::append(cmd);
            } catch (std::exception&) {
                // Do nothing
            }
        }
    }

    // Clear our internal list of commands since we added all the ones
    // that succeeded to the base class list (for undo/redo).
    _cmds.clear();
}

void UsdBulkEditCompositeUndoableCommand::addCommand(const Ptr& cmd)
{
    // Add the command to our own internal list. Later during execute
    // we'll add the ones that succeed to the base class list.
    _cmds.push_back(cmd);
}

} // namespace USDUFE_NS_DEF
