//
// Copyright 2018 Pixar
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
#include "primUpdater.h"

#include <mayaUsd/fileio/jobs/jobArgs.h>
#include <mayaUsd/fileio/jobs/readJob.h>
#include <mayaUsd/fileio/jobs/writeJob.h>
#include <mayaUsd/fileio/primReaderRegistry.h>
#include <mayaUsd/fileio/utils/writeUtil.h>
#include <mayaUsd/nodes/proxyShapeBase.h>
#include <mayaUsd/ufe/Utils.h>
#include <mayaUsd/undo/OpUndoItems.h>
#include <mayaUsd/utils/traverseLayer.h>
#include <mayaUsdUtils/MergePrims.h>

#include <pxr/usd/sdf/copyUtils.h>
#include <pxr/usd/sdf/path.h>

#include <maya/MAnimUtil.h>
#include <maya/MGlobal.h>
#include <maya/MItDag.h>
#include <ufe/hierarchy.h>
#include <ufe/path.h>
#include <ufe/pathString.h>

#include <string>

namespace MAYAUSD_NS_DEF {
namespace ufe {

//------------------------------------------------------------------------------
// Global variables
//------------------------------------------------------------------------------
extern Ufe::Rtid g_MayaRtid;

} // namespace ufe
} // namespace MAYAUSD_NS_DEF

using namespace MAYAUSD_NS_DEF;

PXR_NAMESPACE_OPEN_SCOPE

UsdMayaPrimUpdater::UsdMayaPrimUpdater(
    const UsdMayaPrimUpdaterContext& context,
    const MFnDependencyNode&         depNodeFn,
    const Ufe::Path&                 path)
    : _mayaObject(depNodeFn.object())
    , _path(path)
    , _context(&context)
{
}

bool UsdMayaPrimUpdater::shouldAutoEdit() const { return true; }

bool UsdMayaPrimUpdater::canEditAsMaya() const
{
    // To be editable as Maya data we must ensure that there is an importer (to
    // Maya).  As of 17-Nov-2021 it is not possible to determine how the
    // prim will round-trip back through export, so we do not check for
    // exporter (to USD) capability.
    auto prim = MayaUsd::ufe::ufePathToPrim(_path);
    TF_AXIOM(prim);
    return (UsdMayaPrimReaderRegistry::Find(prim.GetTypeName()) != nullptr);
}

bool UsdMayaPrimUpdater::editAsMaya() { return true; }

bool UsdMayaPrimUpdater::discardEdits()
{
    MObject objectToDelete = getMayaObject();
    if (objectToDelete.isNull())
        return true;

    // Nodes that are from a referenced file cannot be deleted
    // as they live in a different file. They will go away once
    // the reference is unloaded. Don't try to delete them here.
    MFnDependencyNode depNode(objectToDelete);
    if (!depNode.isFromReferencedFile()) {
        MStatus status = NodeDeletionUndoItem::deleteNode(
            "Discard edits delete individual pulled node", depNode.absoluteName(), objectToDelete);

        if (status != MS::kSuccess) {
            TF_WARN("Discard edits: cannot delete node.");
            return false;
        }
    }

    return true;
}

bool UsdMayaPrimUpdater::pushEnd()
{
    // Nothing. We rely on the PrimUpdaterManager to delete the nodes in the correct order.
    return true;
}

UsdMayaPrimUpdater::PushCopySpecs UsdMayaPrimUpdater::pushCopySpecs(
    UsdStageRefPtr srcStage,
    SdfLayerRefPtr srcLayer,
    const SdfPath& srcSdfPath,
    UsdStageRefPtr dstStage,
    SdfLayerRefPtr dstLayer,
    const SdfPath& dstSdfPath)
{
    MayaUsdUtils::MergePrimsOptions options;
    return MayaUsdUtils::mergePrims(
               srcStage, srcLayer, srcSdfPath, dstStage, dstLayer, dstSdfPath, options)
        ? PushCopySpecs::Continue
        : PushCopySpecs::Failed;
}

const MObject& UsdMayaPrimUpdater::getMayaObject() const { return _mayaObject; }

const Ufe::Path& UsdMayaPrimUpdater::getUfePath() const { return _path; }

UsdPrim UsdMayaPrimUpdater::getUsdPrim() const { return MayaUsd::ufe::ufePathToPrim(_path); }

/* static */
bool UsdMayaPrimUpdater::isAnimated(const MDagPath& path)
{
    if (!MAnimUtil::isAnimated(path, true)) {
        MItDag dagIt(MItDag::kDepthFirst);
        dagIt.reset(path);
        for (; !dagIt.isDone(); dagIt.next()) {
            MDagPath dagPath;
            dagIt.getPath(dagPath);

            if (MAnimUtil::isAnimated(dagPath, true))
                return true;
        }
    } else {
        return true;
    }

    return false;
}

PXR_NAMESPACE_CLOSE_SCOPE
