//
// Copyright 2017 Animal Logic
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
#include "AL/usdmaya/ForwardDeclares.h"
#include "AL/usdmaya/nodes/Scope.h"

#include <maya/MObjectHandle.h>
#include <maya/MPxTransform.h>

namespace AL {
namespace usdmaya {
namespace nodes {

class TransformationMatrix;

//----------------------------------------------------------------------------------------------------------------------
/// \brief  The AL::usdmaya::nodes::Transform node is a custom transform node that allows you to
/// manipulate a USD
//          transform type directly from inside Maya. It works by providing a custom MPxTransform
//          node which uses a
///         custom MPxTransformationMatrix type (AL::usdmaya::nodes::TransformationMatrix). The
///         custom transformation matrix listens for changes that affect the transform (e.g.
///         rotateBy, translateBy, etc), and if \b pushToPrim is enabled, applies those changes to
///         the USD transformation.
///
///         Typically this node should have two input connections:
///          \li \b inStageData - connected from the output stage data of an
///          AL::usdmaya::nodes::ProxyShape \li \b time - (probably) connected from the output time
///          of an AL::usdmaya::nodes::ProxyShape, or directly to the time1.outAttr or equivalent.
///
///
///         The following attributes can be used to scale and offset the time values:
///          \li \b timeOffset - an offset (in current UI time units) of say 30, means animation
///          wont start until frame 30. \li \b timeScalar - a speed multiplier. 2.0 will double the
///          playback speed, 0.5 will halve it.
///
///
///         The following attribute determines which UsdPrim is being watched:
///          \li \b primPath - a Usd path of the prim being watched, e.g.  "/root/foo/pCube1"
///
///
///         We then have these two attributes:
///          \li \b localTranslateOffset - an offset applied *after* all other transforms. Useful
///          for positioning items on a table. \li \b pushToPrim - When enabled, any changes you
///          make to the transform values in maya, will be pushed back onto the USD primitive.
///
///
///         Finally we have the following outputs:
///          \li \b outTime = (time - timeOffset) * timeScalar
///
///
/// \todo   General todo list, and other quirks....
///         -# pushToPrim when enabled, does not add transform operations into the UsdPrim it is
///         tracking. So for
///            example, if you have a prim with no transform ops, not much is going to happen. If
///            however your prim has the full spectrum of rotate axis, translate, scale, rotate,
///            shear, etc; then you will be able to have full control over the prim. This will need
///            to be addressed at some point soon. One of the more challenging aspects here is that
///            we will need to modify a) the geom op order [e.g. insert a scale op, where there was
///            not one before]; and b) rotation is going to be a PITA [There may be a rotateX op,
///            but after modification that may need to be deleted, and replaced with a rotateXYZ op]
///         -# If pushToPrim is disabled, any modifications to the transform values are stored as
///         offsets from the USD
///            prim values. This works quite well for local space operations such as scale and
///            rotation, semi-works for translation [effectively this is a parent space translation
///            offset - useful for moving an anim clip] However for values such as rotation and
///            scale pivots, yeah, the result might be a little strange.
///         -# I'm not convinced the way that I've organised compute and validateAndSetValue is
///         ideal. It works,
///            but if anyone has some improvements to suggest, I'm all ears.
///         -# Generally speaking, when localTranslateOffset is (0,0,0), then the
///         translate/rotate/scale tools work quite
///            well. If however localTranslateOffset is not (0,0,0), then the behaviour of the
///            rotate tool is a little odd. Really this should be taken into account within the
///            AL::usdmaya::nodes::TransformationMatrix::rotateBy and
///            AL::usdmaya::nodes::TransformationMatrix::rotateTo methods.
///         -# If the usd prim xform stack has only one pivot, any separate modifications of
///            scale/rotate pivot in maya will result in an undefined behavior.
/// \ingroup nodes
//----------------------------------------------------------------------------------------------------------------------

class Transform : public Scope
{
public:
    Transform();
    ~Transform();

    //--------------------------------------------------------------------------------------------------------------------
    // Type Info & Registration
    //--------------------------------------------------------------------------------------------------------------------
    AL_MAYA_DECLARE_NODE();

    //--------------------------------------------------------------------------------------------------------------------
    // Input Attributes
    //--------------------------------------------------------------------------------------------------------------------
    AL_DECL_ATTRIBUTE(time);
    AL_DECL_ATTRIBUTE(timeOffset);
    AL_DECL_ATTRIBUTE(timeScalar);
    AL_DECL_ATTRIBUTE(localTranslateOffset);
    AL_DECL_ATTRIBUTE(pushToPrim);
    AL_DECL_ATTRIBUTE(readAnimatedValues);

    //--------------------------------------------------------------------------------------------------------------------
    // Output Attributes
    //--------------------------------------------------------------------------------------------------------------------
    AL_DECL_ATTRIBUTE(outTime);

    //--------------------------------------------------------------------------------------------------------------------
    /// \name Methods
    //--------------------------------------------------------------------------------------------------------------------

    const MObject getProxyShape() const override { return proxyShapeHandle.object(); }

