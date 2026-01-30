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

#include <algorithm>

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

/*! \brief  Sync the instancer and update the cached transforms, if need be.

    This method is executed when we call HdInstancer::_SyncInstancerAndParents
    in GetInstanceTransforms. Even if multiple instances try to sync their
    instancer in parallel, this method will only be called once; 
    _SyncInstancerAndParents takes care of the multithreading aspect.
*/
void HdVP2Instancer::Sync(
    HdSceneDelegate *sceneDelegate, 
    HdRenderParam   *renderParam, 
    HdDirtyBits     *dirtyBits)
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    _UpdateInstancer(sceneDelegate, dirtyBits);

    SdfPath const& id = GetId();

    if (HdChangeTracker::IsAnyPrimvarDirty(*dirtyBits, id)) {
        // If this instancer has dirty primvars, get the list of
        // primvar names and then cache each one.

        HdPrimvarDescriptorVector primvars
            = GetDelegate()->GetPrimvarDescriptors(id, HdInterpolationInstance);

        for (HdPrimvarDescriptor const& pv : primvars) {
            if (HdChangeTracker::IsPrimvarDirty(*dirtyBits, id, pv.name)) {
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

    // Update our instance indices cache if required
    if (*dirtyBits & HdChangeTracker::DirtyInstanceIndex) {
        _maxInstanceIndex = -1;
        _instanceIndicesByPrototype.clear();
        auto prototypeIds = sceneDelegate->GetInstancerPrototypes(id);
        for (const auto& prototypeId : prototypeIds) {
            auto instanceIndices = sceneDelegate->GetInstanceIndices(id, prototypeId);
            if (!instanceIndices.empty()) {
                _maxInstanceIndex = std::max(_maxInstanceIndex, *std::max_element(instanceIndices.begin(), instanceIndices.end()));
                _instanceIndicesByPrototype[prototypeId] = instanceIndices;
            }
        }
    }

    bool updateInstanceTransforms = (*dirtyBits & HdChangeTracker::DirtyTransform)
                                    || HdChangeTracker::IsAnyPrimvarDirty(*dirtyBits, id)
                                    || (*dirtyBits & HdChangeTracker::DirtyInstanceIndex);
    if (updateInstanceTransforms) {
        // Initialize all transforms to the instancer's transform, on which we later
        // apply the individual instances' transformations.
        GfMatrix4d instancerTransform = GetDelegate()->GetInstancerTransform(id);
        _instanceTransforms = VtMatrix4dArray(_maxInstanceIndex + 1, instancerTransform);

#if HD_API_VERSION < 56
        TfToken translationsToken = HdInstancerTokens->translate;
        TfToken rotationsToken = HdInstancerTokens->rotate;
        TfToken scalesToken = HdInstancerTokens->scale;
        TfToken transformsToken = HdInstancerTokens->instanceTransform;
#else
        TfToken translationsToken = HdInstancerTokens->instanceTranslations;
        TfToken rotationsToken = HdInstancerTokens->instanceRotations;
        TfToken scalesToken = HdInstancerTokens->instanceScales;
        TfToken transformsToken = HdInstancerTokens->instanceTransforms;
#endif

        // Translations
        if (_primvarMap.count(translationsToken) > 0) {
            HdVP2BufferSampler sampler(*_primvarMap[translationsToken]);
            for (size_t instanceIndex = 0; instanceIndex <= _maxInstanceIndex; instanceIndex++) {
                GfVec3f translate;
                if (sampler.Sample(instanceIndex, &translate)) {
                    GfMatrix4d translateMat(1);
                    translateMat.SetTranslate(GfVec3d(translate));
                    _instanceTransforms[instanceIndex] = translateMat * _instanceTransforms[instanceIndex];
                }
            }
        }

        // Rotations
        if (_primvarMap.count(rotationsToken) > 0) {
            HdVP2BufferSampler sampler(*_primvarMap[rotationsToken]);
            for (size_t instanceIndex = 0; instanceIndex <= _maxInstanceIndex; instanceIndex++) {
                GfQuath quath;
                if (sampler.Sample(instanceIndex, &quath)) {
                    GfMatrix4d rotateMat(1);
                    rotateMat.SetRotate(quath);
                    _instanceTransforms[instanceIndex] = rotateMat * _instanceTransforms[instanceIndex];
                } else {
                    GfVec4f quat;
                    if (sampler.Sample(instanceIndex, &quat)) {
                        GfMatrix4d rotateMat(1);
                        rotateMat.SetRotate(GfQuatd(quat[0], quat[1], quat[2], quat[3]));
                        _instanceTransforms[instanceIndex] = rotateMat * _instanceTransforms[instanceIndex];
                    }
                }
            }
        }

        // Scales
        if (_primvarMap.count(scalesToken) > 0) {
            HdVP2BufferSampler sampler(*_primvarMap[scalesToken]);
            for (size_t instanceIndex = 0; instanceIndex <= _maxInstanceIndex; instanceIndex++) {
                GfVec3f scale;
                if (sampler.Sample(instanceIndex, &scale)) {
                    GfMatrix4d scaleMat(1);
                    scaleMat.SetScale(GfVec3d(scale));
                    _instanceTransforms[instanceIndex] = scaleMat * _instanceTransforms[instanceIndex];
                }
            }
        }

        // Transforms
        if (_primvarMap.count(transformsToken) > 0) {
            HdVP2BufferSampler sampler(*_primvarMap[transformsToken]);
            for (size_t instanceIndex = 0; instanceIndex <= _maxInstanceIndex; instanceIndex++) {
                GfMatrix4d transformMat;
                if (sampler.Sample(instanceIndex, &transformMat)) {
                    _instanceTransforms[instanceIndex] = transformMat * _instanceTransforms[instanceIndex];
                }
            }
        }
    }
}

/*! \brief  Retrieves or computes all instance transforms for the provided prototype id.

    Taking into account the scene delegate's instancerTransform and the
    instance primvars "instanceTransform", "translate", "rotate", "scale".
    Computes and flattens nested transforms, if necessary.

    \param prototypeId The prototype to get transforms for.

    \return One transform per instance, to apply when drawing.
*/
VtMatrix4dArray HdVP2Instancer::GetInstanceTransforms(SdfPath const& prototypeId)
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();
    
    HdInstancer::_SyncInstancerAndParents(GetDelegate()->GetRenderIndex(), GetId());

    // Get the instance indices from our cache instead of querying the scene delegate.
    auto itInstanceIndices = _instanceIndicesByPrototype.find(prototypeId);
    if (itInstanceIndices == _instanceIndicesByPrototype.end()) {
        return {};
    }
    
    // Retrieve only the instance transforms relevant to this prototype
    auto instanceIndices = itInstanceIndices->second;
    VtMatrix4dArray transforms(instanceIndices.size());
    for (size_t i = 0; i < instanceIndices.size(); i++) {
        transforms[i] = _instanceTransforms[instanceIndices[i]];
    }

    if (GetParentId().IsEmpty()) {
        return transforms;
    }

    HdInstancer* parentInstancer = GetDelegate()->GetRenderIndex().GetInstancer(GetParentId());
    if (!TF_VERIFY(parentInstancer)) {
        return transforms;
    }

    // The transforms taking nesting into account are computed by:
    // parentTransforms = parentInstancer->GetInstanceTransforms(GetId())
    // foreach (parentXf : parentTransforms, xf : transforms) {
    //     parentXf * xf
    // }
    VtMatrix4dArray parentTransforms
        = static_cast<HdVP2Instancer*>(parentInstancer)->GetInstanceTransforms(GetId());

    VtMatrix4dArray final(parentTransforms.size() * transforms.size());
    for (size_t i = 0; i < parentTransforms.size(); ++i) {
        for (size_t j = 0; j < transforms.size(); ++j) {
            final[i * transforms.size() + j] = transforms[j] * parentTransforms[i];
        }
    }
    return final;
}

PXR_NAMESPACE_CLOSE_SCOPE
