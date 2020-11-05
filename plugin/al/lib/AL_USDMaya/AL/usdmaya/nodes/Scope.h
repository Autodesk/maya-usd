//
// Copyright 2019 Animal Logic
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

#include "AL/maya/utils/MayaHelperMacros.h"
#include "AL/maya/utils/NodeHelper.h"
#include "AL/usdmaya/TypeIDs.h"
#include "AL/usdmaya/nodes/BasicTransformationMatrix.h"

#include <pxr/usd/usdGeom/scope.h>

#include <maya/MObjectHandle.h>
#include <maya/MPxTransform.h>
#include <maya/MPxTransformationMatrix.h>

namespace AL {
namespace usdmaya {
namespace nodes {

//----------------------------------------------------------------------------------------------------------------------
/// \brief  The AL::usdmaya::nodes::Scope node is a custom transform node that represents a USD
//          scope type prim directly from inside Maya. It works by providing a custom MPxTransform
//          node which uses a
///         custom MPxTransformationMatrix type (AL::usdmaya::nodes::BasicTransformationMatrix).
///
///         As it's fairly simple, we also use it as the interface for other Transform
///         implementations
///
///         Typically this node should have one input connection:
///          \li \b inStageData - connected from the output stage data of an
///          AL::usdmaya::nodes::ProxyShape
///
///         The following attribute determines which UsdPrim is being watched:
///          \li \b primPath - a Usd path of the prim being watched, e.g.  "/root/foo/pCube1"
///
/// \ingroup nodes
//----------------------------------------------------------------------------------------------------------------------

#if MAYA_API_VERSION >= 20190200 && MAYA_API_VERSION < 20200000
class Scope
    : public MPxTransform_BoundingBox
    , public AL::maya::utils::NodeHelper
#else
class Scope
    : public MPxTransform
    , public AL::maya::utils::NodeHelper
#endif
{
public:
    Scope();
    virtual ~Scope();

    //--------------------------------------------------------------------------------------------------------------------
    // Type Info & Registration
    //--------------------------------------------------------------------------------------------------------------------
    AL_MAYA_DECLARE_NODE();

    //--------------------------------------------------------------------------------------------------------------------
    // Input Attributes
    //--------------------------------------------------------------------------------------------------------------------
    AL_DECL_ATTRIBUTE(primPath);
    AL_DECL_ATTRIBUTE(inStageData);

    //--------------------------------------------------------------------------------------------------------------------
    /// \name Methods
    //--------------------------------------------------------------------------------------------------------------------

    /// \brief  returns the transformation matrix for this transform node
    /// \return the transformation matrix
    inline BasicTransformationMatrix* transform() const
    {
        return reinterpret_cast<BasicTransformationMatrix*>(transformationMatrixPtr());
    }

    virtual const MObject getProxyShape() const { return proxyShapeHandle.object(); }

    MBoundingBox boundingBox() const override;

protected:
    //--------------------------------------------------------------------------------------------------------------------
    /// virtual overrides
    //--------------------------------------------------------------------------------------------------------------------
    MStatus compute(const MPlug& plug, MDataBlock& datablock) override;

private:
    MPxNode::SchedulingType schedulingType() const override { return kParallel; }

    MStatus validateAndSetValue(
        const MPlug&       plug,
        const MDataHandle& handle,
        const MDGContext&  context) override;
    MPxTransformationMatrix* createTransformationMatrix() override;
    void                     postConstructor() override;
    MStatus connectionMade(const MPlug& plug, const MPlug& otherPlug, bool asSrc) override;
    MStatus connectionBroken(const MPlug& plug, const MPlug& otherPlug, bool asSrc) override;
    bool    isBounded() const override { return true; }
    bool    treatAsTransform() const override { return false; }

    //--------------------------------------------------------------------------------------------------------------------
    /// \name Input Attributes
    //--------------------------------------------------------------------------------------------------------------------

    /// \var    MPlug primPathPlug() const;
    /// \brief  access the primPath attribute plug on this node instance.
    ///         primPath - a Usd path of the prim being watched, e.g.  "/root/foo/pCube1"
    /// \return the plug to the primPath attribute
    /// \var    MPlug inStageDataPlug() const;
    /// \brief  access the inStageData attribute plug on this node instance
    ///         inStageData - connected from the output stage data of an
    ///         AL::usdmaya::nodes::ProxyShape
    /// \return the plug to the inStageData attribute
    /// \var    static MObject primPath();
    /// \brief  access the primPath attribute handle
    /// \return the primPath attribute
    /// \var    static MObject inStageData();
    /// \brief  access the inStageData attribute handle
    /// \return the inStageData attribute

    MObjectHandle proxyShapeHandle;
};

//----------------------------------------------------------------------------------------------------------------------
} // namespace nodes
} // namespace usdmaya
} // namespace AL
//----------------------------------------------------------------------------------------------------------------------
