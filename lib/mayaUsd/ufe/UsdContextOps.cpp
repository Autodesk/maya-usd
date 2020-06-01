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

#include <cassert>

#include <maya/MGlobal.h>

#include <ufe/attributes.h>
#include <ufe/attribute.h>
#include <ufe/path.h>

#include <pxr/usd/sdf/reference.h>
#include <pxr/usd/usd/references.h>
#include <pxr/usd/usd/variantSets.h>
#include <pxr/base/tf/diagnostic.h>
#include <pxr/usd/usdGeom/tokens.h>

#include <mayaUsd/utils/util.h>

#include <mayaUsd/ufe/Utils.h>
#include <mayaUsd/ufe/UsdSceneItem.h>

namespace {

// Ufe::ContextItem strings
// - the "Item" describe the operation to be performed.
// - the "Label" is used in the context menu (can be localized).
static constexpr char kUSDLayerEditorItem[] = "USD Layer Editor";
static constexpr char kUSDLayerEditorLabel[] = "USD Layer Editor...";
static constexpr char kUSDVariantSetsItem[] = "Variant Sets";
static constexpr char kUSDVariantSetsLabel[] = "Variant Sets";
static constexpr char kUSDToggleVisibilityItem[] = "Toggle Visibility";
static constexpr char kUSDMakeVisibleLabel[] = "Make Visible";
static constexpr char kUSDMakeInvisibleLabel[] = "Make Invisible";
static constexpr char kUSDAddNewPrimItem[] = "Add New Prim";
static constexpr char kUSDAddNewPrimLabel[] = "Add New Prim";
static constexpr char kUSDDefPrimItem[] = "Def";
static constexpr char kUSDDefPrimLabel[] = "Def";
static constexpr char kUSDScopePrimItem[] = "Scope";
static constexpr char kUSDScopePrimLabel[] = "Scope";
static constexpr char kUSDXformPrimItem[] = "Xform";
static constexpr char kUSDXformPrimLabel[] = "Xform";
static constexpr char kUSDCapsulePrimItem[] = "Capsule";
static constexpr char kUSDCapsulePrimLabel[] = "Capsule";
static constexpr char kUSDConePrimItem[] = "Cone";
static constexpr char kUSDConePrimLabel[] = "Cone";
static constexpr char kUSDCubePrimItem[] = "Cube";
static constexpr char kUSDCubePrimLabel[] = "Cube";
static constexpr char kUSDCylinderPrimItem[] = "Cylinder";
static constexpr char kUSDCylinderPrimLabel[] = "Cylinder";
static constexpr char kUSDSpherePrimItem[] = "Sphere";
static constexpr char kUSDSpherePrimLabel[] = "Sphere";

//! \brief Undoable command for variant selection change
class SetVariantSelectionUndoableCommand : public Ufe::UndoableCommand
{
public:
    
    SetVariantSelectionUndoableCommand(
        const UsdPrim&                   prim,
        const Ufe::ContextOps::ItemPath& itemPath
    ) : fVarSet(prim.GetVariantSets().GetVariantSet(itemPath[1])),
        fOldSelection(fVarSet.GetVariantSelection()),
        fNewSelection(itemPath[2])
    {}

    void undo() override { fVarSet.SetVariantSelection(fOldSelection); }

    void redo() override { fVarSet.SetVariantSelection(fNewSelection); }

private:

    UsdVariantSet     fVarSet;
    const std::string fOldSelection;
    const std::string fNewSelection;
};

//! \brief Undoable command for add new prim
class AddNewPrimUndoableCommand : public Ufe::UndoableCommand
{
public:
    AddNewPrimUndoableCommand(const MayaUsd::ufe::UsdSceneItem::Ptr& usdSceneItem,
                              const Ufe::ContextOps::ItemPath& itemPath)
    : Ufe::UndoableCommand()
    {
        // First get the stage from the proxy shape.
        auto ufePath = usdSceneItem->path();
        auto segments = ufePath.getSegments();
        auto dagSegment = segments[0];
        _stage = MayaUsd::ufe::getStage(Ufe::Path(dagSegment));
        if (_stage) {
            // Rename the new prim for uniqueness, if needed.
            // When we have a gateway type node, the prim we'll be adding will be
            // in a different runtime (USD vs Maya). So technically just appending
            // itemPath[1] isn't correct. But in this case it is only used by
            // uniqueChildName() which just takes the back of the path for child
            //  name, so it's fine.
            Ufe::Path newUfePath = ufePath + itemPath[1];
            auto newPrimName = uniqueChildName(usdSceneItem, newUfePath);

            // Build (and store) the path for the new prim with the unique name.
            PXR_NS::SdfPath usdItemPath = usdSceneItem->prim().GetPath();
            _primPath = usdItemPath.AppendChild(PXR_NS::TfToken(newPrimName));

            // The type of prim we were asked to create.
            // Note: "Def" means create typeless prim.
            _primToken = (itemPath[1] == kUSDDefPrimItem) ? TfToken() : TfToken(itemPath[1]);
        }
    }

