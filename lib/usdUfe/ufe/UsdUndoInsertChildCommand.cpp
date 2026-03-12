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

#include <usdUfe/base/tokens.h>
#include <usdUfe/ufe/UfeNotifGuard.h>
#include <usdUfe/ufe/Utils.h>
#include <usdUfe/undo/UsdUndoBlock.h>
#include <usdUfe/utils/editRouter.h>
#include <usdUfe/utils/editRouterContext.h>
#include <usdUfe/utils/layers.h>
#include <usdUfe/utils/loadRules.h>
#include <usdUfe/utils/mergePrims.h>
#include <usdUfe/utils/usdUtils.h>

#include <pxr/base/tf/token.h>
#include <pxr/usd/sdf/copyUtils.h>
#ifdef USD_HAS_NAMESPACE_EDIT
#include <pxr/usd/sdf/namespaceEdit.h>
#endif
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

#include <functional>
#include <vector>

#define UFE_ENABLE_ASSERTS
#include <ufe/ufeAssert.h>

namespace {
// shared_ptr requires public ctor, dtor, so derive a class for it.
template <class T> struct MakeSharedEnabler : public T
{
    MakeSharedEnabler(
        const UsdUfe::UsdSceneItem::Ptr& parent,
        const UsdUfe::UsdSceneItem::Ptr& child,
        const UsdUfe::UsdSceneItem::Ptr& pos)
        : T(parent, child, pos)
    {
    }
};

// Validate that a prim can be reparented in a component stage.
// Components only allow reparenting prims whose path is contained within
// either the material or geometry scope
void validateComponentReparent(const UsdPrim& prim)
{
    const UsdStagePtr stage = prim.GetStage();
    if (!stage) {
        return;
    }

    // Get the prim path
    const SdfPath primPath = prim.GetPath();

    // Get the default prim to construct full scope paths
    auto defaultPrim = stage->GetDefaultPrim();
    if (!defaultPrim) {
        return;
    }
    const SdfPath defaultPrimPath = defaultPrim.GetPath();

    // Get material and mesh scope names from component
    std::string materialScopeName = UsdUfe::getComponentMaterialScopeName(stage);
    std::string meshScopeName = UsdUfe::getComponentMeshScopeName(stage);

    // Build full scope paths
    std::vector<SdfPath> scopePaths;
    if (!materialScopeName.empty()) {
        scopePaths.push_back(defaultPrimPath.AppendChild(PXR_NS::TfToken(materialScopeName)));
    }
    if (!meshScopeName.empty()) {
        scopePaths.push_back(defaultPrimPath.AppendChild(PXR_NS::TfToken(meshScopeName)));
    }

    // Check if the prim path is contained within any of the scope paths
    for (const SdfPath& scopePath : scopePaths) {
        // Check if prim path is not equal to the scope_path and a descendant of the scope path
        if (primPath != scopePath && primPath.HasPrefix(scopePath)) {
            return;
        }
    }

    // Disallow - prim is not in component scopes
    const std::string error = TfStringPrintf(
        "Cannot reparent prim \"%s\" in a component stage. "
        "Only prims within component material or mesh scopes can be reparented. ",
        prim.GetPath().GetText());
    TF_WARN("%s", error.c_str());
    throw std::runtime_error(error);
}

} // namespace

namespace USDUFE_NS_DEF {

class EditForwardingGuard
{
public:
    EditForwardingGuard()
    {
        // Pause edit forwarding
        pauseEditForwarding(true);
    }

    ~EditForwardingGuard()
    {
        // Unpause edit forwarding
        pauseEditForwarding(false);
    }
};

USDUFE_VERIFY_CLASS_SETUP(Ufe::InsertChildCommand, UsdUndoInsertChildCommand);

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

