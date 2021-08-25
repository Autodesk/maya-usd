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
#include <mayaUsd/fileio/primReaderRegistry.h>
#include <mayaUsd/fileio/translators/translatorSkel.h>
#include <mayaUsd/fileio/translators/translatorUtil.h>

#include <pxr/pxr.h>
#include <pxr/usd/usd/prim.h>
#include <pxr/usd/usd/primRange.h>
#include <pxr/usd/usdGeom/xform.h>
#include <pxr/usd/usdSkel/cache.h>
#include <pxr/usd/usdSkel/root.h>
#include <pxr/usd/usdSkel/skeletonQuery.h>
#include <pxr/usd/usdSkel/skinningQuery.h>

#include <maya/MObject.h>

PXR_NAMESPACE_OPEN_SCOPE

/// Prim reader for skeletons.
/// This produce a joint hierarchy, possibly animated, corresponding
/// to a UsdSkelSkeleton.
class UsdMayaPrimReaderSkeleton : public UsdMayaPrimReader
{
public:
    UsdMayaPrimReaderSkeleton(const UsdMayaPrimReaderArgs& args)
        : UsdMayaPrimReader(args)
    {
    }

    ~UsdMayaPrimReaderSkeleton() override { }

    bool Read(UsdMayaPrimReaderContext& context) override;

private:
    // TODO: Ideally, we'd share the cache across different models
    // if importing multiple skel roots.
    UsdSkelCache _cache;
};

TF_REGISTRY_FUNCTION_WITH_TAG(UsdMayaPrimReaderRegistry, UsdSkelSkeleton)
{
    UsdMayaPrimReaderRegistry::Register<UsdSkelSkeleton>([](const UsdMayaPrimReaderArgs& args) {
        return UsdMayaPrimReaderSharedPtr(new UsdMayaPrimReaderSkeleton(args));
    });
}

/// Prim reader for a UsdSkelRoot.
/// This post-processes the skinnable prims beneath a UsdSkelRoot
/// to define skin clusters, etc. for bound skeletons.
class UsdMayaPrimReaderSkelRoot : public UsdMayaPrimReader
{
public:
    UsdMayaPrimReaderSkelRoot(const UsdMayaPrimReaderArgs& args)
        : UsdMayaPrimReader(args)
    {
    }

    ~UsdMayaPrimReaderSkelRoot() override { }

    bool Read(UsdMayaPrimReaderContext& context) override;

    bool HasPostReadSubtree() const override { return true; }

    void PostReadSubtree(UsdMayaPrimReaderContext& context) override;

private:
    // TODO: Ideally, we'd share the cache across different models
    // if importing multiple skel roots.
    UsdSkelCache _cache;
};

TF_REGISTRY_FUNCTION_WITH_TAG(UsdMayaPrimReaderRegistry, UsdSkelRoot)
{
    UsdMayaPrimReaderRegistry::Register<UsdSkelRoot>([](const UsdMayaPrimReaderArgs& args) {
        return UsdMayaPrimReaderSharedPtr(new UsdMayaPrimReaderSkelRoot(args));
    });
}

bool UsdMayaPrimReaderSkeleton::Read(UsdMayaPrimReaderContext& context)
{
    UsdSkelSkeleton skel(_GetArgs().GetUsdPrim());
    if (!TF_VERIFY(skel))
        return false;

    if (UsdSkelSkeletonQuery skelQuery = _cache.GetSkelQuery(skel)) {

        MObject parentNode = context.GetMayaNode(skel.GetPrim().GetPath().GetParentPath(), true);

        // Build out a joint hierarchy.
        VtArray<MObject> joints;
        if (UsdMayaTranslatorSkel::CreateJointHierarchy(
                skelQuery, parentNode, _GetArgs(), &context, &joints)) {

            // Add a dagPose node to hold the rest pose.
            // This is not necessary for skinning to function in Maya, but is
            // necessary in order to properly round-trip the Skeleton's
            // restTransforms, and is a requirement of some exporters.
            // The dagPose command also will not work without this.
            MObject bindPose;
            if (UsdMayaTranslatorSkel::CreateBindPose(skelQuery, joints, &context, &bindPose)) {
                return true;
            }
        }
    }
    return false;
}

