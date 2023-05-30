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

#include <pxr/base/tf/stacked.h>
#include <pxr/base/tf/token.h>
#include <pxr/usd/sdf/layer.h>
#include <pxr/usd/usd/editTarget.h>
#include <pxr/usd/usd/prim.h>
#include <pxr/usd/usd/stage.h>

namespace USDUFE_NS_DEF {

/*! \brief Base class for recursive edit router context.
 *
 * Only its sub-classes should be used, which is why its constructor is protected.
 */

class USDUFE_PUBLIC StackedEditRouterContext : public PXR_NS::TfStacked<StackedEditRouterContext>
{
public:
    /*! \brief Retrieve the current targeted layer.
     * \return The targeted layer. Null if the edit target was not changed.
     */
    const PXR_NS::SdfLayerHandle& getLayer() const;

    /*! \brief Retrieve the routed stage.
     * \return The stage that is being routed. Null if the edit target was not changed.
     */
    PXR_NS::UsdStagePtr getStage() const;

protected:
    /*! \brief Check if an edit context higher-up in the call-stack of this
     *         thread already routed the edits to a specific layer.
     */
    bool isTargetAlreadySet() const;

    /*! \brief Set the edit target of the given stage to the given layer.
     *         If the layer is null, then the target is not changed.
     */
    StackedEditRouterContext(const PXR_NS::UsdStagePtr& stage, const PXR_NS::SdfLayerHandle& layer);
    ~StackedEditRouterContext();

private:
    PXR_NS::UsdStagePtr    _stage;
    PXR_NS::SdfLayerHandle _layer;
    PXR_NS::UsdEditTarget  _previousTarget;
};

/*! \brief Select the target layer for an operation, via the edit router.
 *
 * Commands and other code that wish to be routable via an operation name
 * should use this instead of the native USD USdEditContext class.
 *
 * Support nesting properly, so that if a composite command is routed to a layer,
 * all sub-commands will use that layer and not individually routted layers. The
 * nesting is per-thread.
 *
 * We may add ways for edit routers of sub-command to force routing to a different
 * layer in the future. Using this class will make this transparent.
 */

class USDUFE_PUBLIC OperationEditRouterContext : public StackedEditRouterContext
{
public:
    /*! \brief Route the given operation on a prim.
     */
    OperationEditRouterContext(const PXR_NS::TfToken& operationName, const PXR_NS::UsdPrim& prim);

    /*! \brief Route to the given stage and layer.
     *  Should be used in undo to ensure the same target is used as in the initial execution.
     */
    OperationEditRouterContext(
        const PXR_NS::UsdStagePtr&    stage,
        const PXR_NS::SdfLayerHandle& layer);

private:
    PXR_NS::SdfLayerHandle
    getOperationLayer(const PXR_NS::TfToken& operationName, const PXR_NS::UsdPrim& prim);
};

/*! \brief Select the target layer when modifying USD attributes, via the edit router.
 *
 * Commands and other code that wish to be routable when modifying an USD attribute
 * should use this instead of the native USD USdEditContext class.
 *
 * Support nesting properly, so that if a composite command is routed to a layer,
 * all sub-commands will use that layer and not individually routted layers. The
 * nesting is per-thread.
 *
 * We may add ways for edit routers of sub-command to force routing to a different
 * layer in the future. Using this class will make this transparent.
 */
class USDUFE_PUBLIC AttributeEditRouterContext : public StackedEditRouterContext
{
public:
    /*! \brief Route an attribute operation on a prim for the given attribute.
     */
    AttributeEditRouterContext(const PXR_NS::UsdPrim& prim, const PXR_NS::TfToken& attributeName);

    /*! \brief Route to the given stage and layer.
     *  Should be used in undo to ensure the same target is used as in the initial execution.
     */
    AttributeEditRouterContext(
        const PXR_NS::UsdStagePtr&    stage,
        const PXR_NS::SdfLayerHandle& layer);

private:
    PXR_NS::SdfLayerHandle
    getAttributeLayer(const PXR_NS::UsdPrim& prim, const PXR_NS::TfToken& attributeName);
};

} // namespace USDUFE_NS_DEF
