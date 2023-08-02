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
#include "MayaUsdContextOps.h"

#ifdef UFE_V3_FEATURES_AVAILABLE
#include <mayaUsd/commands/PullPushCommands.h>
#include <mayaUsd/fileio/primUpdaterManager.h>
#endif
#include <mayaUsd/ufe/UsdUndoMaterialCommands.h>
#include <mayaUsd/ufe/Utils.h>
#include <mayaUsd/utils/layers.h>
#include <mayaUsd/utils/util.h>
#include <mayaUsd/utils/utilFileSystem.h>

#include <usdUfe/ufe/UsdSceneItem.h>
#include <usdUfe/ufe/UsdUndoAddPayloadCommand.h>
#include <usdUfe/ufe/UsdUndoAddReferenceCommand.h>
#include <usdUfe/ufe/UsdUndoClearPayloadsCommand.h>
#include <usdUfe/ufe/UsdUndoClearReferencesCommand.h>
#include <usdUfe/ufe/UsdUndoPayloadCommand.h>

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
#include <pxr/usd/usd/stage.h>
#include <pxr/usd/usdShade/material.h>

#include <maya/MGlobal.h>
#include <ufe/globalSelection.h>
#include <ufe/hierarchy.h>
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
#ifdef UFE_V3_FEATURES_AVAILABLE
static constexpr char    kEditAsMayaItem[] = "Edit As Maya Data";
static constexpr char    kEditAsMayaLabel[] = "Edit As Maya Data";
static const std::string kEditAsMayaImage { "edit_as_Maya.png" };
static constexpr char    kDuplicateAsMayaItem[] = "Duplicate As Maya Data";
static constexpr char    kDuplicateAsMayaLabel[] = "Duplicate As Maya Data";
static constexpr char    kAddMayaReferenceItem[] = "Add Maya Reference";
static constexpr char    kAddMayaReferenceLabel[] = "Add Maya Reference...";
#endif
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
static constexpr char kAddRefOrPayloadLabel[] = "Add USD Reference/Payload...";
static constexpr char kAddRefOrPayloadItem[] = "AddReferenceOrPayload";
const constexpr char  kClearAllRefsOrPayloadsLabel[] = "Clear All USD References/Payloads...";
const constexpr char  kClearAllRefsOrPayloadsItem[] = "ClearAllReferencesOrPayloads";

//! \brief Change the cursor to wait state on construction and restore it on destruction.
struct WaitCursor
{
    WaitCursor() { MGlobal::executeCommand("waitCursor -state 1"); }
    ~WaitCursor() { MGlobal::executeCommand("waitCursor -state 0"); }
};

#ifdef UFE_V3_FEATURES_AVAILABLE
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

bool _prepareUSDReferenceTargetLayer(const UsdPrim& prim)
{
    static const bool useSceneFileForRoot = false;
    return UsdMayaUtilFileSystem::prepareLayerSaveUILayer(
        UsdUfe::getCurrentTargetLayer(prim), useSceneFileForRoot);
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
                -caption "Add USD Reference/Payload to Prim"
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

    const std::string layerDirPath = MayaUsd::getTargetLayerFolder(prim);
    auto relativePathAndSuccess = UsdMayaUtilFileSystem::makePathRelativeTo(filePath, layerDirPath);

    if (!relativePathAndSuccess.second) {
        TF_WARN(
            "File name (%s) cannot be resolved as relative to the current edit target layer, "
            "using the absolute path.",
            filePath.c_str());
    }

    return relativePathAndSuccess.first;
}