    void undo() override
    {
        if (_stage) {
            _stage->RemovePrim(_primPath);
        }
    }

    void redo() override
    {
        if (_stage) {
            auto newPrim = _stage->DefinePrim(_primPath, _primToken);
            if (!newPrim.IsValid())
                TF_RUNTIME_ERROR("Failed to create new prim type: %s", _primToken.GetText());
        }
    }

private:
    PXR_NS::UsdStageWeakPtr _stage;
    PXR_NS::SdfPath _primPath;
    PXR_NS::TfToken _primToken;
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

    AddReferenceUndoableCommand(
        const UsdPrim& prim,
        const std::string& filePath
    ) : _prim(prim),
        _sdfRef(),
        _filePath(filePath)
    {}

    void undo() override { 
        if (_prim.IsValid()) {
            UsdReferences primRefs = _prim.GetReferences();
            primRefs.RemoveReference(_sdfRef);
        }
    }

    void redo() override { 
        if (_prim.IsValid()) {
            _sdfRef = SdfReference(_filePath);
            UsdReferences primRefs = _prim.GetReferences();
            primRefs.AddReference(_sdfRef);
        }
    }

private:
    UsdPrim _prim;
    SdfReference _sdfRef;
    const std::string _filePath;
};
const std::string AddReferenceUndoableCommand::commandName("Add Reference...");

class ClearAllReferencesUndoableCommand : public Ufe::UndoableCommand
{
public:
    static const std::string commandName;
    static const MString cancelRemoval;

    ClearAllReferencesUndoableCommand(
        const UsdPrim& prim
    ) : _prim(prim)
    {}

    void undo() override { 

    }

    void redo() override { 
        if (_prim.IsValid()) {
            UsdReferences primRefs = _prim.GetReferences();
            primRefs.ClearReferences();
        }
    }

private:
    UsdPrim _prim;
};
const std::string ClearAllReferencesUndoableCommand::commandName("Clear All References");
const MString ClearAllReferencesUndoableCommand::cancelRemoval("No");

}

