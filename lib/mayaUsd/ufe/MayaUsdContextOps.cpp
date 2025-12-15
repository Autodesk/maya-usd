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
#include <mayaUsd/ufe/Global.h>
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
#include <usdUfe/ufe/UsdUndoReloadRefCommand.h>

#include <pxr/base/tf/diagnostic.h>
#include <pxr/base/tf/stringUtils.h>
#include <pxr/pxr.h>
#include <pxr/usd/sdf/fileFormat.h>
#include <pxr/usd/sdf/path.h>
#include <pxr/usd/sdr/registry.h>
#include <pxr/usd/usd/common.h>
#include <pxr/usd/usd/prim.h>
#include <pxr/usd/usd/stage.h>
#include <pxr/usd/usdShade/material.h>

#include <maya/MGlobal.h>
#include <ufe/globalSelection.h>
#include <ufe/observableSelection.h>
#include <ufe/path.h>
#include <ufe/pathString.h>
#include <ufe/selectionUndoableCommands.h>
#include <ufe/undoableCommand.h>

#ifdef LOOKDEVXUFE_HAS_LEGACY_MTLX_DETECTION
#include <LookdevXUfe/MaterialHandler.h>
#endif

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
static constexpr char    kEditAsMayaOptionsItem[] = "Edit As Maya Data Options";
static constexpr char    kEditAsMayaOptionsLabel[] = "Edit As Maya Data Options...";
static const std::string kEditAsMayaImage { "edit_as_Maya.png" };
static constexpr char    kDuplicateAsMayaItem[] = "Duplicate As Maya Data";
static constexpr char    kDuplicateAsMayaLabel[] = "Duplicate As Maya Data";
static constexpr char    kAddMayaReferenceItem[] = "Add Maya Reference";
static constexpr char    kAddMayaReferenceLabel[] = "Add Maya Reference...";
#endif
static constexpr char kBindMaterialToSelectionItem[] = "Assign Material to Selection";
static constexpr char kBindMaterialToSelectionLabel[] = "Assign Material to Selection";
#ifdef LOOKDEVXUFE_HAS_LEGACY_MTLX_DETECTION
static constexpr char kUpgradeMaterialItem[] = "Upgrade Material";
static constexpr char kUpgradeMaterialLabel[] = "Upgrade Material";
#endif
#ifdef UFE_V4_FEATURES_AVAILABLE
static constexpr char kAssignNewMaterialItem[] = "Assign New Material";
static constexpr char kAssignNewMaterialLabel[] = "Assign New Material";
static constexpr char kAddNewMaterialItem[] = "Add New Material";
static constexpr char kAddNewMaterialLabel[] = "Add New Material";
static constexpr char kAssignExistingMaterialItem[] = "Assign Existing Material";
static constexpr char kAssignExistingMaterialLabel[] = "Assign Existing Material";
#endif
static constexpr char kAddRefOrPayloadLabel[] = "Add...";
static constexpr char kAddRefOrPayloadItem[] = "AddReferenceOrPayload";
const constexpr char  kClearAllRefsOrPayloadsLabel[] = "Clear...";
const constexpr char  kClearAllRefsOrPayloadsItem[] = "ClearAllReferencesOrPayloads";
const constexpr char  kReloadReferenceLabel[] = "Reload";
const constexpr char  kReloadReferenceItem[] = "Reload";
static constexpr char kUSDReferenceItem[] = "Reference";
static constexpr char kUSDReferenceLabel[] = "Reference";

// Copied from UsdUfe::UsdContextOps
static constexpr char kUSDAddNewPrimItem[] = "Add New Prim";
static constexpr char kUSDClassPrimItem[] = "Class";

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

#ifdef LOOKDEVXUFE_HAS_LEGACY_MTLX_DETECTION
class UsdMxUpgradeStageCmd : public Ufe::CompositeUndoableCommand
{
public:
    using Ptr = std::shared_ptr<UsdMxUpgradeStageCmd>;

    static Ptr create(const Ufe::Path& stagePath);

    UsdMxUpgradeStageCmd(const Ufe::Path& stagePath);
    ~UsdMxUpgradeStageCmd() override = default;

