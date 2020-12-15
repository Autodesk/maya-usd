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

#include <AL/maya/utils/MayaHelperMacros.h>
#include <AL/usdmaya/Api.h>
#include <AL/usdmaya/ForwardDeclares.h>
#include <AL/usdmaya/utils/AttributeType.h>
#include <AL/usdmaya/utils/DgNodeHelper.h>
#include <AL/usdmaya/utils/ForwardDeclares.h>

#include <pxr/base/gf/half.h>
#include <pxr/usd/usd/attribute.h>

#include <maya/MAngle.h>
#include <maya/MDistance.h>
#include <maya/MGlobal.h>
#include <maya/MPlug.h>
#include <maya/MStatus.h>
#include <maya/MTime.h>

#include <string>
#include <vector>

namespace AL {
namespace usdmaya {
namespace fileio {
namespace translators {
//----------------------------------------------------------------------------------------------------------------------
/// \brief  Utility class that transfers DgNodes between Maya and USD.
/// \ingroup   translators
//----------------------------------------------------------------------------------------------------------------------
class DgNodeTranslator : public usdmaya::utils::DgNodeHelper
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
    virtual MObject createNode(
        const UsdPrim&        from,
        MObject               parent,
        const char*           nodeType,
        const ImporterParams& params);

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
    /// \return MS::kSuccess if ok
    AL_USDMAYA_PUBLIC
    static MStatus copyAttributes(const MObject& from, UsdPrim& to, const ExporterParams& params);

    /// \brief  A temporary solution. Given a custom attribute, if a translator handles it somehow
    /// (i.e. lazy approach to
    ///         not creating a schema), then overload this method and return true on the attribute
    ///         you are handling. This will prevent the attribute from being imported/exported as a
    ///         dynamic attribute.
    /// \param  usdAttr the attribute to test
    /// \return true if your translator is handling this attr
    AL_USDMAYA_PUBLIC
    virtual bool attributeHandled(const UsdAttribute& usdAttr);
};

//----------------------------------------------------------------------------------------------------------------------
} // namespace translators
} // namespace fileio
} // namespace usdmaya
} // namespace AL
//----------------------------------------------------------------------------------------------------------------------
