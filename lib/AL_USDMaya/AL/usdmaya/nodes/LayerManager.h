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

#include <boost/multi_index_container.hpp>
#include <boost/multi_index/random_access_index.hpp>
#include <boost/multi_index/hashed_index.hpp>

#include <map>
#include <set>
#include <boost/thread.hpp>

PXR_NAMESPACE_USING_DIRECTIVE

namespace AL {
namespace usdmaya {
namespace nodes {


//----------------------------------------------------------------------------------------------------------------------
/// \brief  The layer node stores a reference to an SdfLayer.
/// \ingroup nodes
//----------------------------------------------------------------------------------------------------------------------
class LayerManager
  : public MPxNode,
    public maya::NodeHelper
{
public:

  /// \brief  ctor
  inline LayerManager()
    : MPxNode(), NodeHelper() {}

  /// \brief  Find the already-existing non-referenced LayerManager node in the scene, or return a null MObject
  /// \return the found LayerManager node, or a null MObject
  static MObject findNode();

  /// \brief  Either find the already-existing non-referenced LayerManager node in the scene, or make one
  /// \return the found-or-created LayerManager node
  static MObject findOrCreateNode();

  /// \brief  Find the already-existing non-referenced LayerManager node in the scene, or return a nullptr
  /// \return the found LayerManager, or a nullptr
  static LayerManager* findManager();

  /// \brief  Either find the already-existing non-referenced LayerManager in the scene, or make one
  /// \return the found-or-created LayerManager
  static LayerManager* findOrCreateManager();

  //--------------------------------------------------------------------------------------------------------------------
  /// Methods to handle the saving and restoring of layer data
  //--------------------------------------------------------------------------------------------------------------------

  /// \brief  Add the given layer to the list of layers managed by this node, if not already present.
  /// \return bool which is true if the layer was actually added to the list of layers managed by this node
  ///         (ie, if it wasn't already managed)
  bool addLayer(SdfLayerRefPtr layer);

  /// \brief  Remove the given layer to the list of layers managed by this node, if present.
  /// \return bool which is true if the layer was actually removed from the list of layers managed by this node (ie, if
  //          was previously managed)
  bool removeLayer(SdfLayerRefPtr layer);

  /// \brief  Find the layer in the list of layers managed by this node, by identifier
  /// \return The found layer handle in the layer list managed by this node (invalid if not found)
  SdfLayerHandle findLayer(std::string identifier);

  /// \brief  Store a list of the managed layers' identifiers in the given MStringArray
  /// \param  outputNames The array to hold the identifier names; will be cleared before being filled
  void getLayerIdentifiers(MStringArray& outputNames);

  /// \brief  Ensures that the layers attribute will be filled out with serialized versions of all tracked layers.
  MStatus populateSerialisationAttributes();

  /// \brief  Clears the layers attribute.
  MStatus clearSerialisationAttributes();

  /// \brief  For every serialized layer stored in attributes, loads them as sdf layers
  void loadAllLayers();

  //--------------------------------------------------------------------------------------------------------------------
  /// Type Info & Registration
  //--------------------------------------------------------------------------------------------------------------------
  AL_MAYA_DECLARE_NODE();

  /// \brief  Creates the node, but only if there is not a non-referenced one in the scene already.
  static void* conditionalCreator();

  //--------------------------------------------------------------------------------------------------------------------
  /// Type Info & Registration
  //--------------------------------------------------------------------------------------------------------------------

  // attributes to store the serialised layers (used for file IO only)

  // Note that the layers attribute should ONLY used during serialization, as this is the ONLY
  // times at which these attributes are guaranteed to "line up" to the internal layer register
  // (m_layerList).  Ie, immediately before save (due to the pre-save callback), the attributes will
  // be written from m_layerList; and immediate after open (due to the post-open callback), m_layerList
  // will be initialized from the attributes.  At all other times, the attributes will be OUT OF SYNC
  // (and, in fact, are intentionally set to be "empty", so there's no confusion / someone doesn't
  // try to use "out of date" information)
  AL_DECL_ATTRIBUTE(layers);
  // Not using AL_DECL_ATTRIBUTE for these, because we never want a generic, ie, identifierPlug() -
  // they only make sense for a particular index of the parent array-attribute... and it taking up
  // the "identifierPlug" name is confusing
  AL_DECL_MULTI_CHILD_ATTRIBUTE(identifier);
  AL_DECL_MULTI_CHILD_ATTRIBUTE(serialized);
  AL_DECL_MULTI_CHILD_ATTRIBUTE(anonymous);

private:
  static MObject _findNode();

  // This code modified slightly from sdf/layerRegistry.h

  // Index tags.
  struct by_position {};
  struct by_layer {};
  struct by_identifier {};

  // Key Extractors.
  struct layer_identifier {
      typedef std::string result_type;
      const result_type& operator()(const SdfLayerRefPtr& layer) const;
  };

  // List of layers, with indices by layer and identifier
  typedef boost::multi_index::multi_index_container<
      SdfLayerRefPtr,
      boost::multi_index::indexed_by<
        // Layer<->Layer, one-to-one. Duplicate layer handles cannot be
        // inserted into the container.
        boost::multi_index::hashed_unique<
          boost::multi_index::tag<by_layer>,
          boost::multi_index::identity<SdfLayerRefPtr>,
          TfHash
          >,

        // Layer<->Identifier, one-to-many. The identifier is the path
        // passed in to CreateNew/FindOrOpen, and may be any path form
        // resolvable to a single real path.
        boost::multi_index::hashed_non_unique<
          boost::multi_index::tag<by_identifier>,
          layer_identifier
          >
      >
  > _Layers;

  // Layer index.
  typedef _Layers::index<by_layer>::type _LayersByLayer;
  // Identifier index.
  typedef _Layers::index<by_identifier>::type _LayersByIdentifier;

  _Layers m_layerList;
  boost::shared_mutex m_layersMutex;

  //--------------------------------------------------------------------------------------------------------------------
  /// MPxNode overrides
  //--------------------------------------------------------------------------------------------------------------------

  /// \var    static MObject layers();
  /// \brief  access the layers attribute handle
  /// \return the handle to the layers attribute

  /// \var    static MObject serialized();
  /// \brief  access the serialized attribute handle
  /// \return the handle to the serialized attribute

  /// \var    static MObject identifier();
  /// \brief  access the identifier attribute handle
  /// \return the handle to the identifier attribute

  /// \var    static MObject anonymous();
  /// \brief  access the anonymous attribute handle
  /// \return the handle to the anonymous attribute

  /// \var    MPlug layersPlug() const;
  /// \brief  access the serialized layers plug on this node instance.
  /// \return the plug to the layers attribute

  /// \var    MPlug serializedPlug() const;
  /// \brief  access the serialized attribute plug on this node instance.
  /// \return the plug to the serialized attribute

  /// \var    MPlug identifier() const;
  /// \brief  access the identifier attribute plug on this node instance.
  /// \return the plug to the identifier attribute

  /// \var    MPlug anonymous() const;
  /// \brief  access the anonymous attribute plug on this node instance.
  /// \return the plug to the anonymous attribute

};

} // nodes
} // usdmaya
} // AL
//----------------------------------------------------------------------------------------------------------------------