    // Check component-specific restrictions
    if (isComponentStage(child->path())) {
        // Components only allow reparenting prims from payloads
        validateComponentReparent(childPrim);
    } else {
        // Apply restriction rules for non-component stages
        UsdUfe::applyCommandRestriction(childPrim, "reparent");
        // Note: the parent is only receiving the prim, so it can be declared
        //       in a weaker layer.
        const bool allowStronger = true;
        UsdUfe::applyCommandRestriction(parentPrim, "reparent", allowStronger);
    }
}

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

// Type alias for the removal function
using PrimSpecRemovalFn = std::function<void(const SdfPrimSpecHandle&, const UsdStageRefPtr&)>;

static void doInsertion(
    const SdfPath&    srcUsdPath,
    const Ufe::Path&  srcUfePath,
    const SdfPath&    dstUsdPath,
    const Ufe::Path&  dstUfePath,
    bool              isComponent,
    PrimSpecRemovalFn removalFn)
{
    UsdUfe::InAddOrDeleteOperation ad;

    UsdPrim        srcPrim = ufePathToPrim(srcUfePath);
    UsdStageRefPtr stage = srcPrim.GetStage();

    // Enforce the edit routing for the insert-child command in order to find
    // the target layer. The edit router context sets the edit target of the
    // stage of the given prim, if it gets routed.
    OperationEditRouterContext ctx(UsdUfe::EditRoutingTokens->RouteParent, srcPrim);
    const SdfLayerHandle&      dstLayer = stage->GetEditTarget().GetLayer();

    enforceMutedLayer(srcPrim, "reparent");

    // TODO: the replication of extra information is missing in UsdUfe.
    //       In MayaUsd, MayaUsd::ufe::ReplicateExtrasToUSD duplicates
    //       the Maya display layer information. This is not supported
    //       in UsdUfe at the moment. See the UsdUndoDuplicateCommand,
    //       which is still in MayaUsd for an example of how it is done.

    // Note: the USD duplication code below is similar to the one in
    //       UsdUndoDuplicateCommand, except it targets an arbitrary
    //       destination location. This is why the duplicate command
    //       could not be used directly as a sub-command.

    // Make sure all necessary parents exist in the target layer, at least as over,
    // otherwise SdfCopySepc will fail.
    SdfJustCreatePrimInLayer(dstLayer, dstUsdPath.GetParentPath());

    SdfPrimSpecHandleVector primSpecs;
    if (isComponentStage(srcUfePath)) {
        primSpecs = srcPrim.GetPrimStack();
    } else {
        // Retrieve the local layers around where the prim is defined and order them
        // from weak to strong. That weak-to-strong order allows us to copy the weakest
        // opinions first, so that they will get over-written by the stronger opinions.
        primSpecs = UsdUfe::getDefiningPrimStack(srcPrim);
        std::reverse(primSpecs.begin(), primSpecs.end());
    }

    // If no local layers were affected, then it means the prim is not local.
    // It probably is inside a reference and we do not support reparent from within
    // reference at this point. Report the error and abort the command.
    if (primSpecs.empty()) {
        const std::string error = TfStringPrintf(
            isComponent ? "Cannot reparent component prim \"%s\", prim's prim stack was empty."
                        : "Cannot reparent \"%s\" because we found no local layer containing it.",
            srcPrim.GetPath().GetText());
        TF_WARN("%s", error.c_str());
        throw std::runtime_error(error);
    }

#ifdef USD_HAS_NAMESPACE_EDIT
    // Try to use a single-layer renaming namespace edit (non-component optimization)
    // This only works correctly if there is a single layer and the destination layer
    // is the same as the source layer. If it fails we will fall through to the other
    // algorithm below.
    if (!isComponent && primSpecs.size() == 1) {
        SdfBatchNamespaceEdit edits;
        const auto            parentPath = dstUsdPath.GetParentPath();
        edits.Add(SdfNamespaceEdit::Reparent(srcUsdPath, parentPath, SdfNamespaceEdit::Same));

        if (dstLayer->Apply(edits)) {
            return;
        }
    }
#endif

    // Copy phase: Copy all prim specs to destination
    UsdUfe::MergePrimsOptions options;
    options.verbosity = UsdUfe::MergeVerbosity::None;
    options.mergeChildren = true;

    const bool includeTopLayer = true;
    const auto sessionLayers
        = UsdUfe::getAllSublayerRefs(stage->GetSessionLayer(), includeTopLayer);

    bool isFirst = true;

    for (const SdfPrimSpecHandle& primSpec : primSpecs) {
        const auto layer = primSpec->GetLayer();
        const auto path = primSpec->GetPath();

        // We want to leave session data in the session layers.
        // If a layer is a session layer then we set the target to be that same layer.
        const bool isInSession = UsdUfe::isSessionLayer(layer, sessionLayers);
        const auto targetLayer = isInSession ? layer : dstLayer;

        if (isInSession)
            SdfJustCreatePrimInLayer(targetLayer, dstUsdPath);

        const bool result = (isFirst || isInSession)
            ? SdfCopySpec(layer, path, targetLayer, dstUsdPath)
            : UsdUfe::mergePrims(stage, layer, path, stage, targetLayer, dstUsdPath, options);

        if (!result) {
            const std::string error = TfStringPrintf(
                "Insert child command: moving prim \"%s\" to \"%s\" failed in layer \"%s\".",
                srcUsdPath.GetString().c_str(),
                dstUsdPath.GetString().c_str(),
                layer->GetDisplayName().c_str());
            TF_WARN("%s", error.c_str());
            throw std::runtime_error(error);
        }
        // We only set the first-layer flag to false once we have processed a non-session layer.
        if (!isInSession)
            isFirst = false;
    }

    // Removal phase: Use provided removal function
    for (const SdfPrimSpecHandle& primSpec : primSpecs) {
        removalFn(primSpec, stage);
    }
}

