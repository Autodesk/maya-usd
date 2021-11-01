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
#include <mayaUsd/fileio/utils/writeUtil.h>
#include <mayaUsd/nodes/proxyShapeBase.h>
#include <mayaUsd/ufe/Utils.h>
#include <mayaUsd/utils/traverseLayer.h>

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

bool UsdMayaPrimUpdater::pull(const UsdMayaPrimUpdaterContext& context) { return true; }

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
    SdfLayerRefPtr srcLayer,
    const SdfPath& topSrcPath,
    SdfLayerRefPtr dstLayer,
    const SdfPath& topDstPath)
{
    // Copy to the destination layer one primSpec at a time.  The default
    // SdfCopySpec(srcLayer, topSrcPath, dstLayer, topDstPath) overload
    // recurses down to children, which doesn't allow for per-primSpec control.
    // Call the SdfCopySpec overload with a functor that avoids recursing down
    // to children, so that only the single top-level prim at that point in the
    // traversal is copied.

    // Capture topSrcPath and topDstPath to pass them into SdfShouldCopyValue().
    // Since we're copying a single prim and not recursing to children,
    // topSrcPath and srcPath will be equal, and so will topDstPath and dstPath.
    auto shouldCopyValueFn = [topSrcPath, topDstPath](
                                 SdfSpecType               specType,
                                 const TfToken&            field,
                                 const SdfLayerHandle&     srcLayer,
                                 const SdfPath&            srcPath,
                                 bool                      fieldInSrc,
                                 const SdfLayerHandle&     dstLayer,
                                 const SdfPath&            dstPath,
                                 bool                      fieldInDst,
                                 boost::optional<VtValue>* valueToCopy) {
        return SdfShouldCopyValue(
            topSrcPath,
            topDstPath,
            specType,
            field,
            srcLayer,
            srcPath,
            fieldInSrc,
            dstLayer,
            dstPath,
            fieldInDst,
            valueToCopy);
    };
    auto dontCopyChildrenFn = [](const TfToken& childrenField,
                                 const SdfLayerHandle&,
                                 const SdfPath&,
                                 bool,
                                 const SdfLayerHandle&,
                                 const SdfPath&,
                                 bool,
                                 boost::optional<VtValue>*,
                                 boost::optional<VtValue>*) {
        // There must be an existing list of static children fields.
        // PPT, 18-Oct-21.
        static TfToken properties("properties");
        static TfToken primChildren("primChildren");

        // See traverseLayer() implementation for full list of children field.
        // Property children must be copied, prim children must not.  What
        // about other children field?  If we understand the complete list, we
        // can encode it in a map, rather than a chain of conditionals.  PPT,
        // 18-Oct-21.
        if (childrenField == properties) {
            return true;
        } else if (childrenField == primChildren) {
            return false;
        }

        // Default is copy.
        return true;
    };

    return SdfCopySpec(
        srcLayer, topSrcPath, dstLayer, topDstPath, shouldCopyValueFn, dontCopyChildrenFn);
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
