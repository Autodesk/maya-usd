//
// Copyright 2024 Autodesk
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

#include "UsdClipboardCommands.h"

#include <usdUfe/ufe/Global.h>
#include <usdUfe/ufe/UsdUndoAddNewPrimCommand.h>
#include <usdUfe/ufe/UsdUndoDuplicateSelectionCommand.h>
#include <usdUfe/ufe/Utils.h>
#include <usdUfe/undo/UsdUndoBlock.h>

#include <pxr/usd/usd/stage.h>
#include <pxr/usd/usdShade/material.h>
#include <pxr/usd/usdShade/nodeGraph.h>
#include <pxr/usd/usdShade/shader.h>

#include <ufe/globalSelection.h>
#include <ufe/observableSelection.h>
#include <ufe/runTimeMgr.h>
#include <ufe/sceneItemOps.h>
#include <ufe/selectionUndoableCommands.h>

namespace {

// Metadata used for pasting according to specific rules:
//  When we paste a shader under a scope, we first create a new material
//  (with the name of the original material) and then paste to it.
//  We could paste many shaders all together, and we want to group them
//  so that we take into consideration whether they are from the same
//  material and stage.
//
// For additional info see:
//      https://jira.autodesk.com/browse/LOOKDEVX-1639
//      https://jira.autodesk.com/browse/LOOKDEVX-1722
//
constexpr std::string_view kClipboardMetadata = "ClipboardMetadata";
constexpr std::string_view kMaterialName = "materialName";
constexpr std::string_view kNodeName = "shaderName";
constexpr std::string_view kStagePath = "stagePath";

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Utility functions
void setClipboardMetadata(
    UsdUfe::UsdSceneItem::Ptr& item,
    const std::string&         metadataKey,
    const std::string&         metadataValue)
{
    const auto setMetadataCmd = item->setGroupMetadataCmd(
        std::string(kClipboardMetadata), metadataKey, Ufe::Value(metadataValue));

    if (setMetadataCmd) {
        setMetadataCmd->execute();
    }
}

void setClipboardMetadata(
    const UsdUfe::UsdUndoDuplicateSelectionCommand::Ptr& duplicateSelectionCmd)
{
    for (const auto& duplicatedItem : duplicateSelectionCmd->getDuplicatedItemsMap()) {
        const auto prim = duplicatedItem.first->prim();

        // Do not set ClipboardMetadata for Materials.
        if (PXR_NS::UsdShadeMaterial(prim)) {
            continue;
        }

        std::string       materialName;
        const std::string stagePath = duplicatedItem.first->path().popSegment().string();
        const std::string nodeName = duplicatedItem.first->nodeName();

        // Recursively find the parent material.
        const auto parentMaterial = getParentMaterial(duplicatedItem.first);
        if (parentMaterial) {
            materialName = parentMaterial->nodeName();
        }

        // Get the usdItem for the copied item in the clipboard.
        Ufe::PathSegment segment(
            "/" + duplicatedItem.second->prim().GetName().GetString(),
            UsdUfe::getUsdRunTimeId(),
            '/');
        auto usdItem = UsdUfe::UsdSceneItem::create(segment, duplicatedItem.second->prim());

        // Set the node name in the ClipboardMetadata.
        setClipboardMetadata(usdItem, std::string(kNodeName), nodeName);

        // Set the material parent name in the ClipboardMetadata.
        setClipboardMetadata(usdItem, std::string(kMaterialName), materialName);

        // Set the stage path in the ClipboardMetadata.
        setClipboardMetadata(usdItem, std::string(kStagePath), stagePath);
    }
}

void clearClipboardMetadata(Ufe::SceneItemList& targetItems)
{
    std::shared_ptr<Ufe::CompositeUndoableCommand> compositeClearMetadataCmd
        = std::make_shared<Ufe::CompositeUndoableCommand>();

    // Remove ClipboardMetadata.
    for (auto& targetItem : targetItems) {
        auto usdItem = UsdUfe::downcast(targetItem);

        if (PXR_NS::UsdShadeMaterial(usdItem->prim()))
            continue;

        compositeClearMetadataCmd->append(
            usdItem->clearGroupMetadataCmd(std::string(kClipboardMetadata)));
    }

    compositeClearMetadataCmd->execute();
}

Ufe::SceneItem::Ptr renameItemUsingMetadata(Ufe::SceneItem::Ptr item)
{
    if (item) {
        auto usdItem = UsdUfe::downcast(item);
        if (usdItem) {
            const auto newName
                = usdItem->getGroupMetadata(std::string(kClipboardMetadata), std::string(kNodeName))
                      .get<std::string>();

            // Nothing to rename, return the item.
            if (item->nodeName() == newName || newName.empty()) {
                return item;
            }

            const Ufe::SceneItemOps::Ptr sceneItemOps = Ufe::SceneItemOps::sceneItemOps(item);
            if (sceneItemOps) {
                Ufe::PathComponent namePathComponent(newName);
                const auto renameCmd = sceneItemOps->renameItemCmdNoExecute(namePathComponent);
                if (renameCmd) {
                    renameCmd->execute();
                    return renameCmd->sceneItem();
                }
            }
        }
    }

    return {};
}

Ufe::SceneItemList
pasteItemsToNewMaterial(const UsdUfe::UsdSceneItem::Ptr& dstItem, Ufe::Selection& items)
{
    auto compositeCmd = Ufe::CompositeUndoableCommand::create({});

    // Preserve the scene item order from the selection.
    std::map<std::string, std::map<std::string, Ufe::Selection>> stageMaterialNamesMap;
    Ufe::SceneItemList                                           createdMaterials;

    // Group all the items with the same material name and from the same stage.
    for (const auto& item : items) {
        const auto materialName
            = item->getGroupMetadata(std::string(kClipboardMetadata), std::string(kMaterialName))
                  .get<std::string>();
        const auto originStage
            = item->getGroupMetadata(std::string(kClipboardMetadata), std::string(kStagePath))
                  .get<std::string>();

        stageMaterialNamesMap[originStage][materialName].append(item);
    }

    // Create the new Materials taking into consideration also the stage.
    // See metadata description above for rules on pasting.
    for (const auto& stagesNameMap : stageMaterialNamesMap) {
        for (const auto& materialNamesMap : stagesNameMap.second) {
            // Create a material using the name given from the metadata.
            // The uniqueness of the name will be solved by UsdUndoAddNewPrimCommand.
            auto createCmd = UsdUfe::UsdUndoAddNewPrimCommand::create(
                dstItem, materialNamesMap.first, "Material");
            if (createCmd) {
                createCmd->execute();

                // Use the created Material as paste target.
                if (createCmd->sceneItem()) {
                    createdMaterials.push_back(createCmd->sceneItem());
                    compositeCmd->append(UsdUfe::UsdUndoDuplicateSelectionCommand::create(
                        materialNamesMap.second, UsdUfe::downcast(createCmd->sceneItem())));
                }
            }
        }
    }

    // Execute the composite cmd.
    compositeCmd->execute();

    Ufe::SceneItemList pastedItems;
    for (const auto& materialItem : createdMaterials) {
        auto       usdItem = UsdUfe::downcast(materialItem);
        const auto matHierarchy = Ufe::Hierarchy::hierarchy(materialItem);
        if (matHierarchy) {
            for (auto& child : matHierarchy->children()) {
                // If necessary, rename the child using the name in the metadata.
                pastedItems.push_back(renameItemUsingMetadata(child));
            }
        }
    }

    // Clear the Clipboard metadata.
    clearClipboardMetadata(pastedItems);

    return createdMaterials;
}

} // namespace

