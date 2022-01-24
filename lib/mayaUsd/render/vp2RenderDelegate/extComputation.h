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

#ifndef HD_VP2_EXTCOMPUTATION
#define HD_VP2_EXTCOMPUTATION

#include <pxr/base/tf/hashmap.h>
#include <pxr/base/tf/token.h>
#include <pxr/imaging/hd/extComputation.h>
#include <pxr/imaging/hd/vtBufferSource.h>
#include <pxr/pxr.h>

#include <mutex>

PXR_NAMESPACE_OPEN_SCOPE

/*! \brief  VP2 computation
    \class  HdVP2ExtComputation
*/

class HdVP2ExtComputation : public HdExtComputation
{
public:
    /// Construct a new ExtComputation identified by id.
    HdVP2ExtComputation(SdfPath const& id);

    ~HdVP2ExtComputation() override;

    void Sync(HdSceneDelegate* sceneDelegate, HdRenderParam* renderParam, HdDirtyBits* dirtyBits)
        override;

private:
    // No default construction or copying
    HdVP2ExtComputation() = delete;
    HdVP2ExtComputation(const HdVP2ExtComputation&) = delete;
    HdVP2ExtComputation& operator=(const HdVP2ExtComputation&) = delete;

    bool _evaluatedInputs { false };
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
