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
#include "Utils.h"

#include <ufe/attributes.h>
#include <ufe/attribute.h>

#include <pxr/usd/usd/variantSets.h>

#include <maya/MGlobal.h>

#include <cassert>

namespace {

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

}

MAYAUSD_NS_DEF {
namespace ufe {

UsdContextOps::UsdContextOps()
	: Ufe::ContextOps()
{
}

UsdContextOps::~UsdContextOps()
{
}

/*static*/
UsdContextOps::Ptr UsdContextOps::create()
{
	return std::make_shared<UsdContextOps>();
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
)
{
    Ufe::ContextOps::Items items;
    if (itemPath.empty()) {
        // Top-level items.  Variant sets, visibility, and list attributes.
        if (fPrim.HasVariantSets()) {
            Ufe::ContextItem variantSetsOp("Variant Sets", "Variant Sets", true);
            items.push_back(variantSetsOp);
        }
        auto attributes = Ufe::Attributes::attributes(sceneItem());
        if (attributes) {
            auto visibility =
                std::dynamic_pointer_cast<Ufe::AttributeEnumString>(
                    attributes->attribute("visibility"));
            if (visibility) {
                auto current = visibility->get();
                const std::string l = (current == "invisible") ?
                    std::string("Make Visible") : std::string("Make Invisible");
                Ufe::ContextItem visibilityOp("Toggle Visibility", l);
                items.push_back(visibilityOp);
            }
            Ufe::ContextItem listAttribsOp(
                "List Attributes", "List Attributes...");
            items.push_back(listAttribsOp);
        }
    }
    else {
        if (itemPath[0] == "Variant Sets") {
            UsdVariantSets varSets = fPrim.GetVariantSets();
            std::vector<std::string> varSetsNames;
            varSets.GetNames(&varSetsNames);

            if (itemPath.size() == 1u) {
                // Variant sets list.
                for (auto i = varSetsNames.crbegin(); i != varSetsNames.crend(); ++i) {
                    
                    items.push_back(Ufe::ContextItem(*i, *i, true));
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
                    const bool isSelected(vn == selected);
                    items.push_back(
                        Ufe::ContextItem(vn, vn, false, true, isSelected, true));
                }
            } // Variants of a variant set
        } // Variant sets
    } // Top-level items

    return items;
}

bool UsdContextOps::doOp(const ItemPath& itemPath)
{
    // Empty argument means no operation was specified, error.
    if (itemPath.empty()) {
        return false;
    }

    if (itemPath[0] == "Variant Sets") {
        // Operation is to set a variant in a variant set.  Need both the
        // variant set and the variant as arguments to the operation.
        if (itemPath.size() != 3u) {
            return false;
        }

        // At this point we know we have enough arguments to execute the
        // operation.  Get the variant set.
        UsdVariantSets varSets = fPrim.GetVariantSets();
        UsdVariantSet varSet = varSets.GetVariantSet(itemPath[1]);
        return varSet.SetVariantSelection(itemPath[2]);
    } // Variant sets
    else if (itemPath[0] == "Toggle Visibility") {
        auto attributes = Ufe::Attributes::attributes(sceneItem());
        assert(attributes);
        auto visibility = std::dynamic_pointer_cast<Ufe::AttributeEnumString>(
            attributes->attribute("visibility"));
        assert(visibility);
        auto current = visibility->get();
        visibility->set(current == "invisible" ? "inherited" : "invisible");
    } // Visibility
    else if (itemPath[0] == "List Attributes") {
        MGlobal::executePythonCommand("import maya.cmds as cmds");
        MGlobal::executePythonCommand("cmds.confirmDialog()");
    }

    return false;
}

Ufe::UndoableCommand::Ptr UsdContextOps::doOpCmd(const ItemPath& itemPath)
{
    // Empty argument means no operation was specified, error.
    if (itemPath.empty()) {
        return nullptr;
    }

    if (itemPath[0] == "Variant Sets") {
        // Operation is to set a variant in a variant set.  Need both the
        // variant set and the variant as arguments to the operation.
        if (itemPath.size() != 3u) {
            return nullptr;
        }

        // At this point we know we have enough arguments to execute the
        // operation.
        return std::make_shared<SetVariantSelectionUndoableCommand>(
            fPrim, itemPath);
    } // Variant sets
    else if (itemPath[0] == "Toggle Visibility") {
        auto attributes = Ufe::Attributes::attributes(sceneItem());
        assert(attributes);
        auto visibility = std::dynamic_pointer_cast<Ufe::AttributeEnumString>(
            attributes->attribute("visibility"));
        assert(visibility);
        auto current = visibility->get();
        return visibility->setCmd(
            current == "invisible" ? "inherited" : "invisible");
    } // Visibility
    else if (itemPath[0] == "List Attributes") {
        const char* script = R"(
import maya.cmds as cmds
from functools import partial

def dismissUSDListAttributes(data, msg):
    cmds.layoutDialog(dismiss=msg)

def buildUSDListAttributesDialog():
    form = cmds.setParent(q=True)

    # layoutDialog's are unfortunately not resizable, so hard code a size
    # here, to make sure all UI elements are visible.
    #
    cmds.formLayout(form, e=True, width=500)

    sceneItem = next(iter(ufe.GlobalSelection.get()))

    # Listing the attributes requires Attributes interface support.  This
    # should have been validated by the context ops interface.
    attr = ufe.Attributes.attributes(sceneItem)
    attrNames = attr.attributeNames

    attributesWidget = cmds.textField(editable=False, text='\n'.join(attrNames))

    okBtn = cmds.button(label='OK',
                        command=partial(dismissUSDListAttributes, msg='ok'))

    vSpc = 10
    hSpc = 10

    cmds.formLayout(
        form, edit=True,
        attachForm=[(attributesWidget, 'top',    vSpc),
                    (attributesWidget, 'left',   0),
                    (attributesWidget, 'right',  0),
                    (okBtn,            'bottom', vSpc),
                    (okBtn,            'right',  hSpc)])

cmds.layoutDialog(ui=buildUSDListAttributesDialog, title='Attributes')
)";
        MGlobal::executePythonCommand(script);
    }

    return nullptr;
}

} // namespace ufe
} // namespace MayaUsd
