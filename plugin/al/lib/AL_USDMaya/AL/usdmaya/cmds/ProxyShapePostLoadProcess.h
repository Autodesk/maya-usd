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
#include "AL/usdmaya/ForwardDeclares.h"
#include "AL/usdmaya/fileio/translators/TranslatorContext.h"

#include <pxr/pxr.h>

PXR_NAMESPACE_USING_DIRECTIVE

namespace AL {
namespace usdmaya {
namespace cmds {

//----------------------------------------------------------------------------------------------------------------------
/// \brief  When importing a new usd stage into an AL::usdmaya::nodes::ProxyShape, there are a
/// selection of post import
///         processes that need to take place in order to import any custom usd plugins found.
///
///         \li If any custom plugin prims are found, that represent DAG nodes (e.g. they are shapes
///         or transforms),
///             then a hierarchy of AL::usdmaya::nodes::Transform nodes will be generated so that
///             the USD transform values found on the prims, will have a direct mapping to a maya
///             transform. This allows use of the standard Maya scale/translate/rotate manipulators
///             to control maya nodes directly.
///         \li For every custom plugin prim found, that node will need to be translated into Maya,
///         and then connected
///             up with the other imported prims.
///         \li A certain amount of book keeping information will need to be generated, so that
///         variant switching can
///             be used to modify or remove those nodes at a later date.
///
///         This helper class manages those processes.
///
/// \ingroup commands
//----------------------------------------------------------------------------------------------------------------------
class ProxyShapePostLoadProcess
{
public:
    /// a mapping from an MObject to a UsdPrim
    typedef std::vector<std::pair<MObject, UsdPrim>> MObjectToPrim;

    /// \brief  called after a proxy shape has been created. Traverses the prim hierarchy and looks
    /// to see whether
    ///         any custom plugin translators need to be called, and performs additional
    ///         book-keeping.
    /// \param  shape the proxy shape node that has just been created
    /// \return MS::kSuccess if ok
    static MStatus initialise(nodes::ProxyShape* shape);

    /// \brief  called after all custom translation finished, to do some clean up or resetting.
    /// \param  shape the proxy shape node that has finished its custom translation.
    /// \return MS::kSuccess if ok
    static MStatus uninitialise(nodes::ProxyShape* shape);

    /// \brief  given a specific proxy shape, and a collection of UsdPrims that represent custom
    /// DagNode types, this
    ///         will generate a transform hierarchy that will allow you to map the UsdPrims to
    ///         equivalent maya transforms
    /// \param  shape the proxy shape these transforms are being pulled in from
    /// \param  schemaPrims an array of prims for which transforms should be created
    /// \param  proxyTransformPath a path to the transform node that resides above the proxy shape
    /// \param  objsToCreate a mapping from the MObjects created (for the transform nodes), to the
    /// UsdPrim that is represented \param  pushToPrim specifies the value of the pushToPrim
    /// attribute on the created transform nodes \param  readAnimatedValues specifies the value of
    /// the readAnimatedValues attribute on the created transform nodes
    static void createTranformChainsForSchemaPrims(
        nodes::ProxyShape*          shape,
        const std::vector<UsdPrim>& schemaPrims,
        const MDagPath&             proxyTransformPath,
        MObjectToPrim&              objsToCreate,
        bool pushToPrim = MGlobal::optionVarIntValue("AL_usdmaya_pushToPrim"),
        bool readAnimatedValues = MGlobal::optionVarIntValue("AL_usdmaya_readAnimatedValues"));

    /// \brief  After transforms exist to parent the custom plugin-prim types (i.e. after a call to
    ///         createTranformChainsForSchemaPrims), this method should be called to call the plugin
    ///         translators for all those nodes that should be imported into the Maya Scene.
    /// \param  proxy the proxy shape to create the schema prims on
    /// \param  objsToCreate the mapping returned from createTranformChainsForSchemaPrims
    /// \param  param the translator plugin options
    static void createSchemaPrims(
        nodes::ProxyShape*                               proxy,
        const std::vector<UsdPrim>&                      objsToCreate,
        const fileio::translators::TranslatorParameters& param
        = fileio::translators::TranslatorParameters());

    /// \brief  called once all plugin nodes have been created, and will request that each plugin
    /// translator performs
    ///         the postImport fixup to safely make any connections between Maya nodes
    /// \param  proxy the proxy shape to connect the schema prims
    /// \param  objsToCreate the mapping returned from createTranformChainsForSchemaPrims
    static void
    connectSchemaPrims(nodes::ProxyShape* proxy, const std::vector<UsdPrim>& objsToCreate);

    /// \brief  updates the list of UsdPrims after a variant switch (but when the nodes have not
    /// changed) \param  proxy the proxy shape to update \param  objsToUpdate the list of prims to
    /// be updated
    static void
    updateSchemaPrims(nodes::ProxyShape* proxy, const std::vector<UsdPrim>& objsToUpdate);

private:
    static fileio::ImporterParams m_params;
};

//----------------------------------------------------------------------------------------------------------------------
} // namespace cmds
} // namespace usdmaya
} // namespace AL
//----------------------------------------------------------------------------------------------------------------------