MAYAUSD_NS_DEF {
namespace ufe {

UsdContextOps::UsdContextOps(const UsdSceneItem::Ptr& item)
	: Ufe::ContextOps(), fItem(item), fPrim(item->prim())
{
}

UsdContextOps::~UsdContextOps()
{
}

/*static*/
UsdContextOps::Ptr UsdContextOps::create(const UsdSceneItem::Ptr& item)
{
	return std::make_shared<UsdContextOps>(item);
}

void UsdContextOps::setItem(const UsdSceneItem::Ptr& item)
{
	fPrim = item->prim();
	fItem = item;
}

const Ufe::Path& UsdContextOps::path() const
{
	return fItem->path();
}

//------------------------------------------------------------------------------
// Ufe::ContextOps overrides
//------------------------------------------------------------------------------

Ufe::SceneItem::Ptr UsdContextOps::sceneItem() const
{
	return fItem;
}

Ufe::ContextOps::Items UsdContextOps::getItems(
    const Ufe::ContextOps::ItemPath& itemPath
) const
{
    Ufe::ContextOps::Items items;
    if (itemPath.empty()) {
        // Top-level item - USD Layer editor (for all context op types).
        int hasLayerEditorCmd{0};
        MGlobal::executeCommand("runTimeCommand -exists UsdLayerEditor", hasLayerEditorCmd);
        if (hasLayerEditorCmd) {
            items.emplace_back(kUSDLayerEditorItem, kUSDLayerEditorLabel);
#if UFE_PREVIEW_VERSION_NUM >= 2015
            items.emplace_back(Ufe::ContextItem::kSeparator);
#endif
        }

        // Top-level items.  Variant sets and visibility. Do not add for gateway type node.
        if (!fIsAGatewayType) {
            if (fPrim.HasVariantSets()) {
                items.emplace_back(
                    kUSDVariantSetsItem, kUSDVariantSetsLabel, Ufe::ContextItem::kHasChildren);
            }
            // If the item has a visibility attribute, add menu item to change visibility.
            // Note: certain prim types such as shaders & materials don't support visibility.
            auto attributes = Ufe::Attributes::attributes(sceneItem());
            if (attributes && attributes->hasAttribute(UsdGeomTokens->visibility)) {
                auto visibility =
                    std::dynamic_pointer_cast<Ufe::AttributeEnumString>(
                        attributes->attribute(UsdGeomTokens->visibility));
                if (visibility) {
                    auto current = visibility->get();
                    const std::string l = (current == UsdGeomTokens->invisible) ?
                        std::string(kUSDMakeVisibleLabel) : std::string(kUSDMakeInvisibleLabel);
                    items.emplace_back(kUSDToggleVisibilityItem, l);
                }
            }
        }

        // Top level item - Add New Prim (for all context op types).
        items.emplace_back(
            kUSDAddNewPrimItem, kUSDAddNewPrimLabel, Ufe::ContextItem::kHasChildren);

        if (!fIsAGatewayType) {
            items.emplace_back(AddReferenceUndoableCommand::commandName,
                                AddReferenceUndoableCommand::commandName);
            items.emplace_back(ClearAllReferencesUndoableCommand::commandName,
                                ClearAllReferencesUndoableCommand::commandName);            
        }
    }
    else {
        if (itemPath[0] == kUSDVariantSetsItem) {
            UsdVariantSets varSets = fPrim.GetVariantSets();
            std::vector<std::string> varSetsNames;
            varSets.GetNames(&varSetsNames);

            if (itemPath.size() == 1u) {
                // Variant sets list.
                for (auto i = varSetsNames.crbegin(); i != varSetsNames.crend(); ++i) {
                    items.emplace_back(*i, *i, Ufe::ContextItem::kHasChildren);
                }
            }
            else {
                // Variants of a given variant set.  Second item in the path is
                // the variant set name.
                assert(itemPath.size() == 2u);

                UsdVariantSet varSet = varSets.GetVariantSet(itemPath[1]);
                auto selected = varSet.GetVariantSelection();

                const auto varNames = varSet.GetVariantNames();
                for (const auto& vn : varNames) {
                    const bool checked(vn == selected);
                    items.emplace_back(vn, vn, Ufe::ContextItem::kNoChildren,
                                       Ufe::ContextItem::kCheckable, checked,
                                       Ufe::ContextItem::kExclusive);
                }
            } // Variants of a variant set
        } // Variant sets
        else if (itemPath[0] == kUSDAddNewPrimItem) {
            items.emplace_back(kUSDDefPrimItem, kUSDDefPrimLabel);  // typeless prim
            items.emplace_back(kUSDScopePrimItem, kUSDScopePrimLabel);
            items.emplace_back(kUSDXformPrimItem, kUSDXformPrimLabel);
#if UFE_PREVIEW_VERSION_NUM >= 2015
            items.emplace_back(Ufe::ContextItem::kSeparator);
#endif
            items.emplace_back(kUSDCapsulePrimItem, kUSDCapsulePrimLabel);
            items.emplace_back(kUSDConePrimItem, kUSDConePrimLabel);
            items.emplace_back(kUSDCubePrimItem, kUSDCubePrimLabel);
            items.emplace_back(kUSDCylinderPrimItem, kUSDCylinderPrimLabel);
            items.emplace_back(kUSDSpherePrimItem, kUSDSpherePrimLabel);
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

    if (itemPath[0] == kUSDVariantSetsItem) {
        // Operation is to set a variant in a variant set.  Need both the
        // variant set and the variant as arguments to the operation.
        if (itemPath.size() != 3u) {
            TF_CODING_ERROR("Wrong number of arguments");
            return nullptr;
        }

        // At this point we know we have enough arguments to execute the
        // operation.
        return std::make_shared<SetVariantSelectionUndoableCommand>(
            fPrim, itemPath);
    } // Variant sets
    else if (itemPath[0] == kUSDToggleVisibilityItem) {
        auto attributes = Ufe::Attributes::attributes(sceneItem());
        assert(attributes);
        auto visibility = std::dynamic_pointer_cast<Ufe::AttributeEnumString>(
            attributes->attribute(UsdGeomTokens->visibility));
        assert(visibility);
        auto current = visibility->get();
        return visibility->setCmd(
            current == UsdGeomTokens->invisible ? UsdGeomTokens->inherited : UsdGeomTokens->invisible);
    } // Visibility
    else if (!itemPath.empty() && (itemPath[0] == kUSDAddNewPrimItem)) {
        // Operation is to create a new prim of the type specified.
        if (itemPath.size() != 2u) {
            TF_CODING_ERROR("Wrong number of arguments");
            return nullptr;
        }

        // At this point we know we have 2 arguments to execute the operation.
        // itemPath[1] contains the new prim type to create.
        return std::make_shared<AddNewPrimUndoableCommand>(fItem, itemPath);
    }
    else if (itemPath[0] == kUSDLayerEditorItem) {
        // Just open the editor directly and return null so we don't have undo.
        MGlobal::executeCommand("UsdLayerEditor");
        return nullptr;
    }
    else if (itemPath[0] == AddReferenceUndoableCommand::commandName) {
        MString fileRef = MGlobal::executeCommandStringResult(selectUSDFileScript);

        std::string path = UsdMayaUtil::convert(fileRef);
        if (path.empty())
            return nullptr;

        return std::make_shared<AddReferenceUndoableCommand>(
            fPrim, path);        
    }
    else if (itemPath[0] == ClearAllReferencesUndoableCommand::commandName) {
        MString confirmation = MGlobal::executeCommandStringResult(clearAllReferencesConfirmScript);
        if (ClearAllReferencesUndoableCommand::cancelRemoval == confirmation)
            return nullptr;

        return std::make_shared<ClearAllReferencesUndoableCommand>(fPrim);
        
    }

    return nullptr;
}

} // namespace ufe
} // namespace MayaUsd
