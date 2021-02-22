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

#include <pxr/usd/usd/stage.h>

#include <maya/MPxNode.h>

#include <map>
#include <set>
#include <shared_mutex>

PXR_NAMESPACE_USING_DIRECTIVE

namespace AL {
namespace usdmaya {
namespace nodes {

class ProxyShape;

//----------------------------------------------------------------------------------------------------------------------
/// \brief  Iterator wrapper for LayerToIdsMap that hides non-dirty items
///         Implemented as a template to define const / non-const iterator at same time
/// \ingroup nodes
//----------------------------------------------------------------------------------------------------------------------
template <typename WrappedIterator> class DirtyOnlyIterator
//    : public std::iterator<std::forward_iterator_tag,
//                           typename WrappedIterator::value_type>
{
public:
    typedef typename WrappedIterator::value_type value_type;

    /// \brief  ctor
    /// \param  it the start of the iteration range
    /// \param  end the end of the iteration range
    DirtyOnlyIterator(WrappedIterator it, WrappedIterator end)
        : m_iter(it)
        , m_end(end)
    {
        SetToNextDirty();
    }

    /// \brief  copy ctor
    /// \param  other the iterator to copy
    DirtyOnlyIterator(const DirtyOnlyIterator& other)
        : m_iter(other.m_iter)
        , m_end(other.m_end)
    {
        SetToNextDirty();
    }

    /// \brief  pre-increment operator
    /// \return a reference to this
    DirtyOnlyIterator& operator++()
    {
        ++m_iter;
        SetToNextDirty();
        return *this;
    }

    /// \brief  post-increment operator
    /// \return a copy of the original iterator value
    DirtyOnlyIterator operator++(int)
    {
        WrappedIterator tmp(*this);
                        operator++();
        return tmp;
    }

    /// \brief  equivalence operator
    /// \param  rhs the iterator to compare against
    /// \return true if equivalent
    bool operator==(const DirtyOnlyIterator& rhs) const
    {
        // You could argue we should check m_end too,
        // but all we really care about is whether we're pointed
        // at the same place, and it's faster...
        return m_iter == rhs.m_iter;
    }

    /// \brief  non equivalence operator
    /// \param  rhs the iterator to compare against
    /// \return false if equivalent, true otherwise
    bool operator!=(const DirtyOnlyIterator& rhs) const { return m_iter != rhs.m_iter; }

    /// \brief  dereference operator
    value_type& operator*() { return *m_iter; }

private:
    void SetToNextDirty()
    {
        while (m_iter != m_end && !m_iter->first->IsDirty()) {
            ++m_iter;
        }
    }

    WrappedIterator m_iter;
    WrappedIterator m_end;
};

//----------------------------------------------------------------------------------------------------------------------
/// \brief  Stores layers, in a way that they may be looked up by the layer ref ptr, or by
/// identifier
///         Also, unlike boost::multi_index, we can have multiple identifiers per layer
///         You can add non-dirty layers to the database, but the query operations will "hide" them
///         - ie, iteration will skip by them, and findLayer will return an invalid ptr if it's not
///         dirty We allow adding non-dirty items because if we want to guarantee we always have all
///         the latest items, we need to deal with the situation where the current edit target
///         starts out not dirty... and it's easiest to just add it then filter it if it's not dirty
/// \ingroup nodes
//----------------------------------------------------------------------------------------------------------------------
class LayerDatabase
{
public:
    typedef std::map<SdfLayerRefPtr, std::vector<std::string>> LayerToIdsMap;
    typedef std::map<std::string, SdfLayerRefPtr>              IdToLayerMap;

    /// \brief  Add the given layer to the set of layers in this LayerDatabase, if not already
    /// present,
    ///         and optionally add an extra identifier as a key to it
    /// \param  layer What layer to add to this database
    /// \param  identifier Extra identifier to add as a key to this layer; note that the "canonical"
    /// identifier,
    ///         as returned by layer.GetIdentifier(), is ALWAYS added as an identifier key for this
    ///         layer so this is intended as a way to provide a second identifier for the same layer
    ///         (or third or more, if you call it repeatedly). This is useful both because multiple
    ///         identifiers may resolve to the same underlying layer (especially when considering
    ///         asset resolution), and for serializing and deserializing anonymous layers, the
    ///         "canonical" identifier will change every time it is serialized and deserialized (and
    ///         it can be necessary to refer to the layer both by it's "old" and "new" ids). If this
    ///         is an empty string, it is ignored.
    /// \return bool which is true if the layer was actually added to the set of layers managed by
    /// this node
    ///         (ie, if it wasn't already managed)
    bool addLayer(SdfLayerRefPtr layer, const std::string& identifier = std::string());

    /// \brief  Remove the given layer to the list of layers managed by this node, if present.
    /// \return bool which is true if the layer was actually removed from the set of layers managed
    /// by this node
    ///         (ie, if was previously managed)
    bool removeLayer(SdfLayerRefPtr layer);

