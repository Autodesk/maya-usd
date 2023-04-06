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

#include "UsdUndoInsertChildCommand.h"

#include "Utils.h"
#include "private/UfeNotifGuard.h"
#include "private/Utils.h"

#include <mayaUsd/base/tokens.h>
#include <mayaUsd/utils/editRouter.h>
#include <mayaUsd/utils/layers.h>
#include <mayaUsd/utils/loadRules.h>
#include <mayaUsdUtils/util.h>

#include <pxr/base/tf/token.h>
#include <pxr/usd/sdf/copyUtils.h>
#include <pxr/usd/usd/editContext.h>
#include <pxr/usd/usd/stage.h>
#include <pxr/usd/usdGeom/gprim.h>
#include <pxr/usd/usdShade/material.h>
#include <pxr/usd/usdShade/nodeGraph.h>
#include <pxr/usd/usdShade/shader.h>

#include <ufe/log.h>
#include <ufe/pathString.h>
#include <ufe/scene.h>
#include <ufe/sceneNotification.h>

#define UFE_ENABLE_ASSERTS
#include <ufe/ufeAssert.h>

namespace {
// shared_ptr requires public ctor, dtor, so derive a class for it.
template <class T> struct MakeSharedEnabler : public T
{
    MakeSharedEnabler(
        const MayaUsd::ufe::UsdSceneItem::Ptr& parent,
        const MayaUsd::ufe::UsdSceneItem::Ptr& child,
        const MayaUsd::ufe::UsdSceneItem::Ptr& pos)
        : T(parent, child, pos)
    {
    }
};
} // namespace

