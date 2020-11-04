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
#ifndef PXRUSDTRANSLATORS_JOINT_WRITER_H
#define PXRUSDTRANSLATORS_JOINT_WRITER_H

/// \file

#include <mayaUsd/fileio/primWriter.h>
#include <mayaUsd/fileio/writeJobContext.h>

#include <pxr/pxr.h>
#include <pxr/usd/sdf/path.h>
#include <pxr/usd/usd/timeCode.h>
#include <pxr/usd/usdGeom/xform.h>
#include <pxr/usd/usdSkel/animMapper.h>
#include <pxr/usd/usdSkel/animation.h>
#include <pxr/usd/usdSkel/skeleton.h>
#include <pxr/usd/usdSkel/topology.h>

#include <maya/MFnDependencyNode.h>

PXR_NAMESPACE_OPEN_SCOPE

/// Exports joint hierarchies (the hierarchies of DAG nodes rooted at a joint)
/// as a UsdSkelSkeleton, along with a UsdSkelAnimation if the joints are
/// animated or posed differently from their rest pose. Currently, each joint
/// hierarchy is treated as a separate skeleton, meaning that this prim writer
/// will never produce skeletons with multiple root joints.
///
/// If the joints are posed differently from the rest pose on the export frame
/// (the current frame when the export command is run), a UsdSkelAnimation is
/// created to encode the pose.
/// If the exportAnimation flag is enabled for the write job and the joints do
/// contain animation, then a UsdSkelAnimation is created to encode the joint
/// animations.
class PxrUsdTranslators_JointWriter : public UsdMayaPrimWriter
{
public:
    PxrUsdTranslators_JointWriter(
        const MFnDependencyNode& depNodeFn,
        const SdfPath&           usdPath,
        UsdMayaWriteJobContext&  jobCtx);

    void Write(const UsdTimeCode& usdTime) override;
    bool ExportsGprims() const override;
    bool ShouldPruneChildren() const override;

private:
    bool _WriteRestState();

    bool             _valid;
    UsdSkelSkeleton  _skel;
    UsdSkelAnimation _skelAnim;

    /// The dag path defining the root transform of the Skeleton.
    MDagPath _skelXformPath;

    /// The common parent path of all proper joints.
    MDagPath _jointHierarchyRootPath;

    UsdSkelTopology       _topology;
    UsdSkelAnimMapper     _skelToAnimMapper;
    std::vector<MDagPath> _joints, _animatedJoints;
    UsdAttribute          _skelXformAttr;
    bool                  _skelXformIsAnimated;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