    //@{
    //! Delete the copy/move constructors assignment operators.
    UsdMxUpgradeStageCmd(const UsdMxUpgradeStageCmd&) = delete;
    UsdMxUpgradeStageCmd& operator=(const UsdMxUpgradeStageCmd&) = delete;
    UsdMxUpgradeStageCmd(UsdMxUpgradeStageCmd&&) = delete;
    UsdMxUpgradeStageCmd& operator=(UsdMxUpgradeStageCmd&&) = delete;
    //@}

    constexpr static const char* kCommandString = "UpgradeStageLegacyMaterials";
    constexpr static const char* kCommandLabel = "Upgrade all legacy materials";

    UFE_V4(std::string commandString() const override { return kCommandString; })
};

UsdMxUpgradeStageCmd::Ptr UsdMxUpgradeStageCmd::create(const Ufe::Path& stagePath)
{
    auto materialHandler = LookdevXUfe::MaterialHandler::get(MayaUsd::ufe::getUsdRunTimeId());
    if (!materialHandler) {
        return {};
    }

    auto retVal = std::make_shared<UsdMxUpgradeStageCmd>(stagePath);

    // Could traverse the stage using only Ufe::Hierarchy, but I think going full USD is going to be
    // faster.
    auto stage = UsdUfe::getStage(stagePath);
    TF_AXIOM(stage);

    for (const auto& prim : stage->Traverse()) {
        if (const UsdShadeMaterial materialPrim = UsdShadeMaterial(prim); materialPrim) {
            // Recreate Ufe path:
            const PXR_NS::SdfPath& materialSdfPath = materialPrim.GetPath();
            const Ufe::Path materialUfePath = UsdUfe::usdPathToUfePathSegment(materialSdfPath);

            // Construct a UFE path consisting of two segments:
            // 1. The path to the USD stage
            // 2. The path to our material
            const auto  stagePathSegments = stagePath.getSegments();
            const auto& materialPathSegments = materialUfePath.getSegments();
            if (stagePathSegments.empty() || materialPathSegments.empty()) {
                continue;
            }

            const auto ufePath = Ufe::Path({ stagePathSegments[0], materialPathSegments[0] });

            // Now we have the full path to the material's SceneItem.
            auto cmd = materialHandler->upgradeLegacyShaderGraphCmd(
                UsdUfe::UsdSceneItem::create(ufePath, prim));
            if (cmd) {
                retVal->append(cmd);
            }
        }
    }

    if (retVal->cmdsList().empty()) {
        return nullptr;
    }
    return retVal;
}

UsdMxUpgradeStageCmd::UsdMxUpgradeStageCmd(const Ufe::Path& stagePath) { }

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
                -okCaption Reference
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
    const auto& layer = UsdUfe::getCurrentTargetLayer(prim);
    if (!layer) {
        return filePath;
    }

    if (!UsdMayaUtilFileSystem::requireUsdPathsRelativeToEditTargetLayer()) {
        UsdMayaUtilFileSystem::unmarkPathAsPostponedRelative(layer, filePath);
        return filePath;
    }

    if (layer->IsAnonymous()) {
        UsdMayaUtilFileSystem::markPathAsPostponedRelative(layer, filePath);
        return filePath;
    }

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

#ifdef UFE_V4_FEATURES_AVAILABLE
void addNewMaterialItems(const Ufe::ContextOps::ItemPath& itemPath, Ufe::ContextOps::Items& items)
{
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
        for (auto it = renderersAndMaterials.begin(), end = renderersAndMaterials.end(); it != end;
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
}

void assignExistingMaterialItems(
    const UsdUfe::UsdSceneItem::Ptr& item,
    const Ufe::ContextOps::ItemPath& itemPath,
    Ufe::ContextOps::Items&          items)
{
    std::multimap<std::string, MString> pathsAndMaterials;
    MStringArray                        materials;
    MString                             script;
    script.format(
        "mayaUsdGetMaterialsInStage \"^1s\"", Ufe::PathString::string(item->path()).c_str());
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
                MString materialName = it->second.substring(lastSlash + 1, it->second.length() - 1);
                items.emplace_back(it->second.asChar(), materialName.asChar());
            }
        }
    }
}
#endif