bool sceneItemSupportsShading(const Ufe::SceneItem::Ptr& sceneItem)
{
    if (MayaUsd::ufe::BindMaterialUndoableCommand::CompatiblePrim(sceneItem)) {
        return true;
    }
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

MayaUsdContextOps::MayaUsdContextOps(const UsdSceneItem::Ptr& item)
    : UsdUfe::UsdContextOps(item)
{
}

MayaUsdContextOps::~MayaUsdContextOps() { }

/*static*/
MayaUsdContextOps::Ptr MayaUsdContextOps::create(const UsdSceneItem::Ptr& item)
{
    return std::make_shared<MayaUsdContextOps>(item);
}

//------------------------------------------------------------------------------
// UsdUfe::UsdContextOps overrides
//------------------------------------------------------------------------------

Ufe::ContextOps::Items MayaUsdContextOps::getItems(const Ufe::ContextOps::ItemPath& itemPath) const
{
    // Get the items from our base class.
    const auto baseItems = UsdUfe::UsdContextOps::getItems(itemPath);

    Ufe::ContextOps::Items items;
    if (itemPath.empty()) {
        if (fItem->prim().IsA<UsdShadeMaterial>() && selectionSupportsShading()) {
            items.emplace_back(kBindMaterialToSelectionItem, kBindMaterialToSelectionLabel);
            items.emplace_back(Ufe::ContextItem::kSeparator);
        }
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

        // Add the items from our base class here
        items.insert(items.end(), baseItems.begin(), baseItems.end());

        if (!fIsAGatewayType) {
            items.emplace_back(kAddRefOrPayloadItem, kAddRefOrPayloadLabel);
            items.emplace_back(kClearAllRefsOrPayloadsItem, kClearAllRefsOrPayloadsLabel);
        }
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
    } else {
        // Add the items from our base class here
        items.insert(items.end(), baseItems.begin(), baseItems.end());

        if (itemPath[0] == BindMaterialUndoableCommand::commandName) {
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
    } // Top-level items
    return items;
}

Ufe::UndoableCommand::Ptr MayaUsdContextOps::doOpCmd(const ItemPath& itemPath)
{
    // Empty argument means no operation was specified, error.
    if (itemPath.empty()) {
        TF_CODING_ERROR("Empty path means no operation was specified");
        return nullptr;
    }

    // First check if our base class handles this item.
    auto cmd = UsdUfe::UsdContextOps::doOpCmd(itemPath);
    if (cmd)
        return cmd;

#ifdef WANT_QT_BUILD
    // When building without Qt there is no LayerEditor
    if (itemPath[0] == kUSDLayerEditorItem) {
        // Just open the editor directly and return null so we don't have undo.
        auto ufePath = ufe::stagePath(prim().GetStage());
        auto noWorld = ufePath.popHead().string();
        auto dagPath = UsdMayaUtil::nameToDagPath(noWorld);
        auto shapePath = dagPath.fullPathName();

        MString script;
        script.format("mayaUsdLayerEditorWindow -proxyShape ^1s mayaUsdLayerEditor", shapePath);
        MGlobal::executeCommand(script);
        return nullptr;
    }
#endif

    if (itemPath[0] == kAddRefOrPayloadItem) {
        if (!_prepareUSDReferenceTargetLayer(prim()))
            return nullptr;

        MString fileRef = MGlobal::executeCommandStringResult(_selectUSDFileScript());
        if (fileRef.length() == 0)
            return nullptr;

        const std::string path
            = makeUSDReferenceFilePathRelativeIfRequested(UsdMayaUtil::convert(fileRef), prim());
        if (path.empty())
            return nullptr;

        const bool asRef = UsdMayaUtilFileSystem::wantReferenceCompositionArc();
        const bool prepend = UsdMayaUtilFileSystem::wantPrependCompositionArc();
        if (asRef) {
            return std::make_shared<UsdUfe::UsdUndoAddReferenceCommand>(prim(), path, prepend);
        } else {
            Ufe::UndoableCommand::Ptr preloadCmd;
            const bool                preload = UsdMayaUtilFileSystem::wantPayloadLoaded();
            if (preload) {
                preloadCmd = std::make_shared<UsdUfe::UsdUndoLoadPayloadCommand>(
                    prim(), UsdLoadWithDescendants);
            } else {
                preloadCmd = std::make_shared<UsdUfe::UsdUndoUnloadPayloadCommand>(prim());
            }

            auto payloadCmd
                = std::make_shared<UsdUfe::UsdUndoAddPayloadCommand>(prim(), path, prepend);

            auto compoCmd = std::make_shared<Ufe::CompositeUndoableCommand>();
            compoCmd->append(preloadCmd);
            compoCmd->append(payloadCmd);

            return compoCmd;
        }
    } else if (itemPath[0] == kClearAllRefsOrPayloadsItem) {
        if (fItem->path().empty())
            return nullptr;
        MString itemName = fItem->path().back().string().c_str();

        MString cmd;
        cmd.format(
            "import mayaUsdClearRefsOrPayloadsOptions; "
            "mayaUsdClearRefsOrPayloadsOptions.showClearRefsOrPayloadsOptions(r'''^1s''')",
            itemName);
        MStringArray results;
        MGlobal::executePythonCommand(cmd, results);
        if (MString("Clear") != results[0])
            return nullptr;
        std::shared_ptr<Ufe::CompositeUndoableCommand> compositeCmd;
        for (const MString& res : results) {
            if (res == "references") {
                if (!compositeCmd) {
                    compositeCmd = std::make_shared<Ufe::CompositeUndoableCommand>();
                }
                compositeCmd->append(
                    std::make_shared<UsdUfe::UsdUndoClearReferencesCommand>(prim()));
            } else if (res == "payloads") {
                if (!compositeCmd) {
                    compositeCmd = std::make_shared<Ufe::CompositeUndoableCommand>();
                }
                compositeCmd->append(std::make_shared<UsdUfe::UsdUndoClearPayloadsCommand>(prim()));
            }
        }
        return compositeCmd;
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
        if (!_prepareUSDReferenceTargetLayer(prim()))
            return nullptr;

        MString script;
        script.format("addMayaReferenceToUsd \"^1s\"", Ufe::PathString::string(path()).c_str());
        MString result = MGlobal::executeCommandStringResult(
            script, /* display = */ false, /* undoable = */ true);
    }
#endif
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
    return nullptr;
}

UsdUfe::UsdContextOps::SchemaNameMap MayaUsdContextOps::getSchemaPluginNiceNames() const
{
    auto pluginNiceNames = Parent::getSchemaPluginNiceNames();

    // clang-format off
    static const SchemaNameMap mayaSchemaNiceNames = {
        { "mayaUsd_Schemas", "Maya Reference" },
        { "AL_USDMayaSchemasTest", "" }, // Skip legacy AL schemas
        { "AL_USDMayaSchemas", "" }      // Skip legacy AL schemas
    };
    // clang-format on

    pluginNiceNames.insert(mayaSchemaNiceNames.begin(), mayaSchemaNiceNames.end());
    return pluginNiceNames;
}

} // namespace ufe
} // namespace MAYAUSD_NS_DEF
