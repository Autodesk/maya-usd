//
// Copyright 2023 Autodesk
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
#include "UsdUndoCreateStageWithNewLayerCommand.h"

#include <mayaUsd/ufe/UsdUndoRenameCommand.h>
#include <mayaUsd/ufe/Utils.h>
#include <mayaUsd/undo/OpUndoItemRecorder.h>
#include <mayaUsd/undo/OpUndoItems.h>

#include <maya/MDagModifier.h>
#include <ufe/hierarchy.h>

PXR_NAMESPACE_USING_DIRECTIVE

namespace MAYAUSD_NS_DEF {
namespace ufe {

UsdUndoCreateStageWithNewLayerCommand::UsdUndoCreateStageWithNewLayerCommand(
    const Ufe::SceneItem::Ptr& parentItem)
    : _parentItem(nullptr)
    , _insertedChild(nullptr)
{
    if (!TF_VERIFY(parentItem))
        return;

    _parentItem = parentItem;
}

UsdUndoCreateStageWithNewLayerCommand::~UsdUndoCreateStageWithNewLayerCommand() { }

UsdUndoCreateStageWithNewLayerCommand::Ptr
UsdUndoCreateStageWithNewLayerCommand::create(const Ufe::SceneItem::Ptr& parentItem)
{
    if (!parentItem) {
        return nullptr;
    }

    return std::make_shared<UsdUndoCreateStageWithNewLayerCommand>(parentItem);
}

Ufe::SceneItem::Ptr UsdUndoCreateStageWithNewLayerCommand::sceneItem() const
{
    return _insertedChild;
}

void UsdUndoCreateStageWithNewLayerCommand::execute()
{
    if (!_parentItem) {
        return;
    }

    bool success = false;
    try {
        OpUndoItemRecorder undoRecorder(_undoItemList);
        success = executeWithinUndoRecorder();
    } catch (const std::exception&) {
        _undoItemList.undo();
        throw;
    }

    if (!success) {
        _undoItemList.undo();
    }
}

bool UsdUndoCreateStageWithNewLayerCommand::executeWithinUndoRecorder()
{
    // Get a MObject from the parent scene item.
    // Note: If and only if the parent is the world node, MDagPath::transform() will set status
    // to kInvalidParameter. In this case MObject::kNullObj is returned, which is a valid parent
    // object. Thus, kInvalidParameter will not be treated as a failure.
    MStatus  status;
    MDagPath parentDagPath = MayaUsd::ufe::ufeToDagPath(_parentItem->path());
    MObject  parentObject = parentDagPath.transform(&status);
    if (status != MStatus::kInvalidParameter && MFAIL(status))
        return false;

    MDagModifier& dagMod = MDagModifierUndoItem::create("Create stage with new Layer");

    // Create a transform node.
    // Note: It would be possible to create the transform and the proxy shape in one doIt() call
    // of a single MDagModifier. However, doing so causes notifications to be sent in a
    // different order, which triggers a `TF_VERIFY(g_StageMap.isDirty())` in
    // StagesSubject::onStageSet(). Using a separate MDagModifier to create the transform seems
    // more robust and avoids triggering the TF_VERIFY.
    MObject transformObj;
    transformObj = dagMod.createNode("transform", parentObject);
    if (transformObj.isNull())
        return false;

    if (!dagMod.doIt())
        return false;

    // Create a proxy shape.
    MObject proxyShape;
    proxyShape = dagMod.createNode("mayaUsdProxyShape", transformObj);
    if (proxyShape.isNull())
        return false;

    // Rename the transform and the proxy shape.
    // Note: The transform is renamed twice. The first rename operation renames it from its
    // default name "transform1" to "stage1". The number-suffix will be automatically
    // incremented if necessary. The second rename operation renames it from "stageX" to
    // "stage1". This doesn't do anything for the transform itself but it will adjust the
    // number-suffix of the proxy shape according to the suffix of the transform, because they
    // now share the common prefix "stage".
    if (!dagMod.renameNode(proxyShape, "stageShape1"))
        return false;
    if (!dagMod.renameNode(transformObj, "stage1"))
        return false;
    if (!dagMod.renameNode(transformObj, "stage1"))
        return false;

    // Get the global `time1` object and its `outTime` attribute.
    MSelectionList selection;
    selection.add("time1");
    MObject time1;
    if (!selection.getDependNode(0, time1))
        return false;
    MFnDependencyNode time1DepNodeFn(time1, &status);
    if (MFAIL(status))
        return false;
    MObject time1OutTimeAttr = time1DepNodeFn.attribute("outTime");
    if (time1OutTimeAttr.isNull())
        return false;

    // Get the `time` attribute of the newly created mayaUsdProxyShape.
    MDagPath proxyShapeDagPath;
    if (!MDagPath::getAPathTo(proxyShape, proxyShapeDagPath))
        return false;
    MFnDependencyNode proxyShapeDepNodeFn(proxyShapeDagPath.node(), &status);
    if (MFAIL(status))
        return false;
    MObject proxyShapeTimeAttr = proxyShapeDepNodeFn.attribute("time");
    if (proxyShapeTimeAttr.isNull())
        return false;

    // Connect `time1.outTime` to `proxyShapde.time`.
    if (!dagMod.connect(time1, time1OutTimeAttr, proxyShape, proxyShapeTimeAttr))
        return false;

    // Execute the operations.
    if (!dagMod.doIt())
        return false;

    // Create a UFE scene item for the newly created mayaUsdProxyShape.
    Ufe::Path proxyShapeUfePath = MayaUsd::ufe::dagPathToUfe(proxyShapeDagPath);
    _insertedChild = Ufe::Hierarchy::createItem(proxyShapeUfePath);

    // Refresh the cache of the stage map.
    // When creating the proxy shape, the stage map gets dirtied and cleaned. Afterwards, the
    // proxy shape is renamed. The stage map does not observe the Maya data model, so renaming
    // does not dirty the stage map again. Thus, the cache is in an invalid state, where it
    // contains the path of the proxy shape before it was renamed. Calling getProxyShape()
    // refreshes the cache. See comments within UsdStageMap::proxyShape() for more details.
    getProxyShape(proxyShapeUfePath);

    return true;
}

void UsdUndoCreateStageWithNewLayerCommand::undo() { _undoItemList.undo(); }

void UsdUndoCreateStageWithNewLayerCommand::redo()
{
    _undoItemList.redo();

    // Refresh the cache of the stage map.
    if (_insertedChild)
        getProxyShape(_insertedChild->path());
}

} // namespace ufe
} // namespace MAYAUSD_NS_DEF
