//
// Copyright 2020 Autodesk
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
#ifndef MAYAUSD_PROXY_ACCESSOR_H
#define MAYAUSD_PROXY_ACCESSOR_H

#include <mayaUsd/base/api.h>
#include <mayaUsd/base/syncId.h>
#include <mayaUsd/nodes/proxyStageProvider.h>

#include <pxr/base/tf/notice.h>
#include <pxr/pxr.h>
#include <pxr/usd/usd/notice.h>
#include <pxr/usd/usd/stage.h>
#include <pxr/usd/usd/timeCode.h>

#include <maya/MCallbackIdArray.h>
#include <maya/MDataBlock.h>
#include <maya/MFnDependencyNode.h>
#include <maya/MFnNumericAttribute.h>
#include <maya/MObject.h>
#include <maya/MPlug.h>
#include <maya/MPlugArray.h>
#include <maya/MPxNode.h>
#include <maya/MStatus.h>

#include <memory>
#include <tuple>
#include <type_traits>

PXR_NAMESPACE_OPEN_SCOPE
class UsdGeomXformCache;
PXR_NAMESPACE_CLOSE_SCOPE

PXR_NAMESPACE_USING_DIRECTIVE

namespace MAYAUSD_NS_DEF {
struct ConverterArgs;
class Converter;
class ComputeContext;

/*! \brief Proxy accessor class enables an MPxNode node with ProxyStageProvider interface to
 write and read data from the USD stage.

 Proxy accessor will discover accessor dynamic attributes on MPxNode and categorize them as
 inputs or outputs.

 During compute, proxy accessor will read all inputs, i.e. accessor attributes which have source
 connection and write them to the stage and time provided by the ProxyStageProvider interface of
 the owning node. Output attributes are then read from the stage and written to the data block.

 Accessor attributes are dynamic attributes created on owning MPxNode and having the following
 characteristics:
 - attribute name is created using following formula - "AP_" + sanitized sdf path
 - attribute nice name is used to store SdfPath to prim or prim property
 - when no property is provided in the SdfPath, we assume the world matrix is requested. This
 makes only sense for output plugs.

 A proxy accessor is owned by MPxNode and extends base class methods. For more information read
 the documentation for the public interface of this class.

 \note Currently the only class leveraging proxy accessor is the proxy shape.
*/
class ProxyAccessor
{
public:
    using Owner = std::unique_ptr<ProxyAccessor>;

    /*! \brief  Construct ProxyAccessor for a given MPxNode with ProxyStageProvider interface
        \note   Call from MPxNode::postConstructor()
     */
    template <
        typename ProxyClass,
        typename std::enable_if<
            (std::is_base_of<MPxNode, ProxyClass>::value
             && std::is_base_of<ProxyStageProvider, ProxyClass>::value),
            int>::type
        = 0>
    static Owner createAndRegister(ProxyClass& proxyNode)
    {
        MPxNode&            node = static_cast<MPxNode&>(proxyNode);
        ProxyStageProvider& stageProvider = static_cast<ProxyStageProvider&>(proxyNode);

        auto accessor = Owner(new ProxyAccessor(stageProvider));

        // add hidden attribute to force compute when USD changes
        MFnDependencyNode fnDep(node.thisMObject());
        {
            MFnNumericAttribute attr;

            accessor->_forceCompute
                = attr.create("forceCompute", "forceCompute", MFnNumericData::kBoolean);

            attr.setReadable(false);
            attr.setWritable(true);
            attr.setKeyable(false);
            attr.setHidden(true);
            attr.setConnectable(false);

            fnDep.addAttribute(accessor->_forceCompute);
        }

        accessor->addCallbacks(node.thisMObject());

        return accessor;
    }

    /*! \brief  Insert extra plug level dependencies for accessor plugs
        \note   Call from MPxNode::setDependentsDirty()
     */
    static MStatus
    addDependentsDirty(const Owner& accessor, const MPlug& plug, MPlugArray& plugArray)
    {
        if (accessor)
            return accessor->addDependentsDirty(plug, plugArray);
        else
            return MStatus::kFailure;
    }

    /*! \brief  Compute will write input accessor plugs and write converted data to the stage.
       Once completed, all output accessor plugs will be provided with data. \note   Call from
       MPxNode::compute()
     */
    static MStatus compute(const Owner& accessor, const MPlug& plug, MDataBlock& dataBlock)
    {
        if (accessor)
            return accessor->compute(plug, dataBlock);
        else
            return MStatus::kFailure;
    }

    /*! \brief  Proxy accessor is creating acceleration structure to avoid the discovery of
     accessor plugs at each compute. This accelleration structure has to be invalidate when
     stage changes.
    */
    static MStatus stageChanged(
        const Owner&                     accessor,
        const MObject&                   node,
        const UsdNotice::ObjectsChanged& notice)
    {
        if (accessor && !accessor->inCompute())
            return accessor->stageChanged(node, notice);
        else
            return MStatus::kFailure;
    }

