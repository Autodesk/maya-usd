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
#ifndef HD_VP2_INSTANCER
#define HD_VP2_INSTANCER

#include <pxr/base/gf/vec4f.h>
#include <pxr/base/tf/hashmap.h>
#include <pxr/base/tf/token.h>
#include <pxr/base/vt/array.h>
#include <pxr/base/vt/types.h>
#include <pxr/imaging/hd/instancer.h>
#include <pxr/imaging/hd/vtBufferSource.h>
#include <pxr/pxr.h>

#include <mutex>
#include <unordered_map>

PXR_NAMESPACE_OPEN_SCOPE

/*! \brief  VP2 instancing of prototype geometry with varying transforms
    \class  HdVP2Instancer

    Nested instancing can be handled by recursion, and by taking the
    cartesian product of the transform arrays at each nesting level, to
    create a flattened transform array.
*/
class HdVP2Instancer final : public HdInstancer
{
public:
#if defined(HD_API_VERSION) && HD_API_VERSION >= 36
    HdVP2Instancer(HdSceneDelegate* delegate, SdfPath const& id);
#else
    HdVP2Instancer(HdSceneDelegate* delegate, SdfPath const& id, SdfPath const& parentInstancerId);
#endif

    ~HdVP2Instancer();

    void Sync(HdSceneDelegate* sceneDelegate, HdRenderParam* renderParam, HdDirtyBits* dirtyBits)
        override;

    VtMatrix4dArray GetInstanceTransforms(SdfPath const& prototypeId);

private:
    /*! \brief  Looks up the cached instance indices covering \p prototypeId.

        The cache is keyed by the prototype paths the instancer declares, which
        are not always the paths callers ask about. A Hydra 1.0 scene delegate
        lists every prototype rprim individually, so the rprim id an HdVP2Mesh
        passes in matches a key exactly. The Hydra 2.0 native instancing scene
        indices instead declare a single prototype *root* and expect consumers
        to ask about prims beneath it - see
        HdInstancerTopologySchema::ComputeInstanceIndicesForProto, which matches
        with HasPrefix for precisely that reason.

        So: exact match first, which is every Hydra 1.0 lookup, then the
        innermost enclosing prototype root. Read-only, because rprims sync in
        parallel and this runs on each of those threads.

        \return Null if no declared prototype covers \p prototypeId.
    */
    const VtIntArray* _FindInstanceIndices(SdfPath const& prototypeId) const;

    /*! Map of the latest primvar data for this instancer, keyed by
        primvar name. Primvar values are VtValue, an any-type; they are
        interpreted at consumption time (here, in ComputeInstanceTransforms).
    */
    std::unordered_map<TfToken, std::unique_ptr<HdVtBufferSource>, TfToken::HashFunctor>
        _primvarMap;

    // Cache the instance indices to avoid having the scene delegate recompute
    // them every time
    int                                           _maxInstanceIndex = -1;
    TfHashMap<SdfPath, VtIntArray, SdfPath::Hash> _instanceIndicesByPrototype;

    // Instance transforms cache
    VtMatrix4dArray _instanceTransforms;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