    /// \brief  Find the layer in the set of layers managed by this node, by identifier
    /// \param  identifier the identifier the full identifier of the layer to locate
    /// \return The found layer handle in the layer list managed by this node (invalid if not found
    /// or not dirty)
    SdfLayerHandle findLayer(std::string identifier) const;

    /// Because we may have an unknown number of non-dirty member layers which we're treating
    /// as not-existing, we can't get a size without iterating over all the layers; we can,
    /// however, do an empty/non-empty boolean check by seeing if begin() == end(); in the
    /// worst case, when the LayerDatabase consists of nothing but non-dirty layers, begin()
    /// will will still end up iterating through all the layers attempting to find a
    /// non-dirty layer to start at, but the average case should be pretty fast
    ///
    /// We use the safe-bool idiom to avoid nasty automatic conversions, etc
private:
    typedef const LayerToIdsMap LayerDatabase::*_UnspecifiedBoolType;

public:
    operator _UnspecifiedBoolType() const
    {
        return begin() == end() ? &LayerDatabase::m_layerToIds : nullptr;
    }

    /// \brief  Upper bound for the number of non-dirty layers in this object
    ///         This is the count of all tracked layers, dirty-and-non-dirty;
    ///         If it is zero, it can be guaranteed that there are no dirty
    ///         layers, but if it is non-zero, we cannot guarantee that there
    ///         are any non-dirty layers. Use boolean conversion above to test
    ///         that.
    size_t max_size() const { return m_layerToIds.size(); }

    // Iterator interface - skips past non-dirty items
    typedef DirtyOnlyIterator<LayerToIdsMap::iterator>       iterator;
    typedef DirtyOnlyIterator<LayerToIdsMap::const_iterator> const_iterator;

    /// \brief  returns start of layer range
    iterator begin() { return iterator(m_layerToIds.begin(), m_layerToIds.end()); }

    /// \brief  returns start of layer range
    const_iterator begin() const
    {
        return const_iterator(m_layerToIds.cbegin(), m_layerToIds.cend());
    }

    /// \brief  returns start of layer range
    const_iterator cbegin() const
    {
        return const_iterator(m_layerToIds.cbegin(), m_layerToIds.cend());
    }

    /// \brief  returns end of layer range
    iterator end() { return iterator(m_layerToIds.end(), m_layerToIds.end()); }

    /// \brief  returns end of layer range
    const_iterator end() const { return const_iterator(m_layerToIds.cend(), m_layerToIds.cend()); }

    /// \brief  returns end of layer range
    const_iterator cend() const { return const_iterator(m_layerToIds.cend(), m_layerToIds.cend()); }

private:
    void _addLayer(
        SdfLayerRefPtr            layer,
        const std::string&        identifier,
        std::vector<std::string>& idsForLayer);

