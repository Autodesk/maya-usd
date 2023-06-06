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

#include <usdUfe/base/api.h>

#include <pxr/base/tf/hashmap.h>
#include <pxr/base/vt/dictionary.h>
#include <pxr/usd/sdf/types.h>
#include <pxr/usd/usd/prim.h>

#include <functional>
#include <memory>

namespace USDUFE_NS_DEF {

// An edit router is used to direct USD edits to their destination in the scene
// graph.  This may be a layer, a variant, a USD payload file, etc.
class USDUFE_PUBLIC EditRouter
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
class USDUFE_PUBLIC CxxEditRouter : public EditRouter
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

// Utility function that returns a layer for the argument operation.
// If no edit router exists for that operation, a nullptr is returned.
// The edit router is given the prim in the context with key "prim", and is
// expected to return the computed layer in the routingData with key "layer".
USDUFE_PUBLIC
PXR_NS::SdfLayerHandle
getEditRouterLayer(const PXR_NS::TfToken& operation, const PXR_NS::UsdPrim& prim);

// Retrieve the edit router for the argument operation.  If no such edit router
// exits, a nullptr is returned.
USDUFE_PUBLIC
EditRouter::Ptr getEditRouter(const PXR_NS::TfToken& operation);

// Retrieve the layer for the attribute operation. If no edit router for the
// "attribute" operation is found, a nullptr is returned.
USDUFE_PUBLIC
PXR_NS::SdfLayerHandle
getAttrEditRouterLayer(const PXR_NS::UsdPrim& prim, const PXR_NS::TfToken& attrName);

// Register an edit router for the argument operation.
USDUFE_PUBLIC
void registerEditRouter(const PXR_NS::TfToken& operation, const EditRouter::Ptr& editRouter);

// Restore the default edit router for the argument operation, overwriting
// the currently-registered edit router.  Returns false if no such operation
// exists among the registered edit routers.
USDUFE_PUBLIC
bool restoreDefaultEditRouter(const PXR_NS::TfToken& operation);

// Restore all the default edit routers, overwriting the currently-registered edit routers.
// Also remove all routers that have no default.
USDUFE_PUBLIC
void restoreAllDefaultEditRouters();

// Register a default router which will be added to the list which is returned by
// defaultEditRouters().
USDUFE_PUBLIC
void registerDefaultEditRouter(const PXR_NS::TfToken&, const EditRouter::Ptr&);

// Return built-in default edit routers.
EditRouters defaultEditRouters();

} // namespace USDUFE_NS_DEF