bool UsdMayaPrimReaderSkelRoot::Read(UsdMayaPrimReaderContext& context)
{
    UsdSkelRoot skelRoot(_GetArgs().GetUsdPrim());
    if (!TF_VERIFY(skelRoot))
        return false;

    // First pass through:
    // The skel root itself is a transform, so produce a transform.
    // Skeletal bindings will be handled as a post sub-tree process.
    MObject parentNode = context.GetMayaNode(
        skelRoot.GetPrim().GetPath().GetParentPath(),
        /*findAncestors*/ true);

    MStatus status;
    MObject obj;
    return UsdMayaTranslatorUtil::CreateTransformNode(
        skelRoot.GetPrim(), parentNode, _GetArgs(), &context, &status, &obj);
}

void UsdMayaPrimReaderSkelRoot::PostReadSubtree(UsdMayaPrimReaderContext& context)
{
    UsdSkelRoot skelRoot(_GetArgs().GetUsdPrim());
    if (!TF_VERIFY(skelRoot))
        return;

    // Compute skel bindings and create skin clusters for bound skels
    // We do this in a post-subtree stage to ensure that any skinnable
    // prims we produce skin clusters for have been processed first.

    std::vector<UsdSkelBinding> bindings;

#if PXR_VERSION > 2008
    _cache.Populate(skelRoot, UsdTraverseInstanceProxies());

    if (!_cache.ComputeSkelBindings(skelRoot, &bindings, UsdTraverseInstanceProxies())) {
        return;
    }
#else
    _cache.Populate(skelRoot);

    if (!_cache.ComputeSkelBindings(skelRoot, &bindings)) {
        return;
    }
#endif

    for (const UsdSkelBinding& binding : bindings) {
        if (binding.GetSkinningTargets().empty())
            continue;

        if (const UsdSkelSkeletonQuery& skelQuery = _cache.GetSkelQuery(binding.GetSkeleton())) {

            VtArray<MObject> joints;
            if (!UsdMayaTranslatorSkel::GetJoints(skelQuery, &context, &joints)) {
                continue;
            }

            for (const auto& skinningQuery : binding.GetSkinningTargets()) {

                const UsdPrim& skinnedPrim = skinningQuery.GetPrim();

                // Get an ordering of the joints that matches the ordering of
                // the binding.
                VtArray<MObject> skinningJoints;
                if (!skinningQuery.GetMapper()) {
                    skinningJoints = joints;
                } else {
                    // TODO:
                    // UsdSkelAnimMapper currently only supports remapping
                    // of Sdf value types, so we can't easily use it here.
                    // For now, we can delegate remapping behavior by
                    // remapping ordered joint indices.
                    VtIntArray indices(joints.size());
                    for (size_t i = 0; i < joints.size(); ++i)
                        indices[i] = i;

                    VtIntArray remappedIndices;
                    if (!skinningQuery.GetMapper()->Remap(indices, &remappedIndices)) {
                        continue;
                    }

                    skinningJoints.resize(remappedIndices.size());
                    for (size_t i = 0; i < remappedIndices.size(); ++i) {
                        int index = remappedIndices[i];
                        if (index >= 0 && static_cast<size_t>(index) < joints.size()) {
                            skinningJoints[i] = joints[index];
                        }
                    }
                }

                MObject bindPose = UsdMayaTranslatorSkel::GetBindPose(skelQuery, &context);

                // Add a skin cluster to skin this prim.
                UsdMayaTranslatorSkel::CreateSkinCluster(
                    skelQuery,
                    skinningQuery,
                    skinningJoints,
                    skinnedPrim,
                    _GetArgs(),
                    &context,
                    bindPose);
            }
        }
    }
}

PXR_NAMESPACE_CLOSE_SCOPE
