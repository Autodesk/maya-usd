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
#include <AL/usdmaya/fileio/translators/DgNodeTranslator.h>

#include <pxr/base/tf/token.h>

#include <maya/MObject.h>

namespace AL {
namespace usdmaya {
namespace fileio {
namespace translators {
//----------------------------------------------------------------------------------------------------------------------
/// \brief  A class to transfer dag node data between Usd <--> Maya
/// \ingroup   translators
//----------------------------------------------------------------------------------------------------------------------
class DagNodeTranslator : public DgNodeTranslator
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
    /// \return MS::kSuccess if ok
    static MStatus copyAttributes(const MObject& from, UsdPrim& to, const ExporterParams& params)
    {
        return MS::kSuccess;
    }

    /// \brief  assign the default material to the shape specified
    /// \param  shape the maya shape to assign a material to
    /// \return MS::kSuccess if ok
    AL_USDMAYA_PUBLIC
    MStatus applyDefaultMaterialOnShape(MObject shape);

    AL_USDMAYA_PUBLIC
    static void initialiseDefaultShadingGroup(MObject& target);

protected:
    /// an MObject handle to the initial shading group, which can be assigned to newly imported
    /// geometry so that default shading is applied to the shading group
    static MObject m_initialShadingGroup;

    /// the visibility attribute common to all dag nodes
    static MObject m_visible;
};

//----------------------------------------------------------------------------------------------------------------------
} // namespace translators
} // namespace fileio
} // namespace usdmaya
} // namespace AL
//----------------------------------------------------------------------------------------------------------------------
