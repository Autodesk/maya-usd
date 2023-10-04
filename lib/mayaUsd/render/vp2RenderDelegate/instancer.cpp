//
// Copyright 2016 Pixar
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
// Modifications copyright (C) 2019 Autodesk
//
#include "instancer.h"

#include "sampler.h"

#include <pxr/base/gf/matrix4d.h>
#include <pxr/base/gf/quaternion.h>
#include <pxr/base/gf/rotation.h>
#include <pxr/base/gf/vec3f.h>
#include <pxr/base/gf/vec4f.h>
#include <pxr/base/tf/staticTokens.h>
#include <pxr/imaging/hd/sceneDelegate.h>

PXR_NAMESPACE_OPEN_SCOPE

/*! \brief  Constructor.

    \param delegate     The scene delegate backing this instancer's data.
    \param id           The unique id of this instancer.
    \param parentId     The unique id of the parent instancer,
                        or an empty id if not applicable.
*/
HdVP2Instancer::HdVP2Instancer(
    HdSceneDelegate* delegate,
#if defined(HD_API_VERSION) && HD_API_VERSION >= 36
    SdfPath const& id)
    : HdInstancer(delegate, id)
#else
    SdfPath const& id,
    SdfPath const& parentId)
    : HdInstancer(delegate, id, parentId)
#endif
{
}

/*! \brief  Destructor.
 */
HdVP2Instancer::~HdVP2Instancer()
{
    TF_FOR_ALL(it, _primvarMap) { delete it->second; }
    _primvarMap.clear();
}

/*! \brief  Checks the change tracker to determine whether instance primvars are
            dirty, and if so pulls them.

    Since primvars can only be pulled once, and are cached, this function is not
    re-entrant. However, this function is called by ComputeInstanceTransforms,
    which is called by HdVP2Mesh::Sync(), which is dispatched in parallel, so it needs
    to be guarded by _instanceLock.
*/
void HdVP2Instancer::_SyncPrimvars()
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    HdChangeTracker& changeTracker = GetDelegate()->GetRenderIndex().GetChangeTracker();
    SdfPath const&   id = GetId();

    // Use the double-checked locking pattern to check if this instancer's
    // primvars are dirty.
    HdDirtyBits dirtyBits = changeTracker.GetInstancerDirtyBits(id);
    if (HdChangeTracker::IsAnyPrimvarDirty(dirtyBits, id)
        || HdChangeTracker::IsInstancerDirty(dirtyBits, id)
        || HdChangeTracker::IsInstanceIndexDirty(dirtyBits, id)) {
        std::lock_guard<std::mutex> lock(_instanceLock);

        // If not dirty, then another thread did the job
        dirtyBits = changeTracker.GetInstancerDirtyBits(id);

#if defined(HD_API_VERSION) && HD_API_VERSION >= 36
        _UpdateInstancer(GetDelegate(), &dirtyBits);
#endif

        if (HdChangeTracker::IsAnyPrimvarDirty(dirtyBits, id)) {

            // If this instancer has dirty primvars, get the list of
            // primvar names and then cache each one.

            TfTokenVector             primvarNames;
            HdPrimvarDescriptorVector primvars
                = GetDelegate()->GetPrimvarDescriptors(id, HdInterpolationInstance);

            for (HdPrimvarDescriptor const& pv : primvars) {
                if (HdChangeTracker::IsPrimvarDirty(dirtyBits, id, pv.name)) {
                    VtValue value = GetDelegate()->Get(id, pv.name);
                    if (!value.IsEmpty()) {
                        if (_primvarMap.count(pv.name) > 0) {
                            delete _primvarMap[pv.name];
                        }
                        _primvarMap[pv.name] = new HdVtBufferSource(pv.name, value);
                    }
                }
            }
        }

        // Mark the instancer as clean
        changeTracker.MarkInstancerClean(id);
    }
}

/*! \brief  Computes all instance transforms for the provided prototype id.

    Taking into account the scene delegate's instancerTransform and the
    instance primvars "instanceTransform", "translate", "rotate", "scale".
    Computes and flattens nested transforms, if necessary.

    \param prototypeId The prototype to compute transforms for.

    \return One transform per instance, to apply when drawing.
*/
VtMatrix4dArray HdVP2Instancer::ComputeInstanceTransforms(SdfPath const& prototypeId)
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    _SyncPrimvars();

    // The transforms for this level of instancer are computed by:
    // foreach(index : indices) {
    //     instancerTransform
    //     * hydra:translate(index)
    //     * hydra:rotate(index)
    //     * hydra:scale(index)
    //     * hydra:instanceTransform(index)
    // }
    // If any transform isn't provided, it's assumed to be the identity.

    GfMatrix4d instancerTransform = GetDelegate()->GetInstancerTransform(GetId());
    VtIntArray instanceIndices = GetDelegate()->GetInstanceIndices(GetId(), prototypeId);

    VtMatrix4dArray transforms(instanceIndices.size());
    for (size_t i = 0; i < instanceIndices.size(); ++i) {
        transforms[i] = instancerTransform;
    }

    // "hydra:instanceTranslations" holds a translation vector for each index.