namespace MAYAUSD_NS_DEF {
namespace ufe {

UsdUndoInsertChildCommand::UsdUndoInsertChildCommand(
    const UsdSceneItem::Ptr& parent,
    const UsdSceneItem::Ptr& child,
    const UsdSceneItem::Ptr& /* pos */)
    : Ufe::InsertChildCommand()
    , _ufeDstItem(nullptr)
    , _ufeSrcPath(child->path())
    , _ufeParentPath(parent->path())
    , _usdSrcPath(child->prim().GetPath())
{
    const auto& childPrim = child->prim();
    const auto& parentPrim = parent->prim();

    // Don't allow parenting to a Gprim.
    // USD strongly discourages parenting of one gprim to another.
    // https://graphics.pixar.com/usd/docs/USD-Glossary.html#USDGlossary-Gprim
    if (parentPrim.IsA<UsdGeomGprim>()) {
        std::string err = TfStringPrintf(
            "Parenting geometric prim [%s] under geometric prim [%s] is not allowed "
            "Please parent geometric prims under separate XForms and reparent between XForms.",
            childPrim.GetName().GetString().c_str(),
            parentPrim.GetName().GetString().c_str());
        throw std::runtime_error(err);
    }

    // UsdShadeShader can only have UsdShadeNodeGraph and UsdShadeMaterial as parent.
    if (childPrim.IsA<UsdShadeShader>() && !parentPrim.IsA<UsdShadeNodeGraph>()) {
        std::string err = TfStringPrintf(
            "Parenting Shader prim [%s] under %s prim [%s] is not allowed. "
            "Shader prims can only be parented under NodeGraphs and Materials.",
            childPrim.GetName().GetString().c_str(),
            parentPrim.GetTypeName().GetString().c_str(),
            parentPrim.GetName().GetString().c_str());
        throw std::runtime_error(err);
    }

    // UsdShadeNodeGraph can only have a UsdShadeNodeGraph and UsdShadeMaterial as parent.
    if (childPrim.IsA<UsdShadeNodeGraph>() && !childPrim.IsA<UsdShadeMaterial>()
        && !parentPrim.IsA<UsdShadeNodeGraph>()) {
        std::string err = TfStringPrintf(
            "Parenting NodeGraph prim [%s] under %s prim [%s] is not allowed. "
            "NodeGraph prims can only be parented under NodeGraphs and Materials.",
            childPrim.GetName().GetString().c_str(),
            parentPrim.GetTypeName().GetString().c_str(),
            parentPrim.GetName().GetString().c_str());
        throw std::runtime_error(err);
    }

    // UsdShadeMaterial cannot have UsdShadeShader, UsdShadeNodeGraph or UsdShadeMaterial as parent.
    if (childPrim.IsA<UsdShadeMaterial>()
        && (parentPrim.IsA<UsdShadeShader>() || parentPrim.IsA<UsdShadeNodeGraph>())) {
        std::string err = TfStringPrintf(
            "Parenting Material prim [%s] under %s prim [%s] is not allowed.",
            childPrim.GetName().GetString().c_str(),
            parentPrim.GetTypeName().GetString().c_str(),
            parentPrim.GetName().GetString().c_str());
        throw std::runtime_error(err);
    }

    // Reparenting directly under an instance prim is disallowed
    if (parentPrim.IsInstance()) {
        std::string err = TfStringPrintf(
            "Parenting geometric prim [%s] under instance prim [%s] is not allowed.",
            childPrim.GetName().GetString().c_str(),
            parentPrim.GetName().GetString().c_str());
        throw std::runtime_error(err);
    }

    // Apply restriction rules
    ufe::applyCommandRestriction(childPrim, "reparent");
    ufe::applyCommandRestriction(parentPrim, "reparent");

    _childLayer = childPrim.GetStage()->GetEditTarget().GetLayer();
    _parentLayer = getEditRouterLayer(MayaUsdEditRoutingTokens->RouteParent, parentPrim);
}

UsdUndoInsertChildCommand::~UsdUndoInsertChildCommand() { }

#ifdef UFE_V4_FEATURES_AVAILABLE
std::string UsdUndoInsertChildCommand::commandString() const
{
    return std::string("InsertChild ") + Ufe::PathString::string(_ufeSrcPath) + " "
        + Ufe::PathString::string(_ufeParentPath);
}
#endif

/*static*/
UsdUndoInsertChildCommand::Ptr UsdUndoInsertChildCommand::create(
    const UsdSceneItem::Ptr& parent,
    const UsdSceneItem::Ptr& child,
    const UsdSceneItem::Ptr& pos)
{
    if (!parent || !child) {
        return nullptr;
    }

    // Error if requested parent is currently a child of requested child.
    if (parent->path().startsWith(child->path())) {
        return nullptr;
    }

    return std::make_shared<MakeSharedEnabler<UsdUndoInsertChildCommand>>(parent, child, pos);
}

static void doUsdInsertion(
    const UsdStagePtr&    stage,
    const SdfLayerHandle& srcLayer,
    const SdfPath&        srcUsdPath,
    const SdfLayerHandle& dstLayer,
    const SdfPath&        dstUsdPath)
{
    SdfJustCreatePrimInLayer(dstLayer, dstUsdPath.GetParentPath());
    if (!SdfCopySpec(srcLayer, srcUsdPath, dstLayer, dstUsdPath)) {
        const std::string error = TfStringPrintf(
            "Insert child command: moving prim \"%s\" to \"%s\" with SdfCopySpec() failed.",
            srcUsdPath.GetString().c_str(),
            dstUsdPath.GetString().c_str());
        TF_WARN("%s", error.c_str());
        throw std::runtime_error(error);
    }
}

static UsdSceneItem::Ptr doInsertion(
    const SdfLayerHandle& srcLayer,
    const SdfPath&        srcUsdPath,
    const Ufe::Path&      srcUfePath,
    const SdfLayerHandle& dstLayer,
    const SdfPath&        dstUsdPath,
    const Ufe::Path&      dstUfePath)
{
    // We must retrieve the item every time we are called since it could be stale.
    // We need to get the USD prim from the UFE path.
    const UsdPrim     srcPrim = ufePathToPrim(srcUfePath);
    const UsdStagePtr stage = srcPrim.GetStage();

    enforceMutedLayer(srcPrim, "reparent");

    // Make sure the load state of the reparented prim will be preserved.
    // We copy all rules that applied to it specifically and remove the rules
    // that applied to it specifically.
    duplicateLoadRules(*stage, srcUsdPath, dstUsdPath);
    removeRulesForPath(*stage, srcUsdPath);

    // Do the insertion fro, the soufce layer to the target layer.
    doUsdInsertion(stage, srcLayer, srcUsdPath, dstLayer, dstUsdPath);

    // Do the insertion in all other applicable layers, which, due to the command
    // restrictions that have been verified when the command was created, should
    // only be session layers.
    PrimLayerFunc insertionFunc =
        [stage, srcUsdPath, dstUsdPath](const UsdPrim& prim, const PXR_NS::SdfLayerRefPtr& layer) {
            doUsdInsertion(stage, layer, srcUsdPath, layer, dstUsdPath);
        };

    const bool includeTopLayer = true;
    const auto sessionLayers = getAllSublayerRefs(stage->GetSessionLayer(), includeTopLayer);
    applyToSomeLayersWithOpinions(srcPrim, sessionLayers, insertionFunc);

    // Remove all scene descriptions for the source path and its subtree in the source layer.
    // Note: is the layer targeting really needed? We are removing the prim entirely.
    PrimLayerFunc removeFunc
        = [stage, srcUsdPath](const UsdPrim& prim, const PXR_NS::SdfLayerRefPtr& layer) {
              UsdEditContext ctx(stage, layer);
              if (!stage->RemovePrim(srcUsdPath)) {
                  const std::string error = TfStringPrintf(
                      "Insert child command: removing prim \"%s\" in layer \"%s\" failed.",
                      srcUsdPath.GetString().c_str(),
                      layer->GetDisplayName().c_str());
                  TF_WARN("%s", error.c_str());
                  throw std::runtime_error(error);
              }
          };

    applyToAllLayersWithOpinions(srcPrim, removeFunc);

    UsdSceneItem::Ptr dstItem = UsdSceneItem::create(dstUfePath, ufePathToPrim(dstUfePath));
    sendNotification<Ufe::ObjectReparent>(dstItem, srcUfePath);
    return dstItem;
}

void UsdUndoInsertChildCommand::insertChildRedo()
{
    if (_usdDstPath.IsEmpty()) {
        const auto& parentPrim = ufePathToPrim(_ufeParentPath);

        // First, check if we need to rename the child.
        const auto childName = uniqueChildName(parentPrim, _ufeSrcPath.back().string());

        // Create a new segment if parent and child are in different run-times.
        // parenting a USD node to the proxy shape node implies two different run-times
        auto cRtId = _ufeSrcPath.runTimeId();
        if (_ufeParentPath.runTimeId() == cRtId) {
            _ufeDstPath = _ufeParentPath + childName;
        } else {
            auto cSep = _ufeSrcPath.getSegments().back().separator();
            _ufeDstPath
                = _ufeParentPath + Ufe::PathSegment(Ufe::PathComponent(childName), cRtId, cSep);
        }
        _usdDstPath = parentPrim.GetPath().AppendChild(TfToken(childName));
    }

    // We need to keep the generated item to be able to return it to the caller
    // via the insertedChild() member function.
    _ufeDstItem = doInsertion(
        _childLayer, _usdSrcPath, _ufeSrcPath, _parentLayer, _usdDstPath, _ufeDstPath);
}

void UsdUndoInsertChildCommand::insertChildUndo()
{
    // Note: we don't need to keep the source item, we only need it to validate
    //       that the operation worked.
    doInsertion(_parentLayer, _usdDstPath, _ufeDstPath, _childLayer, _usdSrcPath, _ufeSrcPath);
}

void UsdUndoInsertChildCommand::undo()
{
    InPathChange pc;
    insertChildUndo();
}

void UsdUndoInsertChildCommand::redo()
{
    InPathChange pc;
    insertChildRedo();
}

} // namespace ufe
} // namespace MAYAUSD_NS_DEF