inline bool sceneItemSupportsShading(const Ufe::SceneItem::Ptr& sceneItem)
{
    return MayaUsd::ufe::BindMaterialUndoableCommand::CompatiblePrim(sceneItem);
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

#ifdef UFE_V4_FEATURES_AVAILABLE
bool canAssignMaterialToNodeType(const Ufe::SceneItem::Ptr& sceneItem)
{
    int     allowMaterialFunctions = 0;
    MString script;
    script.format(
        "mayaUsdMaterialBindings \"^1s\" -canAssignMaterialToNodeType true",
        Ufe::PathString::string(sceneItem->path()).c_str());
    MGlobal::executeCommand(script, allowMaterialFunctions);
    return (allowMaterialFunctions != 0);
}
#endif // UFE_V4_FEATURES_AVAILABLE

#ifdef UFE_V3_FEATURES_AVAILABLE

void executeEditAsMaya(const Ufe::Path& path)
{
    MString script;
    script.format(
        "^1s \"^2s\"",
        MayaUsd::ufe::EditAsMayaCommand::commandName,
        Ufe::PathString::string(path).c_str());
    UsdUfe::WaitCursor wait;
    MGlobal::executeCommand(script, /* display = */ true, /* undoable = */ true);
}

void executeEditAsMayaOptions(const Ufe::Path& path)
{
    // The edit as maya command name.
    const char editAsMayaOptionsCommand[] = "mayaUsdMenu_EditAsMayaDataOptions";
    MString    script;
    script.format("^1s \"^2s\"", editAsMayaOptionsCommand, Ufe::PathString::string(path).c_str());
    UsdUfe::WaitCursor wait;
    MGlobal::executeCommand(script, /* display = */ true, /* undoable = */ true);
}
#endif
} // namespace

