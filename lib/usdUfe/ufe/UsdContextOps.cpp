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

#include <usdUfe/ufe/SetVariantSelectionCommand.h>
#include <usdUfe/ufe/UsdObject3dHandler.h>
#include <usdUfe/ufe/UsdSceneItem.h>
#include <usdUfe/ufe/UsdUndoAddNewPrimCommand.h>
#include <usdUfe/ufe/UsdUndoPayloadCommand.h>
#include <usdUfe/ufe/UsdUndoSelectAfterCommand.h>
#include <usdUfe/ufe/UsdUndoToggleActiveCommand.h>
#include <usdUfe/ufe/UsdUndoToggleInstanceableCommand.h>

#include <pxr/base/plug/plugin.h>
#include <pxr/base/plug/registry.h>
#include <pxr/usd/usd/variantSets.h>

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

static constexpr char kAllRegisteredTypesItem[] = "All Registered";
static constexpr char kAllRegisteredTypesLabel[] = "All Registered";

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
getConcretePrimTypes(bool sorted, const UsdContextOps::SchemaNameMap& schemaPluginNiceNames)
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

std::vector<SchemaTypeGroup> UsdContextOps::schemaTypeGroups = {};

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
    Ufe::ContextOps::Items items;
    if (itemPath.empty()) {
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
            // If the item has the object3d interface, add menu item to change visibility.
            // Note: certain prim types such as shaders & materials don't support visibility.
            auto object3dHndlr = UsdObject3dHandler::create();
            if (object3dHndlr) {
                auto object3d = object3dHndlr->object3d(sceneItem());
                if (object3d) {
                    // Don't actually use UsdObject3d::visibility() - it looks at the authored visibility
                    // attribute. Instead, compute the effective visibility to decide on the label to use.
                    const auto imageable = UsdGeomImageable(prim());
                    const auto visibility = imageable.ComputeVisibility() != UsdGeomTokens->invisible;
                    const auto label = visibility ? std::string(kUSDMakeInvisibleLabel)
                        : std::string(kUSDMakeVisibleLabel);
                    items.emplace_back(kUSDToggleVisibilityItem, label);
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

        return std::make_shared<UsdUndoLoadPayloadCommand>(prim(), policy);
    } else if (itemPath[0u] == kUSDUnloadItem) {
        return std::make_shared<UsdUndoUnloadPayloadCommand>(prim());
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
        auto object3d = UsdObject3d::create(fItem);
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
            fItem, primType, primType);
#else
        return UsdUfe::UsdUndoAddNewPrimCommand::create(fItem, primType, primType);
#endif
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
    };
    // clang-format on
    return schemaPluginNiceNames;
}

} // namespace USDUFE_NS_DEF
