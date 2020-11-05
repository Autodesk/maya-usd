//
// Copyright 2018 Pixar
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
#ifndef PXRUSDMAYA_TRANSLATOR_SKEL_H
#define PXRUSDMAYA_TRANSLATOR_SKEL_H

#include <mayaUsd/base/api.h>
#include <mayaUsd/fileio/primReaderArgs.h>
#include <mayaUsd/fileio/primReaderContext.h>

#include <pxr/base/vt/array.h>
#include <pxr/pxr.h>

PXR_NAMESPACE_OPEN_SCOPE

class UsdSkelSkeleton;
class UsdSkelSkeletonQuery;
class UsdSkelSkinningQuery;

struct UsdMayaTranslatorSkel
{
    /// Returns true if \p joint is being used to identify the root of
    /// a UsdSkelSkeleton.
    MAYAUSD_CORE_PUBLIC
    static bool IsUsdSkeleton(const MDagPath& joint);

    /// Returns true if \p Skeleton was originally generated from Maya.
    /// This is based on bool metadata Maya:generated, and is used to
    /// determine whether or not a joint should be created to represent a
    /// Skeleton when importing a Skeleton from USD that was originally
    /// created in Maya.
    MAYAUSD_CORE_PUBLIC
    static bool IsSkelMayaGenerated(const UsdSkelSkeleton& skel);

    /// Mark a Skeleton as being originally exported from Maya.
    MAYAUSD_CORE_PUBLIC
    static void MarkSkelAsMayaGenerated(const UsdSkelSkeleton& skel);

    /// Create joint nodes for each joint in \p skelQuery.
    /// Animation is applied to the joints if \p args enable it.
    MAYAUSD_CORE_PUBLIC
    static bool CreateJointHierarchy(
        const UsdSkelSkeletonQuery&  skelQuery,
        MObject&                     parentNode,
        const UsdMayaPrimReaderArgs& args,
        UsdMayaPrimReaderContext*    context,
        VtArray<MObject>*            joints);

    /// Find the set of MObjects joint objects for a skeleton.
    MAYAUSD_CORE_PUBLIC
    static bool GetJoints(
        const UsdSkelSkeletonQuery& skelQuery,
        UsdMayaPrimReaderContext*   context,
        VtArray<MObject>*           joints);

    /// Create a dagPose node holding a bind pose for skel \p skelQuery.
    MAYAUSD_CORE_PUBLIC
    static bool CreateBindPose(
        const UsdSkelSkeletonQuery& skelQuery,
        const VtArray<MObject>&     joints,
        UsdMayaPrimReaderContext*   context,
        MObject*                    bindPoseNode);

    /// Find the bind pose for a Skeleton.
    MAYAUSD_CORE_PUBLIC
    static MObject
    GetBindPose(const UsdSkelSkeletonQuery& skelQuery, UsdMayaPrimReaderContext* context);

    /// Create a skin cluster for skinning \p primToSkin.
    /// The skinning cluster is wired up to be driven by the joints
    /// created by CreateJoints().
    /// This currently only supports mesh objects.
    MAYAUSD_CORE_PUBLIC
    static bool CreateSkinCluster(
        const UsdSkelSkeletonQuery&  skelQuery,
        const UsdSkelSkinningQuery&  skinningQuery,
        const VtArray<MObject>&      joints,
        const UsdPrim&               primToSkin,
        const UsdMayaPrimReaderArgs& args,
        UsdMayaPrimReaderContext*    context,
        const MObject&               bindPose = MObject());
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
