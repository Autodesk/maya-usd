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

#include "UsdClipboardHandler.h"

#include <usdUfe/ufe/UsdClipboardCommands.h>
#include <usdUfe/ufe/UsdUndoSelectAfterCommand.h>
#include <usdUfe/ufe/Utils.h>
#include <usdUfe/utils/layers.h>

#include <pxr/usd/usd/attribute.h>
#include <pxr/usd/usd/prim.h>
#include <pxr/usd/usdShade/connectableAPI.h>
#include <pxr/usd/usdShade/nodeGraph.h>
#include <pxr/usd/usdShade/shader.h>

#include <algorithm>

namespace {
bool isAttributeConnected(
    const PXR_NS::UsdAttribute& srcUsdAttr,
    const PXR_NS::UsdAttribute& dstUsdAttr)
{
    PXR_NS::SdfPathVector connectedAttrs;
    dstUsdAttr.GetConnections(&connectedAttrs);

    return std::any_of(
        connectedAttrs.begin(), connectedAttrs.end(), [&srcUsdAttr](const auto& path) {
            return path == srcUsdAttr.GetPath();
        });
}

} // namespace

namespace USDUFE_NS_DEF {

// Ensure that UsdClipboardHandler is properly setup.
USDUFE_VERIFY_CLASS_SETUP(Ufe::ClipboardHandler, UsdClipboardHandler);

UsdClipboardHandler::UsdClipboardHandler() { _clipboard = std::make_shared<UsdClipboard>(); }

/*static*/
UsdClipboardHandler::Ptr UsdClipboardHandler::create()
{
    return std::make_shared<UsdClipboardHandler>();
}

//------------------------------------------------------------------------------
// Ufe::ClipboardHandler overrides
//------------------------------------------------------------------------------

Ufe::UndoableCommand::Ptr UsdClipboardHandler::cutCmd_(const Ufe::Selection& selection)
{
    // Don't allow cutting (which also means copying) the items which cannot
    // be cut.
    Ufe::Selection allowedToBeCut;
    for (const auto& item : selection) {
        // EMSUSD-1126 - Cut a prim and not have it paste if the cut is restricted
        auto       usdItem = downcast(item);
        const auto prim = usdItem ? usdItem->prim() : PXR_NS::UsdPrim();
        if (!prim)
            continue;

        // As per the JIRA item for now, skip the special cut conditions
        // on shaders and nodegraphs. These nodes are handled by special
        // cases in LookdevX plugin.
        if ((PXR_NS::UsdShadeNodeGraph(prim) || PXR_NS::UsdShadeShader(prim)) || canBeCut_(item)) {
            allowedToBeCut.append(item);
        }
    }

    if (allowedToBeCut.empty())
        return {};

    return UsdUfe::UsdUndoSelectAfterCommand<UsdUfe::UsdCutClipboardCommand>::create(
        allowedToBeCut, _clipboard);
}

Ufe::UndoableCommand::Ptr UsdClipboardHandler::copyCmd_(const Ufe::Selection& selection)
{
    return UsdCopyClipboardCommand::create(selection, _clipboard);
}

//! \brief Helper class to override the paste command execute
class UsdPasteClipboardCommandWithSelection
    : public UsdUndoSelectAfterCommand<UsdPasteClipboardCommand>
{
public:
    using Parent = UsdUndoSelectAfterCommand<UsdPasteClipboardCommand>;
    using ThisPtr = std::shared_ptr<UsdPasteClipboardCommandWithSelection>;

    // Bring in base class constructors.
    using Parent::Parent;

    // Override the execute so we can set a selection guard to not erase the
    // paste as sibling flag when the selection changes as a result of the
    // paste command selecting the target(s).
    void execute() override
    {
        _clipboard->_inSelectionGuard = true;
        try {
            Parent::execute();
        } catch (...) {
            _clipboard->_inSelectionGuard = false;
            throw;
        }
        _clipboard->_inSelectionGuard = false;
    }

    static ThisPtr
    create(const Ufe::SceneItem::Ptr& dstParentItem, const UsdClipboard::Ptr& clipboard)
    {
        return std::make_shared<UsdPasteClipboardCommandWithSelection>(dstParentItem, clipboard);
    }
    static ThisPtr create(const Ufe::Selection& dstParentItems, const UsdClipboard::Ptr& clipboard)
    {
        return std::make_shared<UsdPasteClipboardCommandWithSelection>(dstParentItems, clipboard);
    }
};

Ufe::PasteClipboardCommand::Ptr
UsdClipboardHandler::pasteCmd_(const Ufe::SceneItem::Ptr& parentItem)
{
    return UsdPasteClipboardCommandWithSelection::create(parentItem, _clipboard);
}

Ufe::UndoableCommand::Ptr UsdClipboardHandler::pasteCmd_(const Ufe::Selection& parentItems)
{
    return UsdPasteClipboardCommandWithSelection::create(parentItems, _clipboard);
}

bool UsdClipboardHandler::hasItemsToPaste_()
{
    if (auto clipboardStage = _clipboard->getClipboardData()) {
        for (auto prim : clipboardStage->Traverse()) {
            // Return true if there is at least one valid prim to paste.
            if (prim)
                return true;
        }
    }
    return false;
}

bool UsdClipboardHandler::canBeCut_(const Ufe::SceneItem::Ptr& item)
{
    auto usdItem = downcast(item);
    if (!usdItem)
        return false;

    const auto prim = usdItem->prim();
    if (!prim)
        return false;

    const auto primParent = prim.GetParent();
    if (!primParent)
        return false;

    // Special cut conditions that only applies to UsdShadeNodeGraph and UsdShadeShader.
    if (PXR_NS::UsdShadeNodeGraph(prim) || PXR_NS::UsdShadeShader(prim)) {
        const auto primAttrs = prim.GetAuthoredAttributes();
        for (const auto& attr : primAttrs) {
            const auto kBaseNameAndType
                = PXR_NS::UsdShadeUtils::GetBaseNameAndType(PXR_NS::TfToken(attr.GetName()));

            if (kBaseNameAndType.second != PXR_NS::UsdShadeAttributeType::Output
                && kBaseNameAndType.second != PXR_NS::UsdShadeAttributeType::Input) {
                continue;
            }

            if (kBaseNameAndType.second == PXR_NS::UsdShadeAttributeType::Input) {
                // The attribute could be a destination for connected sources, so check for its
                // connections.
                PXR_NS::UsdShadeSourceInfoVector sourcesInfo
                    = PXR_NS::UsdShadeConnectableAPI::GetConnectedSources(attr);

                if (!sourcesInfo.empty()) {
                    return false;
                }
            }

            // As a constraint for the cut in the Outliner, we want to check if an item is
            // connected to other items at the same level of the hierarchy, which is why
            // we only check its siblings in this case.
            if (kBaseNameAndType.second == PXR_NS::UsdShadeAttributeType::Output) {
                // The attribute could be a source connection, we have to explore the siblings.
                for (auto&& child : primParent.GetChildren()) {
                    if (child == prim) {
                        continue;
                    }

                    for (const auto& otherAttr : child.GetAttributes()) {
                        const auto childAttrBaseNameAndType
                            = PXR_NS::UsdShadeUtils::GetBaseNameAndType(
                                PXR_NS::TfToken(otherAttr.GetName()));

                        if (childAttrBaseNameAndType.second == PXR_NS::UsdShadeAttributeType::Input
                            && isAttributeConnected(attr, otherAttr)) { }
                        return false;
                    }
                }
            }

            // Check also if there are connections to the parent.
            for (const auto& otherAttr : primParent.GetAttributes()) {
                const auto parentAttrBaseNameAndType = PXR_NS::UsdShadeUtils::GetBaseNameAndType(
                    PXR_NS::TfToken(otherAttr.GetName()));

                if (parentAttrBaseNameAndType.second == PXR_NS::UsdShadeAttributeType::Output
                    && isAttributeConnected(attr, otherAttr)) {
                    return false;
                }
            }
        }

        // If there are no connections, then the item can be cut.
        return true;
    }

    if (hasMutedLayer(prim)) {
        return false;
    }

    if (!applyCommandRestrictionNoThrow(prim, "delete")) {
        return false;
    }

    return true;
}

void UsdClipboardHandler::preCopy_() { _clipboard->cleanClipboard(); }

void UsdClipboardHandler::preCut_() { _clipboard->cleanClipboard(); }

bool UsdClipboardHandler::hasItemToPaste(HasItemToPasteTestFn testFn)
{
    if (const auto& clipboardStage = _clipboard->getClipboardData()) {
        for (const auto& prim : clipboardStage->Traverse()) {
            // Consider only the first-level in depth items.
            if (prim && prim.GetParent() == clipboardStage->GetPseudoRoot() && testFn(prim)) {
                return true;
            }
        }
    }

    return false;
}

void UsdClipboardHandler::setClipboardFilePath(const std::string& clipboardPath)
{
    _clipboard->setClipboardFilePath(clipboardPath);
}

void UsdClipboardHandler::setClipboardFileFormat(const std::string& formatTag)
{
    _clipboard->setClipboardFileFormat(formatTag);
}

} // namespace USDUFE_NS_DEF