namespace USDUFE_NS_DEF {

// Ensure that UsdCopyClipboardCommand is properly setup.
USDUFE_VERIFY_CLASS_SETUP(Ufe::UndoableCommand, UsdCopyClipboardCommand);

UsdCopyClipboardCommand::UsdCopyClipboardCommand(
    const Ufe::Selection&    selection,
    const UsdClipboard::Ptr& clipboard)
    : _selection(selection)
    , _clipboard(clipboard)
{
}

UsdCopyClipboardCommand::Ptr
UsdCopyClipboardCommand::create(const Ufe::Selection& selection, const UsdClipboard::Ptr& clipboard)
{
    if (selection.empty()) {
        return {};
    }

    return std::make_shared<UsdCopyClipboardCommand>(selection, clipboard);
}

void UsdCopyClipboardCommand::execute()
{
    // Create a new empty layer and stage for the clipboard.
    PXR_NS::SdfLayerRefPtr layer = PXR_NS::SdfLayer::CreateAnonymous();
    PXR_NS::UsdStageRefPtr clipboardStage
        = PXR_NS::UsdStage::Open(layer->GetIdentifier(), PXR_NS::UsdStage::LoadNone);

    if (!clipboardStage) {
        // It shouldn't be possible to obtain an invalid stage since we are creating it from an
        // anonymous layer, however as a precaution we leave this check.
        const std::string error = "Failed to create Clipboard stage.";
        throw std::runtime_error(error);
    }

    // Duplicate the selected items to the Clipboard stage using its pseudo-root as parent
    // item destination.
    auto usdParentItem = UsdSceneItem::create(Ufe::Path(), clipboardStage->GetPseudoRoot());
    auto duplicateSelectionUndoableCmd
        = UsdUndoDuplicateSelectionCommand::create(_selection, usdParentItem);
    duplicateSelectionUndoableCmd->execute();

    // Set Clipboard metadata.
    setClipboardMetadata(duplicateSelectionUndoableCmd);

    // Set the clipboard data.
    _clipboard->setClipboardData(clipboardStage);
}

void UsdCopyClipboardCommand::undo()
{
    // Do nothing.
}

void UsdCopyClipboardCommand::redo()
{
    // Do nothing.
}

// Ensure that UsdCutClipboardCommand is properly setup.
USDUFE_VERIFY_CLASS_SETUP(Ufe::UndoableCommand, UsdCutClipboardCommand);

UsdCutClipboardCommand::UsdCutClipboardCommand(
    const Ufe::Selection&    selection,
    const UsdClipboard::Ptr& clipboard)
    : _selection(selection)
    , _clipboard(clipboard)
{
}

UsdCutClipboardCommand::Ptr
UsdCutClipboardCommand::create(const Ufe::Selection& selection, const UsdClipboard::Ptr& clipboard)
{
    if (selection.empty()) {
        return {};
    }

    return std::make_shared<UsdCutClipboardCommand>(selection, clipboard);
}

void UsdCutClipboardCommand::execute()
{
    UsdUndoBlock undoBlock(&_undoableItem);

    // Step 1. Copy the selected items to the Clipboard.
    auto copyClipboardCommand = UsdCopyClipboardCommand::create(_selection, _clipboard);
    if (copyClipboardCommand)
        copyClipboardCommand->execute();

    const auto& sceneItemOpsHandler
        = Ufe::RunTimeMgr::instance().sceneItemOpsHandler(UsdUfe::getUsdRunTimeId());

    auto ufeSn = Ufe::GlobalSelection::get();

    // Step 2. Deselect then delete the selected items.
    for (auto&& item : _selection) {
        if (auto usdItem = downcast(item)) {
            Ufe::SelectionRemoveItem::createAndExecute(ufeSn, item);
            sceneItemOpsHandler->sceneItemOps(usdItem)->deleteItem();
        }
    }
}

void UsdCutClipboardCommand::undo() { _undoableItem.undo(); }

void UsdCutClipboardCommand::redo() { _undoableItem.redo(); }

Ufe::Selection getNewSelectionFromCommand(const UsdCutClipboardCommand& cmd)
{
    // Just keep the same selection (the cut items were already removed).
    Ufe::Selection sel(*Ufe::GlobalSelection::get());
    return sel;
}

// Ensure that UsdPasteClipboardCommand is properly setup.
USDUFE_VERIFY_CLASS_SETUP(Ufe::UndoableCommand, UsdPasteClipboardCommand);

UsdPasteClipboardCommand::UsdPasteClipboardCommand(
    const Ufe::Selection&    dstParentItems,
    const UsdClipboard::Ptr& clipboard)
    : _clipboard(clipboard)
{
    for (const auto& parentItem : dstParentItems) {
        if (const auto usdParentItem = downcast(parentItem)) {
            _dstParentItems.push_back(usdParentItem);
        }
    }
}

UsdPasteClipboardCommand::UsdPasteClipboardCommand(
    const Ufe::SceneItem::Ptr& dstParentItem,
    const UsdClipboard::Ptr&   clipboard)
    : _clipboard(clipboard)
{
    if (const auto usdItem = downcast(dstParentItem)) {
        _dstParentItems.push_back(usdItem);
    }
}

UsdPasteClipboardCommand::Ptr UsdPasteClipboardCommand::create(
    const Ufe::SceneItem::Ptr& dstParentItem,
    const UsdClipboard::Ptr&   clipboard)
{
    if (!dstParentItem)
        return {};
    return std::make_shared<UsdPasteClipboardCommand>(dstParentItem, clipboard);
}

UsdPasteClipboardCommand::Ptr UsdPasteClipboardCommand::create(
    const Ufe::Selection&    dstParentItems,
    const UsdClipboard::Ptr& clipboard)
{
    if (dstParentItems.empty())
        return {};
    return std::make_shared<UsdPasteClipboardCommand>(dstParentItems, clipboard);
}

void UsdPasteClipboardCommand::execute()
{
    UsdUndoBlock undoBlock(&_undoableItem);

    // Get the Clipboard stage.
    auto clipboardData = _clipboard->getClipboardData();

    if (!clipboardData) {
        const std::string error = "Failed to load Clipboard stage.";
        throw std::runtime_error(error);
    }

    // Duplicate the first-level in depth items from the Clipboard stage to the destination
    // parent item.
    Ufe::Selection clipboardMaterials;
    Ufe::Selection clipboardShaders;
    Ufe::Selection clipboardPrims;

    for (auto prim : clipboardData->Traverse()) {
        // Add to the selection only the first-level in depth items.
        if (prim.GetParent() == clipboardData->GetPseudoRoot()) {
            Ufe::PathSegment segment(
                "/" + prim.GetName().GetString(), UsdUfe::getUsdRunTimeId(), '/');

            if (auto usdItem = UsdSceneItem::create(segment, prim)) {
                if (PXR_NS::UsdShadeMaterial(prim)) {
                    clipboardMaterials.append(usdItem);
                } else if (PXR_NS::UsdShadeShader(prim) || PXR_NS::UsdShadeNodeGraph(prim)) {
                    clipboardShaders.append(usdItem);
                } else {
                    clipboardPrims.append(usdItem);
                }
            }
        }
    }

    if (clipboardPrims.empty() && clipboardMaterials.empty() && clipboardShaders.empty()) {
        return; // nothing to paste
    }

    auto appendToVector = [](const auto&             itemsToAppend,
                             std::vector<Ufe::Path>& vector,
                             bool                    useMetadataNames = false) {
        for (const auto& item : itemsToAppend) {
            auto itemPath = item->path();
            if (useMetadataNames) {
                // If we have the original name, use it.
                const auto newName
                    = item->getGroupMetadata(
                              std::string(kClipboardMetadata), std::string(kNodeName))
                          .template get<std::string>();
                if (!newName.empty() && newName != item->nodeName()) {
                    itemPath = itemPath.sibling(newName);
                }
            }

            vector.push_back(itemPath);
        }
    };

    // EMSUSD-1122 - As a user, I'd like to copy and then paste a prim as a sibling
    // If the selection hasn't changed between the copy and the paste, then we'll
    // paste into the parent of each destination parent item.
    // Note: for now only applies to regular prims (not the materials or shaders).
    bool pasteAsSibling = _clipboard->pasteAsSibling();

    for (const auto& dstParentItem : _dstParentItems) {
        Ufe::PasteClipboardCommand::PasteInfo pasteInfo;
        pasteInfo.pasteTarget = dstParentItem->path();

        if (!clipboardMaterials.empty()) {
            if (isMaterialsScope(dstParentItem)) {
                auto duplicateCmd
                    = UsdUndoDuplicateSelectionCommand::create(clipboardMaterials, dstParentItem);
                if (duplicateCmd) {
                    duplicateCmd->execute();
                    appendToVector(duplicateCmd->targetItems(), pasteInfo.successfulPastes);
                    auto tmpTargetItems = duplicateCmd->targetItems();
                    _targetItems.insert(
                        _targetItems.end(), tmpTargetItems.begin(), tmpTargetItems.end());
                }
            } else {
                appendToVector(clipboardMaterials, pasteInfo.failedPastes, true);
            }
        }

        if (!clipboardShaders.empty()) {
            // If the destination target is a Scope and we have shaders to paste, then before we
            // have to create a material using their clipboard metadata and use it as paste target.
            if (isMaterialsScope(dstParentItem)) {
                auto duplicatedItems = pasteItemsToNewMaterial(dstParentItem, clipboardShaders);
                appendToVector(duplicatedItems, pasteInfo.successfulPastes);
                _targetItems.insert(
                    _targetItems.end(), duplicatedItems.begin(), duplicatedItems.end());
            } else if (PXR_NS::UsdShadeNodeGraph(dstParentItem->prim())) {
                auto duplicateCmd
                    = UsdUndoDuplicateSelectionCommand::create(clipboardShaders, dstParentItem);
                if (duplicateCmd) {
                    duplicateCmd->execute();
                    appendToVector(duplicateCmd->targetItems(), pasteInfo.successfulPastes);
                    auto tmpTargetItems = duplicateCmd->targetItems();
                    _targetItems.insert(
                        _targetItems.end(), tmpTargetItems.begin(), tmpTargetItems.end());
                }
            } else {
                appendToVector(clipboardShaders, pasteInfo.failedPastes, true);
            }
        }

        if (!clipboardPrims.empty()) {
            UsdSceneItem::Ptr pasteTarget = dstParentItem;
            if (pasteAsSibling) {
                auto hier = Ufe::Hierarchy::hierarchy(dstParentItem);
                auto parentItem = hier ? hier->parent() : nullptr;
                auto newPasteTarget = downcast(parentItem);
                if (newPasteTarget) {
                    pasteTarget = newPasteTarget;
                } else {
                    pasteAsSibling = false;
                }
            }

            // When pasting as sibling we have changed the paste target to the parent
            // which means we might have the same parent as one of the other paste targets.
            // We don't want to paste twice to that same parent.
            bool skipThisTarget = false;
            for (const auto& prevPasteInto : _pasteInfos) {
                if (prevPasteInto.pasteTarget == pasteTarget->path()) {
                    skipThisTarget = true;
                    break;
                }
            }

            if (!skipThisTarget) {
                // When pasting as sibling we'll use an extra paste info since
                // the paste target is different.
                Ufe::PasteClipboardCommand::PasteInfo extraPasteInfo;
                if (pasteAsSibling)
                    extraPasteInfo.pasteTarget = pasteTarget->path();

                auto duplicateCmd
                    = UsdUndoDuplicateSelectionCommand::create(clipboardPrims, pasteTarget);
                if (duplicateCmd) {
                    duplicateCmd->execute();
                    appendToVector(
                        duplicateCmd->targetItems(),
                        pasteAsSibling ? extraPasteInfo.successfulPastes
                                       : pasteInfo.successfulPastes);
                    auto tmpTargetItems = duplicateCmd->targetItems();
                    _targetItems.insert(
                        _targetItems.end(), tmpTargetItems.begin(), tmpTargetItems.end());
                    _itemsToSelect.insert(
                        _itemsToSelect.end(), tmpTargetItems.begin(), tmpTargetItems.end());
                }
                if (pasteAsSibling) {
                    _pasteInfos.push_back(std::move(extraPasteInfo));
                }
            }
        }

        // Add the paste info.
        _pasteInfos.push_back(std::move(pasteInfo));
    }

    if (!pasteAsSibling) {
        _clipboard->setPasteAsSibling();
    }

    // Remove ClipboardMetadata.
    clearClipboardMetadata(_targetItems);
}

void UsdPasteClipboardCommand::undo() { _undoableItem.undo(); }

void UsdPasteClipboardCommand::redo() { _undoableItem.redo(); }

Ufe::SceneItemList UsdPasteClipboardCommand::targetItems() const { return _targetItems; }

std::vector<Ufe::PasteClipboardCommand::PasteInfo> UsdPasteClipboardCommand::getPasteInfos() const
{
    return _pasteInfos;
}

Ufe::Selection getNewSelectionFromCommand(const UsdPasteClipboardCommand& cmd)
{
    Ufe::Selection newSelection;
    if (!cmd.itemsToSelect().empty()) {
        // Select the newly pasted items. This matches native DCC (ex: Maya) behavior.
        for (const auto& item : cmd.itemsToSelect()) {
            newSelection.append(item);
        }
    }
    return newSelection;
}

} // namespace USDUFE_NS_DEF
