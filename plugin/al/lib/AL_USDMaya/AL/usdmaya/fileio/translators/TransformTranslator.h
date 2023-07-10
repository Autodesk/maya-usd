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

#include <AL/usdmaya/Api.h>
#include <AL/usdmaya/ForwardDeclares.h>
#include <AL/usdmaya/TransformOperation.h>
#include <AL/usdmaya/fileio/translators/DagNodeTranslator.h>

#include <pxr/usd/usd/attribute.h>
#include <pxr/usd/usdGeom/xform.h>
#include <pxr/usd/usdGeom/xformCommonAPI.h>

#include <vector>

namespace AL {
namespace usdmaya {
namespace fileio {
namespace translators {

//----------------------------------------------------------------------------------------------------------------------
/// \brief  A utility class to transfer transform nodes between Maya and USD
/// \ingroup   translators
//----------------------------------------------------------------------------------------------------------------------
class TransformTranslator : public DagNodeTranslator
{
public:
    /// \brief  static type registration
    /// \return MS::kSuccess if ok
    AL_USDMAYA_PUBLIC
    static MStatus registerType();

    /// \brief  Creates a new maya node of the given type and set attributes based on input prim
    /// \param  from the UsdPrim to copy the data from
    /// \param  parent the parent Dag node to parent the newly created object under
    /// \param  nodeType the maya node type to create
    /// \param  params the importer params that determines what will be imported
    /// \return the newly created node
    AL_USDMAYA_PUBLIC
    MObject createNode(
        const UsdPrim&        from,
        MObject               parent,
        const char*           nodeType,
        const ImporterParams& params) override;

    /// \brief  helper method to copy attributes from the UsdPrim to the Maya node
    /// \param  from the UsdPrim to copy the data from
    /// \param  to the maya node to copy the data to
    /// \param  params the importer params to determine what to import
    /// \return MS::kSuccess if ok
    AL_USDMAYA_PUBLIC
    MStatus copyAttributes(const UsdPrim& from, MObject to, const ImporterParams& params);

    /// \brief  Copies data from the maya node onto the usd primitive
    /// \param  from the maya node to copy the data from
    /// \param  to the USD prim to copy the attributes to
    /// \param  params the exporter params to determine what should be exported
    /// \param  path the dag path
    /// \param  exportInWorldSpace parameter to determine if world space xform should be exported
    /// \return MS::kSuccess if ok
    AL_USDMAYA_PUBLIC
    static MStatus copyAttributes(
        const MObject&        from,
        UsdPrim&              to,
        const ExporterParams& params,
        const MDagPath&       path,
        bool                  exportInWorldSpace);

    /// \brief  copy the attribute value from the plug specified, at the given time, and store the
    /// data on the usdAttr. \param  attr the attribute to be copied \param  usdAttr the attribute
    /// to copy the data to \param  timeCode the timecode to use when setting the data
    AL_USDMAYA_PUBLIC
    static void
    copyAttributeValue(const MPlug& attr, UsdAttribute& usdAttr, const UsdTimeCode& timeCode);

    /// \brief  retrieve the corresponding maya attribute for the transform operation.
    /// \param  operation the transform operation we want the maya attribute handle for
    /// \param  attribute the returned attribute handle
    /// \param  conversionFactor a scaling that should be applied to the maya attributes to put them
    /// into the correct
    ///         units for USD
    /// \return true if the attribute is known to be animated, and the attribute/conversionFactor
    /// contain valid results
    AL_USDMAYA_PUBLIC
    static bool getAnimationVariables(
        TransformOperation operation,
        MObject&           attribute,
        double&            conversionFactor);

    /// \brief  helper method to copy attributes from the UsdPrim to the Maya node
    /// \param  attr the maya node to copy the data from
    /// \param  usdAttr the UsdPrim to copy the data to
    /// \param  timeCode Usd timecode to copy to
    /// \param  mergeOffsetMatrix flag if needs to merge offset parent matrix
    AL_USDMAYA_PUBLIC
    static void copyAttributeValue(
        const MPlug&       attr,
        UsdAttribute&      usdAttr,
        const UsdTimeCode& timeCode,
        bool               mergeOffsetMatrix);

    /// \brief  helper method to copy attributes from the UsdPrim to the Maya node
    /// \param  attr the maya node to copy the data from
    /// \param  usdAttr the UsdPrim to copy the data to
    /// \param  scale additional scale value to apply before copying to USD
    /// \param  timeCode Usd timecode to copy to
    /// \param  mergeOffsetMatrix flag if needs to merge offset parent matrix
    AL_USDMAYA_PUBLIC
    static void copyAttributeValue(
        const MPlug&       attr,
        UsdAttribute&      usdAttr,
        float              scale,
        const UsdTimeCode& timeCode,
        bool               mergeOffsetMatrix);

private:
    static MStatus processMetaData(const UsdPrim& from, MObject& to, const ImporterParams& params);

    static MObject m_inheritsTransform;
    static MObject m_scale;
    static MObject m_shear;
    static MObject m_rotation;
    static MObject m_rotationX;
    static MObject m_rotationY;
    static MObject m_rotationZ;
    static MObject m_rotateOrder;
    static MObject m_rotateAxis;
    static MObject m_rotateAxisX;
    static MObject m_rotateAxisY;
    static MObject m_rotateAxisZ;
    static MObject m_translation;
    static MObject m_scalePivot;
    static MObject m_rotatePivot;
    static MObject m_scalePivotTranslate;
    static MObject m_rotatePivotTranslate;
    static MObject m_selectHandle;
    static MObject m_transMinusRotatePivot;
    static MObject m_visibility;
};

//----------------------------------------------------------------------------------------------------------------------
} // namespace translators
} // namespace fileio
} // namespace usdmaya
} // namespace AL
//----------------------------------------------------------------------------------------------------------------------
