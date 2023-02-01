//
// Copyright 2019 Autodesk
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
#include "UsdUndoDeleteCommand.h"

#include "private/UfeNotifGuard.h"
#include "private/Utils.h"

#include <mayaUsd/ufe/Utils.h>

#include <pxr/usd/pcp/layerStack.h>
#include <pxr/usd/sdf/layer.h>

#ifdef UFE_V2_FEATURES_AVAILABLE
#include <mayaUsd/ufe/UsdAttributes.h>
#include <mayaUsd/undo/UsdUndoBlock.h>
#endif

namespace {
#ifdef MAYA_ENABLE_NEW_PRIM_DELETE
bool hasLayersMuted(const PXR_NS::UsdPrim& prim)
{
    const PXR_NS::PcpPrimIndex& primIndex = prim.GetPrimIndex();

    for (const PXR_NS::PcpNodeRef node : primIndex.GetNodeRange()) {

        TF_AXIOM(node);

        const PXR_NS::PcpLayerStackSite&   site = node.GetSite();
        const PXR_NS::PcpLayerStackRefPtr& layerStack = site.layerStack;

        const std::set<std::string>& mutedLayers = layerStack->GetMutedLayers();
        if (mutedLayers.size() > 0) {
            return true;
        }
    }
    return false;
}
#endif
} // anonymous namespace

namespace MAYAUSD_NS_DEF {
namespace ufe {

UsdUndoDeleteCommand::UsdUndoDeleteCommand(const PXR_NS::UsdPrim& prim)
    : Ufe::UndoableCommand()
    , _prim(prim)
{
}

UsdUndoDeleteCommand::~UsdUndoDeleteCommand() { }

UsdUndoDeleteCommand::Ptr UsdUndoDeleteCommand::create(const PXR_NS::UsdPrim& prim)
{
    return std::make_shared<UsdUndoDeleteCommand>(prim);
}

#ifdef UFE_V2_FEATURES_AVAILABLE

void removeAttributesConnections(const PXR_NS::UsdPrim& prim)
{

    PXR_NS::UsdShadeConnectableAPI connectApi(prim);
    const auto                     kPrimParent = prim.GetParent();

    if (!connectApi || !kPrimParent) {
        return;
    }

    const PXR_NS::SdfPath kPrimPath = prim.GetPath();
    const auto            kPrimAttrs = prim.GetAttributes();

    for (const auto& attr : kPrimAttrs) {
        const PXR_NS::SdfPath kPropertyPath = kPrimPath.AppendProperty(attr.GetName());
        const auto            kBaseNameAndType
            = PXR_NS::UsdShadeUtils::GetBaseNameAndType(PXR_NS::TfToken(attr.GetName()));

        if (kBaseNameAndType.second != PXR_NS::UsdShadeAttributeType::Output
            && kBaseNameAndType.second != PXR_NS::UsdShadeAttributeType::Input) {
            continue;
        }

        if (kBaseNameAndType.second == PXR_NS::UsdShadeAttributeType::Input) {
            UsdShadeSourceInfoVector sourcesInfo = connectApi.GetConnectedSources(attr);

            if (!sourcesInfo.empty()) {
                // The attribute is the connection destination.
                const PXR_NS::UsdPrim connectedPrim = sourcesInfo[0].source.GetPrim();

                if (connectedPrim) {
                    const std::string prefix = connectedPrim == kPrimParent
                        ? PXR_NS::UsdShadeUtils::GetPrefixForAttributeType(
                            PXR_NS::UsdShadeAttributeType::Input)
                        : PXR_NS::UsdShadeUtils::GetPrefixForAttributeType(
                            PXR_NS::UsdShadeAttributeType::Output);

                    const std::string sourceName = prefix + sourcesInfo[0].sourceName.GetString();

                    auto srcAttr = connectedPrim.GetAttribute(PXR_NS::TfToken(sourceName));

                    if (srcAttr) {
                        UsdShadeConnectableAPI::DisconnectSource(attr, srcAttr);
                        // Check if we can remove also the property.
                        // Remove the property.
                    }
                }
            }
        }

        auto removeConnections
            = [](const PXR_NS::UsdPrim& prim, const PXR_NS::SdfPath& srcPropertyPath) {
                  for (const auto& attribute : prim.GetAttributes()) {
                      PXR_NS::UsdAttribute  attr = attribute.As<PXR_NS::UsdAttribute>();
                      PXR_NS::SdfPathVector sources;
                      attr.GetConnections(&sources);

                      for (size_t i = 0; i < sources.size(); ++i) {
                          if (sources[i] == srcPropertyPath) {
                              attr.RemoveConnection(srcPropertyPath);
                              // Check if we can remove also the property.
                              // Remove the property.
                              break;
                          }
                      }
                  }
              };

        if (kBaseNameAndType.second == PXR_NS::UsdShadeAttributeType::Output) {
            removeConnections(kPrimParent, kPropertyPath);
            for (const auto& kChildPrim : kPrimParent.GetChildren()) {
                if (kChildPrim != prim) {
                    removeConnections(kChildPrim, kPropertyPath);
                }
            }
        }
    }
}

void UsdUndoDeleteCommand::execute()
{
    if (!_prim.IsValid())
        return;

    MayaUsd::ufe::InAddOrDeleteOperation ad;

    UsdUndoBlock undoBlock(&_undoableItem);

#ifdef MAYA_ENABLE_NEW_PRIM_DELETE
    const auto& stage = _prim.GetStage();
    auto        targetPrimSpec = stage->GetEditTarget().GetPrimSpecForScenePath(_prim.GetPath());

    if (hasLayersMuted(_prim)) {
        TF_WARN("Cannot remove prim because there are muted layers.");
        return;
    }

    if (MayaUsd::ufe::applyCommandRestrictionNoThrow(_prim, "delete")) {
        removeAttributesConnections(_prim);
        auto retVal = stage->RemovePrim(_prim.GetPath());
        if (!retVal) {
            TF_VERIFY(retVal, "Failed to delete '%s'", _prim.GetPath().GetText());
        }
    }
#else
    _prim.SetActive(false);
#endif
}

void UsdUndoDeleteCommand::undo()
{
    MayaUsd::ufe::InAddOrDeleteOperation ad;

    _undoableItem.undo();
}

void UsdUndoDeleteCommand::redo()
{
    MayaUsd::ufe::InAddOrDeleteOperation ad;

    _undoableItem.redo();
}
#else
void UsdUndoDeleteCommand::perform(bool state)
{
    MayaUsd::ufe::InAddOrDeleteOperation ad;
    _prim.SetActive(state);
}

void UsdUndoDeleteCommand::undo() { perform(true); }

void UsdUndoDeleteCommand::redo() { perform(false); }
#endif

} // namespace ufe
} // namespace MAYAUSD_NS_DEF
