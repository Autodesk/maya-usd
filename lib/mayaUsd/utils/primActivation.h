//
// Copyright 2022 Autodesk
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
#ifndef MAYAUSD_ACTIVATION_H
#define MAYAUSD_ACTIVATION_H

#include <mayaUsd/base/api.h>

#include <pxr/usd/sdf/path.h>
#include <pxr/usd/usd/stage.h>

#include <ufe/path.h>

namespace MAYAUSD_NS_DEF {

//! \brief  Change the active status of a prim.
//
// Record the previous activation status of ancestors so that they can be
// restored once the manipulation is done. This is necessary because children
// of a deactivated prim cannot be accessed nor modified in USD. We must first
// activate all ancestor, do the modifications, then restore the ancestor
// activation state.
//
// The temporary activations are done in the session layer.

class MAYAUSD_CORE_PUBLIC PrimActivation
{
public:
    //! \brief empty prim activation. Allow delayed initialization, for example
    //         inside a conditional.
    PrimActivation() = default;

    //! \brief make the prim at the given path accessible.
    PrimActivation(const PXR_NS::UsdStagePtr& stage, const PXR_NS::SdfPath& path);

    //! \brief make the prim at the given path accessible.
    PrimActivation(const Ufe::Path& path);

    //! \brief restore the previous activation status of ancestors.
    ~PrimActivation();

    //! \brief restore the previous activation status of ancestors.
    void restore();

private:
    PXR_NS::UsdStagePtr _stage;

    // Record prims that had de-activation opinions already
    // authored in the session layer. Those are the opinions
    // that need to be explicitly restored as inactive.
    PXR_NS::SdfPathSet _previouslyInactive;

    // Record prims that had de-activation opinions already
    // authored in the layers below the session layer. Those
    // are the opinions that need to be explicitly cleared.
    PXR_NS::SdfPathSet _forcedActive;
};

} // namespace MAYAUSD_NS_DEF

#endif // MAYAUSD_ACTIVATION_H
