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

using EditRouters
    = PXR_NS::TfHashMap<PXR_NS::TfToken, EditRouter::Ptr, PXR_NS::TfToken::HashFunctor>;

MAYAUSD_CORE_PUBLIC
PXR_NS::SdfLayerHandle
getEditRouterLayer(const PXR_NS::TfToken& operation, const PXR_NS::UsdPrim& prim);

// Register an edit router for the argument operation.
MAYAUSD_CORE_PUBLIC
void registerEditRouter(const PXR_NS::TfToken& operation, const EditRouter::Ptr& editRouter);

// Return built-in default edit routers.
EditRouters editRouterDefaults();

} // namespace MAYAUSD_NS_DEF
