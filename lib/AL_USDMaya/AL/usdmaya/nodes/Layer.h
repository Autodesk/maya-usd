//
// Copyright 2017 Animal Logic
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.//
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
#include "AL/usdmaya/Common.h"
#include "AL/maya/NodeHelper.h"
#include "pxr/pxr.h"
#include "pxr/usd/usd/stage.h"

#include "maya/MPxLocatorNode.h"

#include <map>
#include <set>

PXR_NAMESPACE_USING_DIRECTIVE

namespace AL {
namespace usdmaya {
namespace nodes {


//----------------------------------------------------------------------------------------------------------------------
/// \brief  The layer node stores a reference to an SdfLayer.
/// \ingroup nodes
//----------------------------------------------------------------------------------------------------------------------
class Layer
  : public MPxNode,
    public maya::NodeHelper
{
public:

  /// \brief  ctor
  inline Layer()
    : MPxNode(), NodeHelper(), m_handle(), m_shape(0) {}

  /// \brief  Called within the proxy shape to initialise the layer to the specified proxy shape and layer handle.
  /// \param  shape the proxy shape
  /// \param  handle the layer to initialise this node to
  void init(ProxyShape* shape, SdfLayerRefPtr handle);

  /// \brief  get access to the internal layer handle this node represents
  /// \return the layer handle for this node
  inline SdfLayerHandle getHandle()
    { return m_handle; }

  /// \brief  convert a usd display name into something maya can use as a node name
  /// \param  name the name of a usd prim
  /// \return the maya node name for that prim
  static MString toMayaNodeName(std::string name);

  //--------------------------------------------------------------------------------------------------------------------
  /// Methods to work with sublayers
  //--------------------------------------------------------------------------------------------------------------------

  /// \brief  returns an array of all the sub layers connected to this layer
  std::vector<Layer*> getSubLayers();

  /// \brief  returns an array of all the child layers connected to this layer (assets essentially)
  std::vector<Layer*> getChildLayers();

  /// \brief  constructs the sub layers after a proxy shape has loaded.
  /// \param  pmodifier pointer to a modifier that will record the nodes added (primarily if you need to keep hold of
  ///         information for undo later). doIt() will NOT have been called prior to the function returning.
  void buildSubLayers(MDGModifier* pmodifier = 0);

  /// \brief  adds a new sub layer to this layer
  /// \param  subLayer the new layer to add as a sub layer of this one
  /// \param  pmodifier optional modifier to use to build up undo/redo. This method does not call do it on the modifier.
  ///         If no modifier is specified, then the connections are made immediately.
  void addSubLayer(Layer* subLayer, MDGModifier* pmodifier = 0);

  /// \brief  removes a sub layer from this layer
  /// \param  subLayer the layer to remove
  bool removeSubLayer(Layer* subLayer);

  /// \brief  removes a child layer from this layer
  /// \param  childLayer the layer to remove
  bool removeChildLayer(Layer* childLayer);

  /// \brief  adds a new child layer to this layer
  /// \param  childLayer the new layer to add as a child layer of this one
  /// \param  modifier optional modifier to use to build up undo/redo. This method does not call do it on the modifier.
  ///         If no modifier is specified, then the connections are made immediately.
  void addChildLayer(Layer* childLayer, MDGModifier* modifier = 0);

  /// \brief  returns the parent layer (or NULL)
  /// \return the parent layer (or null)
  Layer* getParentLayer();

  /// \brief  returns the plug to the parent layer message attribute
  MPlug parentLayerPlug();

  /// \brief  locate a layer within usdmaya's recorded sublayers and referenced layers
  /// \param  handle the handle to the layer to locate
  /// \return a pointer to the layer if found, NULL otherwise.
  Layer* findLayer(SdfLayerHandle handle);
  //----------------------------------------------------------------------------------------------------------------------
  /// \brief  locate a layer within usdmaya's recorded sublayers
  /// \param  handle the handle to the layer to locate
  /// \return a pointer to the layer if found, NULL otherwise.
  Layer* findSubLayer(SdfLayerHandle handle);
  //----------------------------------------------------------------------------------------------------------------------
  /// \brief  locate a layer within usdmaya's recorded childlayers
  /// \param  handle the handle to the layer to locate
  /// \return a pointer to the layer if found, NULL otherwise.
  Layer* findChildLayer(SdfLayerHandle handle);
  //--------------------------------------------------------------------------------------------------------------------
  /// Methods to handle the saving and restoring of layer data
  //--------------------------------------------------------------------------------------------------------------------

  /// \brief  return true if this layer has been set as the edit target at some point during the proxy having been loaded
  ///         into maya.
  bool hasBeenTheEditTarget() const;

  /// \brief  Sets a flag that indicates whether this layer has been set as the edit target.
  /// \param  value
  void setHasBeenTheEditTarget(bool value);

  /// \brief  If the 'serialized' string attribute has data, then this method will initialize the layer specified to
  ///         the contents of the serialized attribute. This is primarily a mechanism for storing edits when the
  ///         maya scene is saved.
  /// \param  handle the handle to initialize
  void setLayerAndClearAttribute(SdfLayerHandle handle);

  /// \brief  If the 'hasBeenTheEditTarget' flag is true, this method will copy the contents of the layer this node
  ///         is referencing, into the 'serialized' attribute prior to saving the Maya file.
  void populateSerialisationAttributes();

  //--------------------------------------------------------------------------------------------------------------------
  /// Type Info & Registration
  //--------------------------------------------------------------------------------------------------------------------
  AL_MAYA_DECLARE_NODE();

  //--------------------------------------------------------------------------------------------------------------------
  /// Type Info & Registration
  //--------------------------------------------------------------------------------------------------------------------

  AL_DECL_ATTRIBUTE(comment);
  AL_DECL_ATTRIBUTE(defaultPrim);
  AL_DECL_ATTRIBUTE(documentation);
  AL_DECL_ATTRIBUTE(startTime);
  AL_DECL_ATTRIBUTE(endTime);
  AL_DECL_ATTRIBUTE(timeCodesPerSecond);
  AL_DECL_ATTRIBUTE(framePrecision);
  AL_DECL_ATTRIBUTE(owner);
  AL_DECL_ATTRIBUTE(sessionOwner);
  AL_DECL_ATTRIBUTE(permissionToEdit);
  AL_DECL_ATTRIBUTE(permissionToSave);
  AL_DECL_ATTRIBUTE(proxyShape);
  AL_DECL_ATTRIBUTE(subLayers);
  AL_DECL_ATTRIBUTE(childLayers);
  AL_DECL_ATTRIBUTE(parentLayer);

  // read only identification
  AL_DECL_ATTRIBUTE(displayName);
  AL_DECL_ATTRIBUTE(realPath);
  AL_DECL_ATTRIBUTE(fileExtension);
  AL_DECL_ATTRIBUTE(version);
  AL_DECL_ATTRIBUTE(repositoryPath);
  AL_DECL_ATTRIBUTE(assetName);

  // attribute to store the serialised layer (used for file IO only)
  AL_DECL_ATTRIBUTE(serialized);
  AL_DECL_ATTRIBUTE(nameOnLoad);
  AL_DECL_ATTRIBUTE(hasBeenEditTarget);

  /// \brief  return a pointer to the proxy shape that owns this node
  /// \return the proxy shape
  ProxyShape* getProxyShape() const
    { return m_shape; }

  // ----------------  API FOR UNIT TESTING ONLY -------------------------
  /// \brief  Internal function for unit tests only
  inline void testing_clearHandle()
    { m_handle = SdfLayerRefPtr(); }
  // ---------------------------------------------------------------------

private:
  SdfLayerRefPtr m_handle; ///< reference to the USD layer
  ProxyShape* m_shape; ///< reference to the proxy shape

  //--------------------------------------------------------------------------------------------------------------------
  /// MPxNode overrides
  //--------------------------------------------------------------------------------------------------------------------

  void postConstructor() override;
  bool getInternalValueInContext(const MPlug& plug, MDataHandle& dataHandle, MDGContext& ctx) override;
  bool setInternalValueInContext(const MPlug& plug, const MDataHandle& dataHandle, MDGContext& ctx) override;


  /// \var    static MObject comment();
  /// \brief  access the comment attribute handle
  /// \return the handle to the comment attribute

  /// \var    static MObject defaultPrim();
  /// \brief  access the defaultPrim attribute handle
  /// \return the handle to the defaultPrim attribute

  /// \var    static MObject documentation();
  /// \brief  access the documentation attribute handle
  /// \return the handle to the documentation attribute

  /// \var    static MObject startTime();
  /// \brief  access the startTime attribute handle
  /// \return the handle to the startTime attribute

  /// \var    static MObject endTime();
  /// \brief  access the endTime attribute handle
  /// \return the handle to the endTime attribute

  /// \var    static MObject timeCodesPerSecond();
  /// \brief  access the timeCodesPerSecond attribute handle
  /// \return the handle to the timeCodesPerSecond attribute

  /// \var    static MObject framePrecision();
  /// \brief  access the framePrecision attribute handle
  /// \return the handle to the framePrecision attribute

  /// \var    static MObject owner();
  /// \brief  access the owner attribute handle
  /// \return the handle to the owner attribute

  /// \var    static MObject sessionOwner();
  /// \brief  access the sessionOwner attribute handle
  /// \return the handle to the sessionOwner attribute

  /// \var    static MObject permissionToEdit();
  /// \brief  access the permissionToEdit attribute handle
  /// \return the handle to the permissionToEdit attribute

  /// \var    static MObject permissionToSave();
  /// \brief  access the permissionToSave attribute handle
  /// \return the handle to the permissionToSave attribute

  /// \var    static MObject proxyShape();
  /// \brief  access the proxyShape attribute handle
  /// \return the handle to the proxyShape attribute

  /// \var    static MObject subLayers();
  /// \brief  access the subLayers attribute handle
  /// \return the handle to the owner attribute

  /// \var    static MObject childLayers();
  /// \brief  access the childLayers attribute handle
  /// \return the handle to the childLayers attribute

  /// \var    static MObject parentLayer();
  /// \brief  access the parentLayer attribute handle
  /// \return the handle to the parentLayer attribute

  /// \var    static MObject displayName();
  /// \brief  access the displayName attribute handle
  /// \return the handle to the displayName attribute

  /// \var    static MObject realPath();
  /// \brief  access the realPath attribute handle
  /// \return the plug to the realPath attribute

  /// \var    static MObject fileExtension();
  /// \brief  access the fileExtension attribute handle
  /// \return the handle to the fileExtension attribute

  /// \var    static MObject version();
  /// \brief  access the version attribute handle
  /// \return the handle to the version attribute

  /// \var    static MObject repositoryPath();
  /// \brief  access the repositoryPath attribute handle
  /// \return the handle to the repositoryPath attribute

  /// \var    static MObject assetName();
  /// \brief  access the assetName attribute handle
  /// \return the handle to the assetName attribute

  /// \var    static MObject serialized();
  /// \brief  access the serialized attribute handle
  /// \return the handle to the serialized attribute

  /// \var    static MObject nameOnLoad();
  /// \brief  access the nameOnLoad attribute handle
  /// \return the handle to the nameOnLoad attribute

  /// \var    static MObject hasBeenEditTarget();
  /// \brief  access the owner attribute handle
  /// \return the handle to the hasBeenEditTarget attribute

  /// \var    MPlug commentPlug() const;
  /// \brief  access the comment attribute plug on this node instance.
  /// \return the plug to the comment attribute

  /// \var    MPlug defaultPrimPlug() const;
  /// \brief  access the defaultPrim attribute plug on this node instance.
  /// \return the plug to the defaultPrim attribute

  /// \var    MPlug documentationPlug() const;
  /// \brief  access the documentation attribute plug on this node instance.
  /// \return the plug to the documentation attribute

  /// \var    MPlug startTimePlug() const;
  /// \brief  access the startTime attribute plug on this node instance.
  /// \return the plug to the startTime attribute

  /// \var    MPlug endTimePlug() const;
  /// \brief  access the endTime attribute plug on this node instance.
  /// \return the plug to the endTime attribute

  /// \var    MPlug timeCodesPerSecondPlug() const;
  /// \brief  access the timeCodesPerSecond attribute plug on this node instance.
  /// \return the plug to the timeCodesPerSecond attribute

  /// \var    MPlug framePrecisionPlug() const;
  /// \brief  access the framePrecision attribute plug on this node instance.
  /// \return the plug to the framePrecision attribute

  /// \var    MPlug ownerPlug() const;
  /// \brief  access the owner attribute plug on this node instance.
  /// \return the plug to the owner attribute

  /// \var    MPlug sessionOwnerPlug() const;
  /// \brief  access the sessionOwner attribute plug on this node instance.
  /// \return the plug to the sessionOwner attribute

  /// \var    MPlug permissionToEditPlug() const;
  /// \brief  access the permissionToEdit attribute plug on this node instance.
  /// \return the plug to the permissionToEdit attribute

  /// \var    MPlug permissionToSavePlug() const;
  /// \brief  access the permissionToSave attribute plug on this node instance.
  /// \return the plug to the permissionToSave attribute

  /// \var    MPlug proxyShapePlug() const;
  /// \brief  access the proxyShape attribute plug on this node instance.
  /// \return the plug to the proxyShape attribute

  /// \var    MPlug subLayersPlug() const;
  /// \brief  access the subLayers attribute plug on this node instance.
  /// \return the plug to the owner attribute

  /// \var    MPlug childLayersPlug() const;
  /// \brief  access the childLayers attribute plug on this node instance.
  /// \return the plug to the childLayers attribute

  /// \var    MPlug parentLayerPlug() const;
  /// \brief  access the parentLayer attribute plug on this node instance.
  /// \return the plug to the parentLayer attribute

  /// \var    MPlug displayNamePlug() const;
  /// \brief  access the displayName attribute plug on this node instance.
  /// \return the plug to the displayName attribute

  /// \var    MPlug realPathPlug() const;
  /// \brief  access the realPath attribute plug on this node instance.
  /// \return the plug to the realPath attribute

  /// \var    MPlug fileExtensionPlug() const;
  /// \brief  access the fileExtension attribute plug on this node instance.
  /// \return the plug to the fileExtension attribute

  /// \var    MPlug versionPlug() const;
  /// \brief  access the version attribute plug on this node instance.
  /// \return the plug to the version attribute

  /// \var    MPlug repositoryPathPlug() const;
  /// \brief  access the repositoryPath attribute plug on this node instance.
  /// \return the plug to the repositoryPath attribute

  /// \var    MPlug assetNamePlug() const;
  /// \brief  access the assetName attribute plug on this node instance.
  /// \return the plug to the assetName attribute

  /// \var    MPlug serializedPlug() const;
  /// \brief  access the serialized attribute plug on this node instance.
  /// \return the plug to the serialized attribute

  /// \var    MPlug nameOnLoadPlug() const;
  /// \brief  access the nameOnLoad attribute plug on this node instance.
  /// \return the plug to the nameOnLoad attribute

  /// \var    MPlug hasBeenEditTargetPlug() const;
  /// \brief  access the owner attribute plug on this node instance.
  /// \return the plug to the hasBeenEditTarget attribute
};

//----------------------------------------------------------------------------------------------------------------------
} // nodes
} // usdmaya
} // AL
//----------------------------------------------------------------------------------------------------------------------