#if HD_API_VERSION < 56
    if (_primvarMap.count(HdInstancerTokens->translate) > 0) {
        HdVP2BufferSampler sampler(*_primvarMap[HdInstancerTokens->translate]);
#else
    if (_primvarMap.count(HdInstancerTokens->instanceTranslations) > 0) {
        HdVP2BufferSampler sampler(*_primvarMap[HdInstancerTokens->instanceTranslations]);
#endif
        for (size_t i = 0; i < instanceIndices.size(); ++i) {
            GfVec3f translate;
            if (sampler.Sample(instanceIndices[i], &translate)) {
                GfMatrix4d translateMat(1);
                translateMat.SetTranslate(GfVec3d(translate));
                transforms[i] = translateMat * transforms[i];
            }
        }
    }

    // "hydra:instanceRotations" holds a quaternion in <real, i, j, k> format
    // for each index.
#if HD_API_VERSION < 56
    if (_primvarMap.count(HdInstancerTokens->rotate) > 0) {
        HdVP2BufferSampler sampler(*_primvarMap[HdInstancerTokens->rotate]);
#else
    if (_primvarMap.count(HdInstancerTokens->instanceRotations) > 0) {
        HdVP2BufferSampler sampler(*_primvarMap[HdInstancerTokens->instanceRotations]);
#endif
        for (size_t i = 0; i < instanceIndices.size(); ++i) {
            GfQuath quath;
            if (sampler.Sample(instanceIndices[i], &quath)) {
                GfMatrix4d rotateMat(1);
                rotateMat.SetRotate(quath);
                transforms[i] = rotateMat * transforms[i];
            } else {
                GfVec4f quat;
                if (sampler.Sample(instanceIndices[i], &quat)) {
                    GfMatrix4d rotateMat(1);
                    rotateMat.SetRotate(GfQuatd(quat[0], quat[1], quat[2], quat[3]));
                    transforms[i] = rotateMat * transforms[i];
                }
            }
        }
    }

    // "hydra:instanceScales" holds an axis-aligned scale vector for each index.
#if HD_API_VERSION < 56
    if (_primvarMap.count(HdInstancerTokens->scale) > 0) {
        HdVP2BufferSampler sampler(*_primvarMap[HdInstancerTokens->scale]);
#else
    if (_primvarMap.count(HdInstancerTokens->instanceScales) > 0) {
        HdVP2BufferSampler sampler(*_primvarMap[HdInstancerTokens->instanceScales]);
#endif
        for (size_t i = 0; i < instanceIndices.size(); ++i) {
            GfVec3f scale;
            if (sampler.Sample(instanceIndices[i], &scale)) {
                GfMatrix4d scaleMat(1);
                scaleMat.SetScale(GfVec3d(scale));
                transforms[i] = scaleMat * transforms[i];
            }
        }
    }

    // "hydra:instanceTransforms" holds a 4x4 transform matrix for each index.
#if HD_API_VERSION < 56
    if (_primvarMap.count(HdInstancerTokens->instanceTransform) > 0) {
        HdVP2BufferSampler sampler(*_primvarMap[HdInstancerTokens->instanceTransform]);
#else
    if (_primvarMap.count(HdInstancerTokens->instanceTransforms) > 0) {
        HdVP2BufferSampler sampler(*_primvarMap[HdInstancerTokens->instanceTransforms]);
#endif
        for (size_t i = 0; i < instanceIndices.size(); ++i) {
            GfMatrix4d instanceTransform;
            if (sampler.Sample(instanceIndices[i], &instanceTransform)) {
                transforms[i] = instanceTransform * transforms[i];
            }
        }
    }

    if (GetParentId().IsEmpty()) {
        return transforms;
    }

    HdInstancer* parentInstancer = GetDelegate()->GetRenderIndex().GetInstancer(GetParentId());
    if (!TF_VERIFY(parentInstancer)) {
        return transforms;
    }

    // The transforms taking nesting into account are computed by:
    // parentTransforms = parentInstancer->ComputeInstanceTransforms(GetId())
    // foreach (parentXf : parentTransforms, xf : transforms) {
    //     parentXf * xf
    // }
    VtMatrix4dArray parentTransforms
        = static_cast<HdVP2Instancer*>(parentInstancer)->ComputeInstanceTransforms(GetId());

    VtMatrix4dArray final(parentTransforms.size() * transforms.size());
    for (size_t i = 0; i < parentTransforms.size(); ++i) {
        for (size_t j = 0; j < transforms.size(); ++j) {
            final[i * transforms.size() + j] = transforms[j] * parentTransforms[i];
        }
    }
    return final;
}

PXR_NAMESPACE_CLOSE_SCOPE
