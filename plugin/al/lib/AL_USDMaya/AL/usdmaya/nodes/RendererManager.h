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
#include "AL/usdmaya/Api.h"

#include <pxr/base/tf/token.h>

#include <maya/MNodeMessage.h>
#include <maya/MPxNode.h>

/*
#include "AL/maya/utils/MayaHelperMacros.h"

#include <pxr/pxr.h>
#include <pxr/usd/usd/stage.h>

#include <maya/MNodeMessage.h>

#include <map>

*/

PXR_NAMESPACE_USING_DIRECTIVE

namespace AL {
namespace usdmaya {
namespace nodes {

class ProxyShape;

//----------------------------------------------------------------------------------------------------------------------
/// \brief  The layer manager node handles serialization and deserialization of all layers used by
/// all ProxyShapes \ingroup nodes
//----------------------------------------------------------------------------------------------------------------------
class RendererManager
    : public MPxNode
    , public AL::maya::utils::NodeHelper
{
public:
    /// \brief  ctor
    inline RendererManager()
        : MPxNode()
        , NodeHelper()
    {
    }

    /// \brief  Find the already-existing non-referenced RendererManager node in the scene, or
    /// return a null MObject \return the found RendererManager node, or a null MObject
    static MObject findNode();

    /// \brief  Either find the already-existing non-referenced RendererManager node in the scene,
    /// or make one \param dgmod An optional dgmodifier to create the node, if necessary. Note that
    /// if one is passed in,
    ///              createNode might be called on it, but doIt never will be, so the layer manager
    ///              node may not be added to the scene graph yet
    /// \param wasCreated If given, whether a new layer manager had to be created is stored here.
    /// \return the found-or-created RendererManager node
    static MObject findOrCreateNode(MDGModifier* dgmod = nullptr, bool* wasCreated = nullptr);

    /// \brief  Find the already-existing non-referenced RendererManager node in the scene, or
    /// return a nullptr \return the found RendererManager, or a nullptr
    static RendererManager* findManager();

    /// \brief  Either find the already-existing non-referenced RendererManager in the scene, or
    /// make one \param dgmod An optional dgmodifier to create the node, if necessary. Note that if
    /// one is passed in,
    ///              createNode might be called on it, but doIt never will be, so the layer manager
    ///              node may not be added to the scene graph yet
    /// \param wasCreated If given, whether a new layer manager had to be created is stored here.
    /// \return the found-or-created RendererManager
    static RendererManager*
    findOrCreateManager(MDGModifier* dgmod = nullptr, bool* wasCreated = nullptr);

    //--------------------------------------------------------------------------------------------------------------------
    /// Methods to handle renderer plugin
    //--------------------------------------------------------------------------------------------------------------------

    /// \brief  Set current renderer for all proxy shapes
    void onRendererChanged();

    /// \brief  Set current renderer plugin based on provided name
    bool setRendererPlugin(const MString& pluginName);

    /// \brief  Change current renderer plugin for provided ProxyShape
    void changeRendererPlugin(ProxyShape* proxy, bool creation = false);

    /// \brief  Get current renderer plugin index
    int getRendererPluginIndex() const;

    /// \brief  Get list of available Hydra renderer plugin names
    static const MStringArray& getRendererPluginList() { return m_rendererPluginsNames; }

    //--------------------------------------------------------------------------------------------------------------------
    /// Type Info & Registration
    //--------------------------------------------------------------------------------------------------------------------
    AL_MAYA_DECLARE_NODE();

    //--------------------------------------------------------------------------------------------------------------------
    /// Type Info & Registration
    //--------------------------------------------------------------------------------------------------------------------

    /// Hydra renderer plugin name used for rendering (storable)
    AL_DECL_ATTRIBUTE(rendererPluginName);
    /// Hydra renderer plugin index used for UI (internal)
    AL_DECL_ATTRIBUTE(rendererPlugin);

private:
    static MObject _findNode();

    static TfTokenVector m_rendererPluginsTokens;
    static MStringArray  m_rendererPluginsNames;

    //--------------------------------------------------------------------------------------------------------------------
    /// MPxNode overrides
    //--------------------------------------------------------------------------------------------------------------------

    bool setInternalValue(const MPlug& plug, const MDataHandle& dataHandle) override;

    bool getInternalValue(const MPlug& plug, MDataHandle& dataHandle) override;
};

} // namespace nodes
} // namespace usdmaya
} // namespace AL
//----------------------------------------------------------------------------------------------------------------------
