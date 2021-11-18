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
#include <mayaUsd/utils/traverseLayer.h>
#include <mayaUsdUtils/MergePrims.h>

#include <pxr/usd/sdf/copyUtils.h>
#include <pxr/usd/sdf/path.h>

#include <maya/MAnimControl.h>
#include <maya/MAnimUtil.h>
#include <maya/MFnDagNode.h>
#include <maya/MFnSet.h>
#include <maya/MFnStringData.h>
#include <maya/MFnTypedAttribute.h>
#include <maya/MGlobal.h>
#include <maya/MItDag.h>
#include <maya/MPlug.h>
#include <maya/MSelectionList.h>
#include <maya/MStatus.h>
#include <maya/MString.h>
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

PXR_NAMESPACE_OPEN_SCOPE

UsdMayaPrimUpdater::UsdMayaPrimUpdater(const MFnDependencyNode& depNodeFn, const Ufe::Path& path)
    : _mayaObject(depNodeFn.object())
    , _path(path)
{
}

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

bool UsdMayaPrimUpdater::editAsMaya(const UsdMayaPrimUpdaterContext& context) { return true; }

bool UsdMayaPrimUpdater::discardEdits(const UsdMayaPrimUpdaterContext& context)
{
    MObject objectToDelete = getMayaObject();
    if (!objectToDelete.isNull()) {
        MGlobal::deleteNode(objectToDelete);
    }
    return true;
}

bool UsdMayaPrimUpdater::pushEnd(const UsdMayaPrimUpdaterContext& context)
{
    return discardEdits(context);
}

bool UsdMayaPrimUpdater::pushCopySpecs(
    UsdStageRefPtr srcStage,
    SdfLayerRefPtr srcLayer,
    const SdfPath& srcSdfPath,
    UsdStageRefPtr dstStage,
    SdfLayerRefPtr dstLayer,
    const SdfPath& dstSdfPath)
{
    return MayaUsdUtils::mergePrims(srcStage, srcLayer, srcSdfPath, dstStage, dstLayer, dstSdfPath);
}

const MObject& UsdMayaPrimUpdater::getMayaObject() const { return _mayaObject; }

const Ufe::Path& UsdMayaPrimUpdater::getUfePath() const { return _path; }

UsdPrim UsdMayaPrimUpdater::getUsdPrim(const UsdMayaPrimUpdaterContext& context) const
{
    return MayaUsd::ufe::ufePathToPrim(_path);
}

/* static */
bool UsdMayaPrimUpdater::isAnimated(const MDagPath& path)
{
    auto isDagPathAnimated = [](const MDagPath& dagPath) {
        int     upstreamDependencies = -1;
        MString pyCommand;
        pyCommand.format(
            "import maya.cmds as cmds\n"
            "if cmds.evaluationManager( query=True, invalidate=True ):\n"
            "  upstream = cmds.evaluationManager(ust='^1s')\n"
            "  for node in upstream:\n"
            "    if cmds.nodeType(node) == 'mayaUsdProxyShape':\n"
            "      countNonProxyShape -= 1\n"
            "  return countNonProxyShape\n"
            "else:\n"
            "  return -1\n",
            dagPath.fullPathName().asChar());
        MGlobal::executePythonCommand(pyCommand, upstreamDependencies);

        if (upstreamDependencies >= 0)
            return (upstreamDependencies > 0);
        else
            return MAnimUtil::isAnimated(dagPath, true);
    };

    if (!isDagPathAnimated(path)) {
        MItDag dagIt(MItDag::kDepthFirst);
        dagIt.reset(path);
        for (; !dagIt.isDone(); dagIt.next()) {
            MDagPath dagPath;
            dagIt.getPath(dagPath);

            if (isDagPathAnimated(dagPath))
                return true;
        }
    } else {
        return true;
    }

    return false;
}

PXR_NAMESPACE_CLOSE_SCOPE