    inline TransformationMatrix* getTransMatrix() const
    {
        return reinterpret_cast<TransformationMatrix*>(transformationMatrixPtr());
    }

private:
    //--------------------------------------------------------------------------------------------------------------------
    /// virtual overrides
    //--------------------------------------------------------------------------------------------------------------------

    MPxNode::SchedulingType schedulingType() const override { return kParallel; }

    MStatus validateAndSetValue(
        const MPlug&       plug,
        const MDataHandle& handle,
        const MDGContext&  context) override;
    MPxTransformationMatrix* createTransformationMatrix() override;
    MStatus                  compute(const MPlug& plug, MDataBlock& datablock) override;
    void                     postConstructor() override;
    MStatus connectionMade(const MPlug& plug, const MPlug& otherPlug, bool asSrc) override;
    MStatus connectionBroken(const MPlug& plug, const MPlug& otherPlug, bool asSrc) override;
    bool    setInternalValue(const MPlug& plug, const MDataHandle& dataHandle) override;
    bool    isBounded() const override { return true; }
    bool    treatAsTransform() const override { return false; }

    //--------------------------------------------------------------------------------------------------------------------
    /// utils
    //--------------------------------------------------------------------------------------------------------------------

    void updateTransform(MDataBlock& dataBlock);

    //--------------------------------------------------------------------------------------------------------------------
    /// Data members
    //--------------------------------------------------------------------------------------------------------------------
    bool updateTransformInProgress = false;

    //--------------------------------------------------------------------------------------------------------------------
    /// \name Input Attributes
    //--------------------------------------------------------------------------------------------------------------------

    /// \var    MPlug timePlug() const;
    /// \brief  access the time attribute plug on this node instance
    ///         time - (probably) connected from the output time of an
    ///         AL::usdmaya::nodes::ProxyShape (but it doesn't have to be)
    /// \return the plug to the time attribute
    /// \var    MPlug timeOffsetPlug() const;
    /// \brief  access the timeOffset attribute plug on this node instance
    ///         timeOffset - an offset (in current UI time units) of say 30, means animation wont
    ///         start until frame 30.
    /// \return the plug to the timeOffset attribute
    /// \var    MPlug timeScalarPlug() const;
    /// \brief  access the timeScalar attribute plug on this node instance
    ///         timeScalar - a speed multiplier. 2.0 will double the playback speed, 0.5 will halve
    ///         it.
    /// \return the plug to the timeScalar attribute
    /// \var    MPlug localTranslateOffsetPlug() const;
    /// \brief  access the localTranslateOffset attribute plug on this node instance
    ///         localTranslateOffset - an offset applied *after* all other transforms. Useful for
    ///         positioning items on a table.
    /// \return the plug to the localTranslateOffset attribute
    /// \var    MPlug pushToPrimPlug() const;
    /// \brief  access the pushToPrim attribute plug on this node instance
    ///         pushToPrim - When enabled, any changes you make to the transform values in maya,
    ///         will be pushed back onto the USD primitive.
    /// \return the plug to the pushToPrim attribute
    /// \var    MPlug readAnimatedValuesPlug() const;
    /// \brief  With USD, we typically have two possible sets of values on any given attribute.
    /// There is a 'default' value,
    ///         and the keyframed values. If this plug is true, the transform node will read the
    ///         animated values. If the plug is false then the default value will be read.
    /// \return the plug to the readAnimatedValues attribute

    /// \var    static MObject primPath();
    /// \brief  access the primPath attribute handle
    /// \return the primPath attribute
    /// \var    static MObject inStageData();
    /// \brief  access the inStageData attribute handle
    /// \return the inStageData attribute
    /// \var    static MObject time();
    /// \brief  access the time attribute handle
    /// \return the time attribute
    /// \var    static MObject timeOffset();
    /// \brief  access the timeOffset attribute handle
    /// \return the timeOffset attribute
    /// \var    static MObject timeScalar();
    /// \brief  access the timeScalar attribute handle
    /// \return the timeScalar attribute
    /// \var    static MObject pushToPrim();
    /// \brief  access the pushToPrim attribute handle
    /// \return the pushToPrim attribute
    /// \var    static MObject localTranslateOffset();
    /// \brief  access the localTranslateOffset attribute handle
    /// \return the localTranslateOffset attribute
    /// \var    static MObject readAnimatedValues();
    /// \brief  access the readAnimatedValues attribute handle
    /// \return the readAnimatedValues attribute

    //--------------------------------------------------------------------------------------------------------------------
    /// \name Output Attributes
    //--------------------------------------------------------------------------------------------------------------------

    /// \var    MPlug outTimePlug() const;
    /// \brief  access the outTime attribute plug on this node instance
    ///         outTime = (time - timeOffset) * timeScalar
    /// \return the plug to the outTime attribute
    /// \var    static MObject outTime();
    /// \brief  access the outTime attribute handle
    /// \return the outTime attribute

    MObjectHandle proxyShapeHandle;
};

//----------------------------------------------------------------------------------------------------------------------
} // namespace nodes
} // namespace usdmaya
} // namespace AL
//----------------------------------------------------------------------------------------------------------------------
