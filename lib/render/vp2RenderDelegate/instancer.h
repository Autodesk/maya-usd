//
// Copyright 2016 Pixar
//
// Licensed under the Apache License, Version 2.0 (the "Apache License")
// with the following modification; you may not use this file except in
// compliance with the Apache License and the following modification to it:
// Section 6. Trademarks. is deleted and replaced with:
//
// 6. Trademarks. This License does not grant permission to use the trade
//    names, trademarks, service marks, or product names of the Licensor
//    and its affiliates, except as required to comply with Section 4(c) of
//    the License and to reproduce the content of the NOTICE file.
//
// You may obtain a copy of the Apache License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the Apache License with the above modification is
// distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied. See the Apache License for the specific
// language governing permissions and limitations under the Apache License.
//
// Modifications copyright (C) 2019 Autodesk
//
#ifndef HD_VP2_INSTANCER
#define HD_VP2_INSTANCER

#include "pxr/pxr.h"

#include "pxr/imaging/hd/instancer.h"
#include "pxr/imaging/hd/vtBufferSource.h"

#include "pxr/base/tf/hashmap.h"
#include "pxr/base/tf/token.h"

#include <mutex>

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
    HdVP2Instancer(HdSceneDelegate* delegate, SdfPath const& id,
                      SdfPath const &parentInstancerId);

    ~HdVP2Instancer();

    VtMatrix4dArray ComputeInstanceTransforms(SdfPath const &prototypeId);

private:
    void _SyncPrimvars();

    //! Mutex guard for _SyncPrimvars().
    std::mutex _instanceLock;

    /*! Map of the latest primvar data for this instancer, keyed by
        primvar name. Primvar values are VtValue, an any-type; they are
        interpreted at consumption time (here, in ComputeInstanceTransforms).
    */
    TfHashMap<TfToken, HdVtBufferSource*, TfToken::HashFunctor> _primvarMap;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