    LayerToIdsMap m_layerToIds;
    IdToLayerMap  m_idToLayer;
};

//----------------------------------------------------------------------------------------------------------------------
/// \brief  The layer manager node handles serialization and deserialization of all layers used by
/// all ProxyShapes
///         It may temporarily contain non-dirty layers, but those will be filtered out by query
///         operations.
/// \ingroup nodes
//----------------------------------------------------------------------------------------------------------------------
class LayerManager
    : public MPxNode
    , public AL::maya::utils::NodeHelper
{
public:
    /// \brief  ctor
    inline LayerManager()
        : MPxNode()
        , NodeHelper()
    {
    }

    /// \brief  Find the already-existing non-referenced LayerManager node in the scene, or return a
    /// null MObject \return the found LayerManager node, or a null MObject
    AL_USDMAYA_PUBLIC
    static MObject findNode();

    /// \brief  Either find the already-existing non-referenced LayerManager node in the scene, or
    /// make one \param dgmod An optional dgmodifier to create the node, if necessary. Note that if
    /// one is passed in,
    ///              createNode might be called on it, but doIt never will be, so the layer manager
    ///              node may not be added to the scene graph yet
    /// \param wasCreated If given, whether a new layer manager had to be created is stored here.
    /// \return the found-or-created LayerManager node
    AL_USDMAYA_PUBLIC
    static MObject findOrCreateNode(MDGModifier* dgmod = nullptr, bool* wasCreated = nullptr);

    /// \brief  Find the already-existing non-referenced LayerManager node in the scene, or return a
    /// nullptr \return the found LayerManager, or a nullptr
    AL_USDMAYA_PUBLIC
    static LayerManager* findManager();

    /// \brief  Either find the already-existing non-referenced LayerManager in the scene, or make
    /// one \param dgmod An optional dgmodifier to create the node, if necessary. Note that if one
    /// is passed in,
    ///              createNode might be called on it, but doIt never will be, so the layer manager
    ///              node may not be added to the scene graph yet
    /// \param wasCreated If given, whether a new layer manager had to be created is stored here.
    /// \return the found-or-created LayerManager
    AL_USDMAYA_PUBLIC
    static LayerManager*
    findOrCreateManager(MDGModifier* dgmod = nullptr, bool* wasCreated = nullptr);

    //--------------------------------------------------------------------------------------------------------------------
    /// Methods to handle the saving and restoring of layer data
    //--------------------------------------------------------------------------------------------------------------------

    /// \brief  Add the given layer to the list of layers managed by this node, if not already
    /// present. \param  layer What layer to add to this LayerManager \param  identifier Extra
    /// identifier to add as a key to this layer; note that the "canonical" identifier,
    ///         as returned by layer.GetIdentifier(), is ALWAYS added as an identifier key for this
    ///         layer so this is intended as a way to provide a second identifier for the same layer
    ///         (or third or more, if you call it repeatedly). This is useful both because multiple
    ///         identifiers may resolve to the same underlying layer (especially when considering
    ///         asset resolution), and for serializing and deserializing anonymous layers, the
    ///         "canonical" identifier will change every time it is serialized and deserialized (and
    ///         it can be necessary to refer to the layer both by it's "old" and "new" ids). If this
    ///         is an empty string, it is ignored.
    /// \return bool which is true if the layer was actually added to the list of layers managed by
    /// this node
    ///         (ie, if it wasn't already managed, and the given layer handle was valid)
    AL_USDMAYA_PUBLIC
    bool addLayer(SdfLayerHandle layer, const std::string& identifier = std::string());

    /// \brief  Remove the given layer to the list of layers managed by this node, if present.
    /// \return bool which is true if the layer was actually removed from the list of layers managed
    /// by this node
    ///         (ie, if it was previously managed, and the given layer handle was valid)
    AL_USDMAYA_PUBLIC
    bool removeLayer(SdfLayerHandle layer);

    /// \brief  Find the layer in the list of layers managed by this node, by identifier
    /// \return The found layer handle in the layer list managed by this node (invalid if not found)
    AL_USDMAYA_PUBLIC
    SdfLayerHandle findLayer(std::string identifier);

    /// \brief  Store a list of the managed layers' identifiers in the given MStringArray
    /// \param  outputNames The array to hold the identifier names; will be cleared before being
    /// filled.
    ///         No guarantees are made about the order in which the layer identifiers will be
    ///         returned.
    AL_USDMAYA_PUBLIC
    void getLayerIdentifiers(MStringArray& outputNames);

    /// \brief  Ensures that the layers attribute will be filled out with serialized versions of all
    /// tracked layers.
    AL_USDMAYA_PUBLIC
    MStatus populateSerialisationAttributes();

    /// \brief  Clears the layers attribute.
    AL_USDMAYA_PUBLIC
    MStatus clearSerialisationAttributes();

    /// \brief  For every serialized layer stored in attributes, loads them as sdf layers
    AL_USDMAYA_PUBLIC
    void loadAllLayers();

    //--------------------------------------------------------------------------------------------------------------------
    /// Type Info & Registration
    //--------------------------------------------------------------------------------------------------------------------
    AL_MAYA_DECLARE_NODE();

    /// \brief  Creates the node, but only if there is not a non-referenced one in the scene
    /// already.
    AL_USDMAYA_PUBLIC
    static void* conditionalCreator();

    //--------------------------------------------------------------------------------------------------------------------
    /// Type Info & Registration
    //--------------------------------------------------------------------------------------------------------------------

    // attributes to store the serialised layers (used for file IO only)

    // Note that the layers attribute should ONLY used during serialization, as this is the ONLY
    // times at which these attributes are guaranteed to "line up" to the internal layer register
    // (m_layerList).  Ie, immediately before save (due to the pre-save callback), the attributes
    // will be written from m_layerList; and immediate after open (due to the post-open callback),
    // m_layerList will be initialized from the attributes.  At all other times, the attributes will
    // be OUT OF SYNC (and, in fact, are intentionally set to be "empty", so there's no confusion /
    // someone doesn't try to use "out of date" information)
    AL_DECL_ATTRIBUTE(layers);
    // Not using AL_DECL_ATTRIBUTE for these, because we never want a generic, ie, identifierPlug()
    // - they only make sense for a particular index of the parent array-attribute... and it taking
    // up the "identifierPlug" name is confusing
    AL_DECL_MULTI_CHILD_ATTRIBUTE(identifier);
    AL_DECL_MULTI_CHILD_ATTRIBUTE(serialized);
    AL_DECL_MULTI_CHILD_ATTRIBUTE(anonymous);

private:
    static MObject _findNode();

    LayerDatabase m_layerDatabase;

    // Note on layerManager / multithreading:
    // I don't know that layerManager will be used in a multihreaded manenr... but I also don't know
    // it COULDN'T be. (I haven't really looked into the way maya's new multi-threaded node
    // evaluation works, for instance.) This is essentially a globally shared resource, so I figured
    // better be safe...
    std::shared_timed_mutex m_layersMutex;

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

} // namespace nodes
} // namespace usdmaya
} // namespace AL
//----------------------------------------------------------------------------------------------------------------------