    /*! \brief  Update USD state to match what is stored in evaluation cache (when cached
       playback is on) \note   Call from MPxNode::postEvaluation()
     */
    static MStatus syncCache(const Owner& accessor, const MObject& node, MDataBlock& dataBlock)
    {
        if (accessor && !accessor->inCompute())
            return accessor->syncCache(node, dataBlock);
        else
            return MStatus::kFailure;
    }

    //! \brief  Clear all callback when destroying this object
    virtual ~ProxyAccessor() { removeCallbacks(); }

private:
    /*! \brief  Single item in acceleration structure holding.
        To avoid expensive searches during compute, we cache MPlug, SdfPath and converter needed
       to translate values between data models.
     */
    using Item = std::tuple<MPlug, SdfPath, const Converter*, SyncId>;
    using Container = std::vector<Item>;

    ProxyAccessor(ProxyStageProvider& provider)
        : _stageProvider(provider)
    {
    }
    ProxyAccessor() = delete;
    ProxyAccessor(const ProxyAccessor&) = delete;
    ProxyAccessor& operator=(const ProxyAccessor&) = delete;

    //! \brief  Makes access to current stage time easier
    UsdTimeCode getTime() const { return _stageProvider.getTime(); }
    //! \brief  Makes access to the stage easier
    UsdStageRefPtr getUsdStage() const { return _stageProvider.getUsdStage(); }

    //! \brief  Register necessary callbacks
    MStatus addCallbacks(MObject object);
    //! \brief  Remove all registered callbacks
    MStatus removeCallbacks();

    //! \brief  Populate acceleration structure
    void collectAccessorItems(MObject node);
    //! \brief  Invalidate acceleration structure
    void invalidateAccessorItems() { _validAccessorItems = false; }
    //! \brief  Find accessor item in the acceleration structure
    const Item* findAccessorItem(const MPlug& plug, bool isInput) const;

    //! \brief  Notification from MPxNode to insert accessor plugs dependencies
    MStatus addDependentsDirty(const MPlug& plug, MPlugArray& plugArray);
    //! \brief  Notification from MPxNode to compute accessor plugs.
    MStatus compute(const MPlug& plug, MDataBlock& dataBlock);
    //! \brief  Using acceleration structure, do computation of a given accessor input plug.
    MStatus computeInput(
        Item&                outputItemToCompute,
        const UsdStageRefPtr stage,
        MDataBlock&          dataBlock,
        const ConverterArgs& args);
    //! \brief  Using acceleration structure, do computation of a given accessor output plug.
    MStatus computeOutput(
        const Item&          outputItemToCompute,
        const MMatrix&       proxyInclusiveMatrix,
        const UsdStageRefPtr stage,
        MDataBlock&          dataBlock,
        UsdGeomXformCache&   xformCache,
        const ConverterArgs& args);

    /*! \brief  Notification from MPxNode to synchronize evaluation cache with USD stage
        Each manipulation can mutate the state of USD, but not every manipulation will
       invalidate the cache. In order to keep USD state in sync with what was stored in
       evaluation cache, we leverage postEvaluation notification.
    */
    MStatus syncCache(const MObject& node, MDataBlock& dataBlock);

    //! \brief  Something in USD changed and we may have to set it on plugs.
    MStatus stageChanged(const MObject& node, const UsdNotice::ObjectsChanged& notice);
    //! \brief  Trigger computation of accessor plugs
    MStatus forceCompute(const MObject& node);

    //! \brief  Is accessor compute started
    bool inCompute() const { return (_inCompute != nullptr); }

    ProxyStageProvider& _stageProvider; //!< Accessor holds reference to stage provider in order
                                        //!< to query the stage and time

    MObject _forceCompute; //!< Special attribute used to force computation of accessor plugs.
                           //!< Needed when USD changes directly.

    MCallbackIdArray _callbackIds; //!< List of registered callbacks

    //! \brief  Acceleration structure holding all input accessor plugs
    Container _accessorInputItems;
    //! \brief  Acceleration structure holding all output accessor plugs
    Container _accessorOutputItems;

    ComputeContext* _inCompute {
        nullptr
    }; //!< Detect nested compute and provide access to top level context

    //! \brief  Current evaluation id. Used to prevent endless recursion when computing cyclic
    //! dependencies
    Id _evaluationId;

    //! \brief  Flag to indicate if acceleration structure is valid or needs to be recreated
    bool _validAccessorItems { false };

    friend ComputeContext;
};

} // namespace MAYAUSD_NS_DEF

#endif