namespace MAYAUSD_NS_DEF {
namespace ufe {

MAYAUSD_VERIFY_CLASS_SETUP(UsdUfe::UsdContextOps, MayaUsdContextOps);

MayaUsdContextOps::MayaUsdContextOps(const UsdUfe::UsdSceneItem::Ptr& item)
    : UsdUfe::UsdContextOps(item)
{
}

/*static*/
MayaUsdContextOps::Ptr MayaUsdContextOps::create(const UsdUfe::UsdSceneItem::Ptr& item)
{
    return std::make_shared<MayaUsdContextOps>(item);
}

//------------------------------------------------------------------------------
// UsdUfe::UsdContextOps overrides
//------------------------------------------------------------------------------

Ufe::ContextOps::Items MayaUsdContextOps::getItems(const Ufe::ContextOps::ItemPath& itemPath) const
{
    if (isBulkEdit())
        return getBulkItems(itemPath);

    // Get the items from our base class.
    const auto baseItems = Parent::getItems(itemPath);

    Ufe::ContextOps::Items items;
    if (itemPath.empty()) {
        bool needsSeparator = false;
        if (_item->prim().IsA<UsdShadeMaterial>() && selectionSupportsShading()) {
            items.emplace_back(kBindMaterialToSelectionItem, kBindMaterialToSelectionLabel);
            needsSeparator = true;
        }
#ifdef LOOKDEVXUFE_HAS_LEGACY_MTLX_DETECTION
        if (_item->prim().IsA<UsdShadeMaterial>()) {
            auto materialHandler = LookdevXUfe::MaterialHandler::get(path().runTimeId());
            if (materialHandler && materialHandler->isLegacyShaderGraph(sceneItem())) {
                items.emplace_back(kUpgradeMaterialItem, kUpgradeMaterialLabel);
                needsSeparator = true;
            }
        }
        if (_isAGatewayType) {
            if (UsdMxUpgradeStageCmd::create(path())) {
                items.emplace_back(
                    UsdMxUpgradeStageCmd::kCommandString, UsdMxUpgradeStageCmd::kCommandLabel);
                needsSeparator = true;
            }
        }
#endif
        if (needsSeparator) {
            items.emplace_back(Ufe::ContextItem::kSeparator);
        }
#ifdef WANT_QT_BUILD
        // Top-level item - USD Layer editor (for all context op types).
        // Only available when building with Qt enabled.
        items.emplace_back(kUSDLayerEditorItem, kUSDLayerEditorLabel, kUSDLayerEditorImage);
#endif

#ifdef UFE_V3_FEATURES_AVAILABLE
        const bool isClassPrim = prim().IsAbstract();
        const bool isMayaRef = (prim().GetTypeName() == TfToken("MayaReference"));
        if (!isClassPrim && !_isAGatewayType
            && PrimUpdaterManager::getInstance().canEditAsMaya(path())) {
            items.emplace_back(kEditAsMayaItem, kEditAsMayaLabel, kEditAsMayaImage);

            Ufe::ContextItem item(kEditAsMayaOptionsItem, kEditAsMayaOptionsLabel);
#ifdef UFE_CONTEXTOPS_HAS_OPTIONBOX
            item.setMetaData(Ufe::ContextItem::kIsOptionBox, true);
#endif
            items.emplace_back(item);
            if (!isMayaRef) {
                items.emplace_back(kDuplicateAsMayaItem, kDuplicateAsMayaLabel);
            }
        }
        if (!isMayaRef && !isClassPrim) {
            items.emplace_back(kAddMayaReferenceItem, kAddMayaReferenceLabel);
        }
        items.emplace_back(Ufe::ContextItem::kSeparator);
#endif

        // Add the items from our base class here
        items.insert(items.end(), baseItems.begin(), baseItems.end());

        if (!_isAGatewayType) {
            items.emplace_back(
                kUSDReferenceItem, kUSDReferenceLabel, Ufe::ContextItem::kHasChildren);
        }

        if (!_isAGatewayType) {
            // Top level item - Bind/unbind existing materials
            bool materialSeparatorsAdded = false;
            bool allowMaterialFunctions = false;
#ifdef UFE_V4_FEATURES_AVAILABLE
            allowMaterialFunctions = canAssignMaterialToNodeType(_item);
            if (allowMaterialFunctions && sceneItemSupportsShading(_item)) {
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
                MString      script;
                script.format(
                    "mayaUsdGetMaterialsInStage \"^1s\"",
                    Ufe::PathString::string(_item->path()).c_str());
                MGlobal::executeCommand(script, materials);
                if (materials.length() > 0) {
                    items.emplace_back(
                        kAssignExistingMaterialItem,
                        kAssignExistingMaterialLabel,
                        Ufe::ContextItem::kHasChildren);
                }
            }
#endif
            if (allowMaterialFunctions && _item->prim().HasAPI<UsdShadeMaterialBindingAPI>()) {
                UsdShadeMaterialBindingAPI bindingAPI(_item->prim());
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
            if (UsdUndoAddNewMaterialCommand::CompatiblePrim(_item)) {
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
            if (_item) {
                auto prim = _item->prim();
                if (prim) {
                    // Find materials in the global selection. Either directly selected or a direct
                    // child of the selection:
                    if (auto globalSn = Ufe::GlobalSelection::get()) {
                        // Use a set to keep names alphabetically ordered and unique.
                        std::set<std::string> foundMaterials;
                        for (auto&& selItem : *globalSn) {
                            auto usdItem = downcast(selItem);
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
            addNewMaterialItems(itemPath, items);
        } else if (itemPath[0] == kAssignExistingMaterialItem) {
            assignExistingMaterialItems(_item, itemPath, items);
#endif
        } else if (itemPath[0] == kUSDReferenceItem) {
            items.emplace_back(kAddRefOrPayloadItem, kAddRefOrPayloadLabel);
            auto prim = _item->prim();
            if (prim.HasAuthoredReferences() || prim.HasAuthoredPayloads()) {
                items.emplace_back(kReloadReferenceItem, kReloadReferenceLabel);
                items.emplace_back(kClearAllRefsOrPayloadsItem, kClearAllRefsOrPayloadsLabel);
            }
        }
    } // Top-level items
    return items;
}

Ufe::ContextOps::Items MayaUsdContextOps::getBulkItems(const ItemPath& itemPath) const
{
    // Get the items from our base class and append ours to that list.
    auto items = Parent::getBulkItems(itemPath);

    if (itemPath.empty()) {
        items.emplace_back(Ufe::ContextItem::kSeparator);

#ifdef UFE_V4_FEATURES_AVAILABLE
        // Assign New Material:
        items.emplace_back(
            kAssignNewMaterialItem, kAssignNewMaterialLabel, Ufe::ContextItem::kHasChildren);

        // Only show this option if we actually have materials in the stage.
        MStringArray materials;
        MString      script;
        script.format(
            "mayaUsdGetMaterialsInStage \"^1s\"", Ufe::PathString::string(_item->path()).c_str());
        MGlobal::executeCommand(script, materials);
        if (materials.length() > 0) {
            items.emplace_back(
                kAssignExistingMaterialItem,
                kAssignExistingMaterialLabel,
                Ufe::ContextItem::kHasChildren);
        }
#endif
#ifdef LOOKDEVXUFE_HAS_LEGACY_MTLX_DETECTION
        if (_item->prim().IsA<UsdShadeMaterial>()) {
            auto materialHandler = LookdevXUfe::MaterialHandler::get(path().runTimeId());
            if (materialHandler) {
                for (const auto& bulkItem : _bulkItems) {
                    if (materialHandler && materialHandler->isLegacyShaderGraph(bulkItem)) {
                        items.emplace_back(kUpgradeMaterialItem, kUpgradeMaterialLabel);
                        break;
                    }
                }
            }
        }
#endif
        items.emplace_back(
            UnbindMaterialUndoableCommand::commandName, UnbindMaterialUndoableCommand::commandName);
    } // top-level items
    else {
#ifdef UFE_V4_FEATURES_AVAILABLE
        if (itemPath[0] == kAssignNewMaterialItem) {
            addNewMaterialItems(itemPath, items);
        } else if (itemPath[0] == kAssignExistingMaterialItem) {
            assignExistingMaterialItems(_item, itemPath, items);
        }
#endif
    }

    return items;
}

Ufe::UndoableCommand::Ptr MayaUsdContextOps::doOpCmd(const ItemPath& itemPath)
{
    // Empty argument means no operation was specified, error.
    if (itemPath.empty()) {
        TF_CODING_ERROR("Empty path means no operation was specified");
        return nullptr;
    }

    if (isBulkEdit())
        return doBulkOpCmd(itemPath);

    // First check if our base class handles this item.
    auto cmd = Parent::doOpCmd(itemPath);
    if (cmd) {
        // EMSUSD-2499: Create Class Prim
        // Special case when adding a class prim via context menu make sure the Outliner
        // is displaying class prims.
        if (!itemPath.empty() && (itemPath[0] == kUSDAddNewPrimItem)) {
            // At this point we know the last item in the itemPath is the prim type to create
            auto primType = itemPath[itemPath.size() - 1];
            if (primType == kUSDClassPrimItem) {
                enableOutlinerClassPrims();
            }
        }

        return cmd;
    }

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

    if (itemPath.size() == 2u && itemPath[0] == kUSDReferenceItem) {
        if (itemPath[1] == kAddRefOrPayloadItem) {
            if (!_prepareUSDReferenceTargetLayer(prim()))
                return nullptr;

            MString fileRef = MGlobal::executeCommandStringResult(_selectUSDFileScript());
            if (fileRef.length() == 0)
                return nullptr;

            const std::string path = makeUSDReferenceFilePathRelativeIfRequested(
                UsdMayaUtil::convert(fileRef), prim());
            if (path.empty())
                return nullptr;

            const std::string primPath = UsdMayaUtilFileSystem::getReferencedPrimPath();
            const bool        asRef = UsdMayaUtilFileSystem::wantReferenceCompositionArc();
            const bool        prepend = UsdMayaUtilFileSystem::wantPrependCompositionArc();
            if (asRef) {
                return std::make_shared<UsdUfe::UsdUndoAddReferenceCommand>(
                    prim(), path, primPath, prepend);
            } else {
                Ufe::UndoableCommand::Ptr preloadCmd;
                const bool                preload = UsdMayaUtilFileSystem::wantPayloadLoaded();
                if (preload) {
                    preloadCmd = std::make_shared<UsdUfe::UsdUndoLoadPayloadCommand>(
                        prim(), UsdLoadWithDescendants);
                } else {
                    preloadCmd = std::make_shared<UsdUfe::UsdUndoUnloadPayloadCommand>(prim());
                }

                auto payloadCmd = std::make_shared<UsdUfe::UsdUndoAddPayloadCommand>(
                    prim(), path, primPath, prepend);

                auto compoCmd = std::make_shared<Ufe::CompositeUndoableCommand>();
                compoCmd->append(preloadCmd);
                compoCmd->append(payloadCmd);

                return compoCmd;
            }
        } else if (itemPath[1] == kClearAllRefsOrPayloadsItem) {
            if (_item->path().empty())
                return nullptr;
            MString itemName = _item->path().back().string().c_str();

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
                    compositeCmd->append(
                        std::make_shared<UsdUfe::UsdUndoClearPayloadsCommand>(prim()));
                }
            }
            return compositeCmd;
        } else if (itemPath[1] == kReloadReferenceItem) {
            if (_item->path().empty()) {
                return nullptr;
            }

            std::shared_ptr<Ufe::CompositeUndoableCommand> compositeCmd;
            if (!compositeCmd) {
                compositeCmd = std::make_shared<Ufe::CompositeUndoableCommand>();
            }
            compositeCmd->append(std::make_shared<UsdUfe::UsdUndoReloadRefCommand>(prim()));
            return compositeCmd;
        }
    }
#ifdef UFE_V3_FEATURES_AVAILABLE
    else if (itemPath[0] == kEditAsMayaItem) {
        executeEditAsMaya(path());
    } else if (itemPath[0] == kEditAsMayaOptionsItem) {
        executeEditAsMayaOptions(path());
    } else if (itemPath[0] == kDuplicateAsMayaItem) {
        // Note: empty string for target means Maya (hidden) world node.
        MString script;
        script.format(
            "^1s \"^2s\" \"\"",
            DuplicateCommand::commandName,
            Ufe::PathString::string(path()).c_str());
        UsdUfe::WaitCursor wait;
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
        return std::make_shared<BindMaterialUndoableCommand>(_item->path(), SdfPath(itemPath[1]));
    } else if (itemPath[0] == kBindMaterialToSelectionItem) {
        std::shared_ptr<Ufe::CompositeUndoableCommand> compositeCmd;
        if (auto globalSn = Ufe::GlobalSelection::get()) {
            for (auto&& selItem : *globalSn) {
                if (sceneItemSupportsShading(selItem)) {
                    if (!compositeCmd) {
                        compositeCmd = std::make_shared<Ufe::CompositeUndoableCommand>();
                    }
                    compositeCmd->append(std::make_shared<BindMaterialUndoableCommand>(
                        selItem->path(), _item->prim().GetPath()));
                }
            }
        }
        return compositeCmd;
#ifdef LOOKDEVXUFE_HAS_LEGACY_MTLX_DETECTION
    } else if (itemPath[0] == kUpgradeMaterialItem) {
        auto materialHandler = LookdevXUfe::MaterialHandler::get(path().runTimeId());
        if (materialHandler) {
            return materialHandler->upgradeLegacyShaderGraphCmd(sceneItem());
        }
    } else if (itemPath[0] == UsdMxUpgradeStageCmd::kCommandString) {
        return UsdMxUpgradeStageCmd::create(path());
#endif
    } else if (itemPath[0] == UnbindMaterialUndoableCommand::commandName) {
        return std::make_shared<UnbindMaterialUndoableCommand>(_item->path());
#ifdef UFE_V4_FEATURES_AVAILABLE
    } else if (itemPath.size() == 3u && itemPath[0] == kAssignNewMaterialItem) {
        // In single context item mode, only assign material to the context item.
        return std::make_shared<InsertChildAndSelectCommand>(
            UsdUndoAssignNewMaterialCommand::create(_item, itemPath[2]));
    } else if (itemPath.size() == 3u && itemPath[0] == kAddNewMaterialItem) {
        return std::make_shared<InsertChildAndSelectCommand>(
            UsdUndoAddNewMaterialCommand::create(_item, itemPath[2]));
    } else if (itemPath.size() == 3u && itemPath[0] == kAssignExistingMaterialItem) {
        // In single context item mode, only assign material to the context item.
        return std::make_shared<BindMaterialUndoableCommand>(_item->path(), SdfPath(itemPath[2]));
#endif
    }
    return nullptr;
}

Ufe::UndoableCommand::Ptr MayaUsdContextOps::doBulkOpCmd(const ItemPath& itemPath)
{
    // First check if our base class handles this item.
    auto cmd = Parent::doBulkOpCmd(itemPath);
    if (cmd)
        return cmd;

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

#ifdef UFE_V4_FEATURES_AVAILABLE
    if ((itemPath.size() == 3u) && (itemPath[0] == kAssignNewMaterialItem)) {
        // In the bulk edit mode, we only apply the action to the selected
        // items (not adding the item RMB if its outside the selection).
        return std::make_shared<InsertChildAndSelectCommand>(
            UsdUndoAssignNewMaterialCommand::create(_bulkItems, itemPath[2]));
    } else if (itemPath.size() == 3u && itemPath[0] == kAssignExistingMaterialItem) {
        for (auto& selItem : _bulkItems) {
            // The BindMaterialUndoableCommand will throw if given a prim type
            // it cannot handle. Don't let any exception escape here.
            try {
                if (sceneItemSupportsShading(selItem)) {
                    cmdList.emplace_back(std::make_shared<BindMaterialUndoableCommand>(
                        selItem->path(), SdfPath(itemPath[2])));
                }
            } catch (std::exception&) {
                // Do nothing
            }
        }
        return compositeCmdReturn(_bulkItems);
    }
#endif

    if (itemPath[0] == UnbindMaterialUndoableCommand::commandName) {
        for (auto& selItem : _bulkItems) {
            // Only execute this menu item on items that have a direct binding relationship.
            auto usdItem = downcast(selItem);
            if (usdItem) {
                auto prim = usdItem->prim();
                if (prim.HasAPI<UsdShadeMaterialBindingAPI>()) {
                    UsdShadeMaterialBindingAPI bindingAPI(prim);
                    auto                       directBinding = bindingAPI.GetDirectBinding();
                    if (directBinding.GetMaterial()) {
                        auto cmd = std::make_shared<UnbindMaterialUndoableCommand>(selItem->path());
                        cmdList.emplace_back(cmd);
                    }
                }
            }
        }
        return compositeCmdReturn(_bulkItems);
#ifdef LOOKDEVXUFE_HAS_LEGACY_MTLX_DETECTION
    } else if (itemPath[0] == kUpgradeMaterialItem) {
        auto bulkCmd = std::make_shared<Ufe::CompositeUndoableCommand>();
        auto materialHandler = LookdevXUfe::MaterialHandler::get(path().runTimeId());
        if (bulkCmd && materialHandler) {
            for (const auto& bulkItem : _bulkItems) {
                auto cmd = materialHandler->upgradeLegacyShaderGraphCmd(bulkItem);
                if (cmd) {
                    bulkCmd->append(cmd);
                }
            }
            return bulkCmd;
        }
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

void MayaUsdContextOps::enableOutlinerClassPrims() const
{
    // Enable the outliner to display class prims. The OutlinerHelper uses a static variable
    // to keep track of Outliner display. So we can set it on any OutlinerPanel.
    MString script("outlinerEditor -e -uf USD ClassPrims -ufv true outlinerPanel1");
    MGlobal::executeCommand(script, false /*display*/, true /*undoable*/);
}

} // namespace ufe
} // namespace MAYAUSD_NS_DEF
