//
// Copyright 2021 Autodesk
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
#pragma once

#include <mayaUsd/base/api.h>

#include <pxr/base/tf/hashmap.h>
#include <pxr/base/vt/dictionary.h>
#include <pxr/usd/sdf/types.h>
#include <pxr/usd/usd/prim.h>

#include <functional>
#include <memory>

namespace MAYAUSD_NS_DEF {

// An edit router is used to direct USD edits to their destination in the scene
// graph.  This may be a layer, a variant, a USD payload file, etc.
class MAYAUSD_CORE_PUBLIC EditRouter
{
public:
    typedef std::shared_ptr<EditRouter> Ptr;

    virtual ~EditRouter();

    // Compute the routing data.  The context is immutable, and is input to
    // the computation of the routing data.  Routing data may be initialized,
    // so that acceptable defaults can be left unchanged.
    virtual void operator()(const PXR_NS::VtDictionary& context, PXR_NS::VtDictionary& routingData)
        = 0;
};

// Wrap an argument edit router callback for storage in the edit router map.
class MAYAUSD_CORE_PUBLIC CxxEditRouter : public EditRouter
{
public:
    using EditRouterCb = std::function<
        void(const PXR_NS::VtDictionary& context, PXR_NS::VtDictionary& routingData)>;

    CxxEditRouter(EditRouterCb cb)
        : _cb(cb)
    {
    }

    ~CxxEditRouter() override;

    void
    operator()(const PXR_NS::VtDictionary& context, PXR_NS::VtDictionary& routingData) override;

private:
    EditRouterCb _cb;
};

// Guard class to set the edit target to the argument editTarget, then 
// restore it to the stage's previous edit target.
class MAYAUSD_CORE_PUBLIC EditTargetGuard
{
public:

    EditTargetGuard(
        const PXR_NS::UsdPrim&       prim,
        const PXR_NS::UsdEditTarget& editTarget
    )
        : _prim(prim), _prevEditTarget(_prim.GetStage()->GetEditTarget())
    {
        setEditTarget(_prim, editTarget);
    }

    ~EditTargetGuard() {
        setEditTarget(_prim, _prevEditTarget);
    }

private:

    void setEditTarget(
        const PXR_NS::UsdPrim&       prim,
        const PXR_NS::UsdEditTarget& editTarget
    ) {
        prim.GetStage()->SetEditTarget(editTarget);
    }

    const PXR_NS::UsdPrim&       _prim;
    // Need a by-value copy of the previous edit target.  Keeping only a
    // reference leaves the edit target unchanged on EditTargetGuard
    // destruction, most likely because the contents of the reference is
    // changed to the new edit target by the EditTargetGuard constructor.
    const PXR_NS::UsdEditTarget _prevEditTarget;
};

using EditRouters
    = PXR_NS::TfHashMap<PXR_NS::TfToken, EditRouter::Ptr, PXR_NS::TfToken::HashFunctor>;

// Utility function that returns a layer for the argument operation.
// If no edit router exists for that operation, a nullptr is returned.
// The edit router is given the prim in the context with key "prim", and is
// expected to return the computed layer in the routingData with key "layer".
MAYAUSD_CORE_PUBLIC
PXR_NS::SdfLayerHandle
getEditRouterLayer(const PXR_NS::TfToken& operation, const PXR_NS::UsdPrim& prim);

// Retrieve the edit router for the argument operation.  If no such edit router
// exits, a nullptr is returned.
MAYAUSD_CORE_PUBLIC
EditRouter::Ptr getEditRouter(const PXR_NS::TfToken& operation);

// Register an edit router for the argument operation.
MAYAUSD_CORE_PUBLIC
void registerEditRouter(const PXR_NS::TfToken& operation, const EditRouter::Ptr& editRouter);

// Restore the default edit router for the argument operation, overwriting
// the currently-registered edit router.  Returns false if no such default
// exists.
MAYAUSD_CORE_PUBLIC
bool restoreDefaultEditRouter(const PXR_NS::TfToken& operation);

// Return built-in default edit routers.
EditRouters defaultEditRouters();

} // namespace MAYAUSD_NS_DEF