static void
preserveLoadRules(const Ufe::Path& srcUfePath, const SdfPath& srcUsdPath, const SdfPath& dstUsdPath)
{
    UsdPrim           srcPrim = ufePathToPrim(srcUfePath);
    const UsdStagePtr stage = srcPrim.GetStage();

    // Make sure the load state of the reparented prim will be preserved.
    moveLoadRules(*stage, srcUsdPath, dstUsdPath);
}

static const UsdSceneItem::Ptr
sendReparentNotification(const Ufe::Path& srcUfePath, const Ufe::Path& dstUfePath)
{
    UsdPrim                 dstPrim = ufePathToPrim(dstUfePath);
    const UsdSceneItem::Ptr ufeDstItem = UsdSceneItem::create(dstUfePath, dstPrim);

    sendNotification<Ufe::ObjectReparent>(ufeDstItem, srcUfePath);

    return ufeDstItem;
}

void UsdUndoInsertChildCommand::execute()
{
    InPathChange pc;

    if (_usdDstPath.IsEmpty()) {
        const auto& parentPrim = ufePathToPrim(_ufeParentPath);

        // First, check if we need to rename the child.
        const std::string srcName = _ufeSrcPath.back().string();
        const auto        childName = uniqueChildNameDefault(parentPrim, srcName);

        // Create a new segment if parent and child are in different run-times.
        // parenting a USD node to the proxy shape node implies two different run-times
        // Contrary to MayaUsd, MaxUsd uses 2 segments using the USD RtId. The first
        // segment maps to the pseudo-root prim. In Maya or Max - the segment containing
        // the actual prim path is never the first.
        auto cRtId = _ufeSrcPath.runTimeId();
        if ((_ufeParentPath.runTimeId() == cRtId) && (_ufeParentPath.nbSegments() > 1)) {
            _ufeDstPath = _ufeParentPath + childName;
        } else {
            auto cSep = _ufeSrcPath.getSegments().back().separator();
            _ufeDstPath
                = _ufeParentPath + Ufe::PathSegment(Ufe::PathComponent(childName), cRtId, cSep);
        }
        _usdDstPath = parentPrim.GetPath().AppendChild(TfToken(childName));
    }

    // Load rules must be duplicated before the prim is moved to be able
    // to access the existing rules.
    preserveLoadRules(_ufeSrcPath, _usdSrcPath, _usdDstPath);

    // Pause edit forwarding during the undo operation with RAII
    const EditForwardingGuard efPauser {};

    // We need to keep the generated item to be able to return it to the caller
    // via the insertedChild() member function.
    {
        UsdUndoBlock undoBlock(&_undoableItem);

        // Check if we're reparenting a component and use appropriate removal function
        const bool isComponent = isComponentStage(_ufeSrcPath);

        // Define the removal function based on whether it's a component
        PrimSpecRemovalFn removalFn = isComponent
            ? [](const SdfPrimSpecHandle& primSpec, const UsdStageRefPtr& stage) {
                  // Component: Use direct SDF removal
                  const auto layer = primSpec->GetLayer();
                  const auto path = primSpec->GetPath();

                  UsdUfe::UsdUndoManager::instance().trackLayerStates(layer);
                  SdfPrimSpecHandle parent = primSpec->GetRealNameParent();
                  if (!parent) {
                      const std::string error = TfStringPrintf(
                          "Failed to get parent for component prim \"%s\" in layer \"%s\".",
                          path.GetText(),
                          layer->GetDisplayName().c_str());
                      TF_WARN("%s", error.c_str());
                      throw std::runtime_error(error);
                  }

                  if (!parent->RemoveNameChild(primSpec)) {
                      const std::string error = TfStringPrintf(
                          "Failed to remove component prim \"%s\" from layer \"%s\".",
                          path.GetText(),
                          layer->GetDisplayName().c_str());
                      TF_WARN("%s", error.c_str());
                      throw std::runtime_error(error);
                  }

                  layer->RemovePrimIfInert(parent);
              }
            : [](const SdfPrimSpecHandle& primSpec, const UsdStageRefPtr& stage) {
                  // Non-component: Use stage-level removal
                  const auto layer = primSpec->GetLayer();
                  const auto path = primSpec->GetPath();

                  UsdEditContext ctx(stage, layer);
                  if (!stage->RemovePrim(path)) {
                      const std::string error = TfStringPrintf(
                          "Insert child command: removing prim \"%s\" in layer \"%s\" failed.",
                          path.GetString().c_str(),
                          layer->GetDisplayName().c_str());
                      TF_WARN("%s", error.c_str());
                      throw std::runtime_error(error);
                  }
              };

        doInsertion(_usdSrcPath, _ufeSrcPath, _usdDstPath, _ufeDstPath, isComponent, removalFn);
    }

    _ufeDstItem = sendReparentNotification(_ufeSrcPath, _ufeDstPath);
}

void UsdUndoInsertChildCommand::undo()
{
    InPathChange pc;

    // Load rules must be duplicated before the prim is moved to be able
    // to access the existing rules.
    // Note: the arguments passed are the opposite of those in execute and redo().
    preserveLoadRules(_ufeDstPath, _usdDstPath, _usdSrcPath);

    // Pause edit forwarding during the undo operation with RAII
    const EditForwardingGuard efPauser {};

    _undoableItem.undo();

    // Note: the arguments passed are the opposite of those in execute and redo().
    sendReparentNotification(_ufeDstPath, _ufeSrcPath);
}

void UsdUndoInsertChildCommand::redo()
{
    InPathChange pc;

    // Load rules must be duplicated before the prim is moved to be able
    // to access the existing rules.
    preserveLoadRules(_ufeSrcPath, _usdSrcPath, _usdDstPath);

    // Pause edit forwarding during the undo operation with RAII
    const EditForwardingGuard efPauser {};

    _undoableItem.redo();

    _ufeDstItem = sendReparentNotification(_ufeSrcPath, _ufeDstPath);
}

} // namespace USDUFE_NS_DEF
