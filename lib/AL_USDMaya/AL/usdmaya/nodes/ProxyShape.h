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
#include "AL/usdmaya/DrivenTransformsData.h"
#include "AL/usdmaya/fileio/translators/TranslatorBase.h"
#include "AL/usdmaya/fileio/translators/TranslatorContext.h"
#include "AL/usdmaya/fileio/translators/TransformTranslator.h"
#include "AL/usdmaya/nodes/proxy/PrimFilter.h"
#include "AL/usdmaya/SelectableDB.h"

#include "maya/MPxSurfaceShape.h"
#include "maya/MEventMessage.h"
#include "maya/MNodeMessage.h"
#include "maya/MPxDrawOverride.h"
#include "maya/MEvaluationNode.h"
#include "maya/MDagModifier.h"
#include "maya/MSelectionList.h"
#include "pxr/pxr.h"
#include "pxr/usd/usd/prim.h"
#include "pxr/usd/usd/timeCode.h"
#include "pxr/usd/sdf/path.h"
#include "pxr/base/tf/weakBase.h"
#include "pxr/usd/usd/notice.h"
#include "pxr/usd/sdf/notice.h"
#include <stack>
#include <functional>

PXR_NAMESPACE_USING_DIRECTIVE

PXR_NAMESPACE_OPEN_SCOPE

class UsdImagingGLHdEngine;

PXR_NAMESPACE_CLOSE_SCOPE;

namespace AL {
namespace usdmaya {
namespace nodes {

//----------------------------------------------------------------------------------------------------------------------
/// \brief  A helper class to store the state that is modified during a change to the current selection within a
///         proxy shape. The state it maintains includes:
///
///         \li The USD paths to be selected / deselected
///         \li The Dag modifiers needed to create/destroy the associated maya nodes
///         \li The maya selection list prior to the selection
///         \li The maya selection list after the selection change
///         \li The internal transformation references before and after the selection change
///
///         This class is intended to exist as a member variable on a MEL selection command. Once constructed, the class
///         should be passed to the ProxyShape::doSelect method to construct the internal state changes. At that point,
///         you may call doIt() to perform the changes, undoIt() to revert the changes.
//----------------------------------------------------------------------------------------------------------------------
struct SelectionUndoHelper
{
  /// \brief  Construct with the arguments to select / deselect nodes on a proxy shape
  /// \param  proxy pointer to the maya node on which the selection operation will be performed.
  /// \param  paths the USD paths to be selected / toggled / unselected
  /// \param  mode the selection mode (add, remove, xor, etc)
  /// \param  internal if the internal flag is set, then modifications to Maya's selection list will NOT occur.
  SelectionUndoHelper(nodes::ProxyShape* proxy, SdfPathVector paths, MGlobal::ListAdjustment mode, bool internal = false);

  /// \brief  performs the selection changes
  void doIt();

  /// \brief  will undo the selection changes
  void undoIt();

private:
  friend class ProxyShape;
  nodes::ProxyShape* m_proxy;
  SdfPathVector m_paths;
  SdfPathVector m_previousPaths;
  MGlobal::ListAdjustment m_mode;
  MDagModifier m_modifier1;
  MDagModifier m_modifier2;
  MSelectionList m_previousSelection;
  MSelectionList m_newSelection;
  std::vector<std::pair<SdfPath, MObject>> m_insertedRefs;
  std::vector<std::pair<SdfPath, MObject>> m_removedRefs;
  bool m_internal;
};

//----------------------------------------------------------------------------------------------------------------------
/// \brief  Used as a way to construct a simple selection list that allows for selection highlighting without
///         creating/destroying transforms.
//----------------------------------------------------------------------------------------------------------------------
class SelectionList
{
public:

  /// \brief  default ctor
  SelectionList() = default;

  /// \brief  copy ctor
  /// \param  sl the selection list to copy
  SelectionList(const SelectionList& sl) = default;

  /// \brief  dtor
  ~SelectionList() = default;

  /// \brief  clear the selection list
  inline void clear()
    { m_selected.clear(); }

  /// \brief  adds a path to the selection
  /// \param  path to add
  inline void add(SdfPath path)
    {
      if(std::find(m_selected.begin(), m_selected.end(), path) == m_selected.end())
      {
        m_selected.push_back(path);
      }
    }

  /// \brief  removes the path from the selection
  /// \param  path to remove
  inline void remove(SdfPath path)
    {
      auto it = std::find(m_selected.begin(), m_selected.end(), path);
      if(it != m_selected.end())
      {
        m_selected.erase(it);
      }
    }

  /// \brief  toggles the path in the selection
  /// \param  path to toggle
  inline void toggle(SdfPath path)
    {
      auto it = std::find(m_selected.begin(), m_selected.end(), path);
      if(it == m_selected.end())
      {
        m_selected.push_back(path);
      }
      else
      {
        m_selected.erase(it);
      }
    }

  /// \brief  toggles the path in the selection
  /// \param  path to toggle
  inline bool isSelected(const SdfPath& path) const
    { return std::find(m_selected.begin(), m_selected.end(), path) != m_selected.end(); }

  /// \brief  the paths in the selection list
  /// \return the selected paths
  inline const SdfPathVector& paths() const
    { return m_selected; }

  /// \brief  the paths in the selection list
  /// \return the selected paths
  inline size_t size() const
    { return m_selected.size(); }

private:
  SdfPathVector m_selected;
};

//typedef functions
struct  HierarchyIterationLogic
{
  HierarchyIterationLogic():
      preIteration(nullptr),
      iteration(nullptr),
      postIteration(nullptr)
  {}

  std::function<void()> preIteration;
  std::function<void(const fileio::TransformIterator& transformIterator,const UsdPrim& prim)> iteration;
  std::function<void()> postIteration;
};

struct FindSelectablePrimsLogic : public HierarchyIterationLogic
{
  SdfPathVector newSelectables;
  SdfPathVector removeSelectables;
};


typedef const HierarchyIterationLogic*  HierarchyIterationLogics[2];


//----------------------------------------------------------------------------------------------------------------------
/// \brief  A custom proxy shape node that attaches itself to a USD file, and then renders it.
///         The stage is held internally as a member variable, and it will be composed based on a change to the
///         "filePath" attribute.
/// \ingroup nodes
//----------------------------------------------------------------------------------------------------------------------
class ProxyShape
  : public MPxSurfaceShape,
    public maya::NodeHelper,
    public proxy::PrimFilterInterface,
    public TfWeakBase
{
  friend class SelectionUndoHelper;
  friend class ProxyShapeUI;
public:

  /// \brief  a mapping between a maya transform (or MObject::kNullObj), and the prim that exists at that location
  ///         in the DAG graph.
  typedef std::vector<std::pair<MObject, UsdPrim> > MObjectToPrim;

  /// \brief  ctor
  ProxyShape();

  /// \brief  dtor
  ~ProxyShape();

  //--------------------------------------------------------------------------------------------------------------------
  /// \name   Type Info & Registration
  //--------------------------------------------------------------------------------------------------------------------
  AL_MAYA_DECLARE_NODE();

  //--------------------------------------------------------------------------------------------------------------------
  /// \name   Layers API
  //--------------------------------------------------------------------------------------------------------------------

  /// \brief  Locate the maya node associated with the specified layer
  /// \param  handle the usd layer to locate
  /// \return a pointer to the maya node that references the layer, or NULL if the layer was not found
  Layer* findLayer(SdfLayerHandle handle);

  /// \brief  Locate the name of the maya node associated with the specified layer
  /// \param  handle the usd layer name to locate
  /// \return the name of the maya node that references the layer, or and empty string if the layer was not found
  MString findLayerMayaName(SdfLayerHandle handle);

  /// \brief  return the node that represents the root layer
  /// \return the root layer, or NULL if stage is invalid
  Layer* getLayer();

  //--------------------------------------------------------------------------------------------------------------------
  /// \name   Input Attributes
  //--------------------------------------------------------------------------------------------------------------------

  /// the input USD file path for this proxy
  AL_DECL_ATTRIBUTE(filePath);

  /// a path to a prim you want to view with this shape
  AL_DECL_ATTRIBUTE(primPath);

  /// a comma seperated list of prims you *don't* want to see.
  AL_DECL_ATTRIBUTE(excludePrimPaths);

  /// the input time value (probably connected to time1.outTime)
  AL_DECL_ATTRIBUTE(time);

  /// an offset, in GUI time units, where the animation should start playback
  AL_DECL_ATTRIBUTE(timeOffset);

  /// a scalar value that can speed up (values greater than 1) or slow down (values less than one) or reverse (negative
  /// values) the playback of the animation.
  AL_DECL_ATTRIBUTE(timeScalar);

  /// the subdiv complexity used
  AL_DECL_ATTRIBUTE(complexity);

  /// display guide - sets shape to display geometry of purpose "guide". See <a href="https://github.com/PixarAnimationStudios/USD/blob/95eef7c9a6662a5362dfc312a186f50c58e27ecd/pxr/usd/lib/usdGeom/imageable.h#L165">imageable.h</a>
  AL_DECL_ATTRIBUTE(displayGuides);

  /// display render guide - sets hape to display geometry of purpose "render. See <a href="https://github.com/PixarAnimationStudios/USD/blob/95eef7c9a6662a5362dfc312a186f50c58e27ecd/pxr/usd/lib/usdGeom/imageable.h#L165">imageable.h</a>
  AL_DECL_ATTRIBUTE(displayRenderGuides);

  /// Connection to any layer DG nodes
  AL_DECL_ATTRIBUTE(layers);

  /// serialised session layer
  // TODO reset if the usd file path is updated via the ui
  AL_DECL_ATTRIBUTE(serializedSessionLayer);

  /// serialised asset resolver context
  // @note currently not used
  AL_DECL_ATTRIBUTE(serializedArCtx);

  /// serialised translator context
  AL_DECL_ATTRIBUTE(serializedTrCtx);

  /// Open the stage unloaded.
  AL_DECL_ATTRIBUTE(unloaded);

  /// an array of MPxData for the driven transforms
  AL_DECL_ATTRIBUTE(inDrivenTransformsData);

  /// ambient display colour
  AL_DECL_ATTRIBUTE(ambient);

  /// diffuse display colour
  AL_DECL_ATTRIBUTE(diffuse);

  /// specular display colour
  AL_DECL_ATTRIBUTE(specular);

  /// emission display colour
  AL_DECL_ATTRIBUTE(emission);

  /// material shininess
  AL_DECL_ATTRIBUTE(shininess);

  /// Serialised reference counts to rebuild the transform reference information
  AL_DECL_ATTRIBUTE(serializedRefCounts);

  /// The path list joined by ",", that will be used as a mask when doing UsdStage::OpenMask()
  AL_DECL_ATTRIBUTE(populationMaskIncludePaths);

  /// Version of the plugin at the time of creation (read-only)
  AL_DECL_ATTRIBUTE(version);

  //--------------------------------------------------------------------------------------------------------------------
  /// \name   Output Attributes
  //--------------------------------------------------------------------------------------------------------------------

  /// outTime = (time - timeOffset) * timeScalar
  AL_DECL_ATTRIBUTE(outTime);

  /// inStageData  --->  inStageDataCached  --->  outStageData
  AL_DECL_ATTRIBUTE(outStageData);


  //--------------------------------------------------------------------------------------------------------------------
  /// \name   Public Utils
  //--------------------------------------------------------------------------------------------------------------------

  /// \brief  provides access to the UsdStage that this proxy shape is currently representing
  /// \return the proxy shape
  UsdStageRefPtr getUsdStage() const;

  /// \brief  gets hold of the attributes on this node that control the rendering in some way
  /// \param  attribs the returned set of render attributes (should be of type: UsdImagingGLEngine::RenderParams*. Hiding
  ///         this behind a void pointer to prevent UsdImagingGLEngine leaking into the translator plugin dependencies)
  /// \param  frameContext the frame context for rendering
  /// \param  dagPath the dag path of the node being rendered
  /// \return true if the attribs could be retrieved (i.e. is the stage is valid)
  bool getRenderAttris(void* attribs, const MHWRender::MFrameContext& frameContext, const MDagPath& dagPath);

  /// \brief  compute bounds
  MBoundingBox boundingBox() const override;

  //--------------------------------------------------------------------------------------------------------------------
  /// \name   AL_usdmaya_Transform utils
  /// \brief  A set of commands to manipulate the chains of transforms that map to the usd prims found in a stage.
  ///         There are 3 main reasons for a transform node to exist within maya:
  ///         \li They are required to provide a parent transform for a schema prim that will be imported into maya.
  ///         \li They are the parent node of an object that has been selected
  ///         \li The user has requested that they be imported
  //--------------------------------------------------------------------------------------------------------------------

  /// an enum describing the reason that a transform exists in the scene
  enum TransformReason
  {
    kSelection = 1 << 0,  ///< the node exists for selection
    kRequested = 1 << 1,  ///< the node has been requested by a user
    kRequired = 1 << 2    ///< the node is required for an imported schema prim
  };

  /// \brief  returns true if the path is required for an imported schema prim
  /// \param  path the path to query
  /// \return true if the path represents a prim that is required.
  inline bool isRequiredPath(const SdfPath& path) const
    { return m_requiredPaths.find(path) != m_requiredPaths.end(); }

  /// \brief  returns the MObject of the maya transform for requested path (or MObject::kNullObj)
  /// \param  path the usd prim path to look up
  /// \return the MObject for the parent transform to the path specified
  inline MObject findRequiredPath(const SdfPath& path) const
    {
      const auto it = m_requiredPaths.find(path);
      if(it != m_requiredPaths.end())
      {
        return it->second.node();
      }
      return MObject::kNullObj;
    }

  /// \brief  traverses the UsdStage looking for the prims that are going to be handled by custom transformer
  ///         plug-ins.
  /// \param  proxyTransformPath the DAG path of the proxy shape
  /// \param  startPath the path from which iteration needs to start in the UsdStage
  /// \param  manufacture the translator registry
  /// \return the array of prims found that will need to be imported
  std::vector<UsdPrim> huntForNativeNodesUnderPrim(
      const MDagPath& proxyTransformPath,
      SdfPath startPath,
      fileio::translators::TranslatorManufacture& manufacture);

  /// \brief  constructs a single chain of transform nodes from the usdPrim to the root of this proxy shape.
  /// \param  usdPrim  the leaf of the prim we wish to create
  /// \param  modifier will store the changes as this path is constructed.
  /// \param  reason  the reason why this path is being generated.
  /// \param  modifier2 is specified, the modifier will end up containing a set of commands to switch the pushToPrim
  ///         flags to true. (This has to be done after the transform has been created and initialized, otherwise
  ///         the default maya values will be pushed in the UsdPrim, wiping out the values you just loaded)
  /// \param  createCount the returned number of transforms that were created.
  /// \return the MObject of the parent transform node for the usdPrim
  /// \todo   The mode ProxyShape::kSelection will cause the possibility of instability in the selection system.
  ///         This mode will be removed at a future date
  MObject makeUsdTransformChain(
      const UsdPrim& usdPrim,
      MDagModifier& modifier,
      TransformReason reason,
      MDGModifier* modifier2 = 0,
      uint32_t* createCount = 0);

  /// \brief  Will construct AL_usdmaya_Transform nodes for all of the prims from the specified usdPrim and down.
  /// \param  usdPrim the root for the transforms to be created
  /// \param  modifier the modifier that will store the creation steps for the transforms
  /// \param  reason the reason for creating the transforms (use with selection, etc).
  /// \param  modifier2 if specified, this will contain a set of commands that turn on the pushToPrim flag on the transform
  ///         nodes. These flags need to be set after the transforms have been created
  /// \todo   The mode ProxyShape::kSelection will cause the possibility of instability in the selection system.
  ///         This mode will be removed at a future date
  MObject makeUsdTransforms(
      const UsdPrim& usdPrim,
      MDagModifier& modifier,
      TransformReason reason,
      MDGModifier* modifier2 = 0);

  /// \brief  will destroy all of the AL_usdmaya_Transform nodes from the prim specified, up to the root (unless any
  ///         of those transform nodes are in use by another imported prim).
  /// \param  usdPrim the leaf node in the chain of transforms we wish to remove
  /// \param  modifier will store the changes as this path is constructed.
  /// \param  reason  the reason why this path is being generated.
  /// \todo   The mode ProxyShape::kSelection will cause the possibility of instability in the selection system.
  ///         This mode will be removed at a future date
  void removeUsdTransformChain(
      const UsdPrim& usdPrim,
      MDagModifier& modifier,
      TransformReason reason);

  /// \brief  will destroy all of the AL_usdmaya_Transform nodes from the prim specified, up to the root (unless any
  ///         of those transform nodes are in use by another imported prim).
  /// \param  path the leaf node in the chain of transforms we wish to remove
  /// \param  modifier will store the changes as this path is constructed.
  /// \param  reason  the reason why this path is being generated.
  void removeUsdTransformChain(
      const SdfPath& path,
      MDagModifier& modifier,
      TransformReason reason);

  /// \brief  Will destroy all AL_usdmaya_Transform nodes found underneath the prim (unless those nodes are required
  ///         for another purpose).
  /// \param  usdPrim
  /// \param  modifier the modifier that will be filled with the instructions to delete the transform nodes
  /// \param  reason Are we deleting selected transforms, or those that are required, or requested?
  /// \todo   The mode ProxyShape::kSelection will cause the possibility of instability in the selection system.
  ///         This mode will be removed at a future date
  void removeUsdTransforms(
      const UsdPrim& usdPrim,
      MDagModifier& modifier,
      TransformReason reason);

  /// \brief  Debugging util - prints out the reference counts for each AL_usdmaya_Transform that currently exists
  ///         in the scene
  void printRefCounts() const;

  /// \brief  destroys all internal transform references
  void destroyTransformReferences()
    { m_requiredPaths.clear(); }

  /// \brief  Internal method. Used to filter out a set of paths into groups that need to be created, deleted, or updating.
  /// \param  previousPrims the previous list of prims underneath a prim in the process of a variant change
  /// \param  newPrimSet the list of prims found under neath the prim after the variant change. Prims that can be
  ///         updated will be removed from this list, leaving only the prims that need to be created.
  /// \param  transformsToCreate of the new nodes that can be created, if they are a DAG node (i.e. require a transform),
  ///         then this list will contain those nodes
  /// \param  updatablePrimSet a returned list of prims that can be updated
  /// \param  removedPrimSet a returned set of paths for objects that need to be deleted.
  void filterPrims(
      const SdfPathVector& previousPrims,
      std::vector<UsdPrim>& newPrimSet,
      std::vector<UsdPrim>& transformsToCreate,
      std::vector<UsdPrim>& updatablePrimSet,
      SdfPathVector& removedPrimSet);

  /// \brief  a method that is used within testing only. Returns the current reference count state for the path
  /// \param  path the prim path to test
  /// \param  selected the returned selected reference count
  /// \param  required the returned required reference count
  /// \param  refCount the returned refCount reference count
  void getCounts(SdfPath path, uint32_t& selected, uint32_t& required, uint32_t& refCount)
  {
    auto it = m_requiredPaths.find(path);
    if(it != m_requiredPaths.end())
    {
      selected = it->second.selected();
      required = it->second.required();
      refCount = it->second.refCount();
    }
  }

  /// \brief  Tests to see if a given MObject is currently selected in the proxy shape. If the specified MObject is
  ///         selected, then the path will be filled with the corresponding usd prim path.
  /// \param  obj the input MObject to see if it's selected.
  /// \param  path the returned prim path (if the node is found)
  /// \return true if the maya node is currently selected
  bool isSelectedMObject(MObject obj, SdfPath& path)
  {
    for(auto it : m_requiredPaths)
    {
      if(obj == it.second.node())
      {
        auto iter = std::find(m_selectedPaths.cbegin(), m_selectedPaths.cend(), it.first);
        if(iter != m_selectedPaths.cend())
        {
          return true;
        }
        path = it.first;
        break;
      }
    }
    return false;
  }

  //--------------------------------------------------------------------------------------------------------------------
  /// \name   Plug-in Translator node methods
  //--------------------------------------------------------------------------------------------------------------------

  /// \brief  serialises the translator context
  void serialiseTranslatorContext();

  /// \brief  deserialises the translator context
  void deserialiseTranslatorContext();

  /// \brief aggregates logic that needs to iterate through the hierarchy looking for properties/metdata on prims
  void findTaggedPrims();

  void findTaggedPrims(const HierarchyIterationLogics& iterationLogics);

  /// \brief  searches for the excluded geometry
  void findExcludedGeometry();

  /// \brief searches for paths which are selectable
  void findSelectablePrims();

  //// \brief iterates the prim hierarchy calling pre/iterate/post like functions that are stored in the passed in objects
  void iteratePrimHierarchy();

  /// \brief  returns the plugin translator registry assigned to this shape
  /// \return the translator registry
  fileio::translators::TranslatorManufacture& translatorManufacture()
    { return m_translatorManufacture; }

  /// \brief  returns the plugin translator context assigned to this shape
  /// \return the translator context
  fileio::translators::TranslatorContextPtr& context()
    { return m_context; }

  //--------------------------------------------------------------------------------------------------------------------
  /// \name   ProxyShape selection
  //--------------------------------------------------------------------------------------------------------------------

  /// \brief  returns the paths of the selected items within the proxy shape
  /// \return the paths of the selected prims
  SdfPathVector& selectedPaths()
    { return m_selectedPaths; }

  /// \brief  Performs a selection operation on this node. Intended for use by the ProxyShapeSelect command only
  /// \param  helper provides the arguments to the selection system, and stores the internal proxy shape state
  ///         changes that need to be done/undone
  /// \return true if the operation succeeded
  bool doSelect(SelectionUndoHelper& helper);

  //--------------------------------------------------------------------------------------------------------------------
  /// \name   UsdImaging
  //--------------------------------------------------------------------------------------------------------------------

  /// \brief  constructs the USD imaging engine for this shape
  void constructGLImagingEngine();

  /// \brief  returns the usd imaging engine for this proxy shape
  /// \return the imagine engin instance for this shape (shared between draw override and shape ui)
  inline UsdImagingGLHdEngine* engine() const
    { return m_engine; }

  //--------------------------------------------------------------------------------------------------------------------
  /// \name   Miscellaneous
  //--------------------------------------------------------------------------------------------------------------------

  /// \brief  unloads all maya references
  /// \todo   I think we could remove this now? The only place this is used is within the post load process to ensure we
  ///         don't duplicate any references in the scene. This can probably be removed.
  void unloadMayaReferences();

  /// \brief  if a root prim has been specified by the user in the proxy shape AE, then this method will return
  ///         the associated prim (used by the Hydra rendering code to select a root to render from). If no prim
  ///         has been specified, the pseudo root will be passed to UsdImaging
  /// \return the prim specified by the user (if valid), the pseudo root if no prim has been specified, or a NULL
  ///         prim if the stage is invalid.
  UsdPrim getRootPrim()
    {
      if(m_stage)
      {
        if(!m_path.IsEmpty())
        {
          UsdPrim prim = m_stage->GetPrimAtPath(m_path);
          if(prim)
            return prim;
        }
        return m_stage->GetPseudoRoot();
      }
      return UsdPrim();
    }

  /// \brief  serialise the state of the transform ref counts prior to saving the file
  void serialiseTransformRefs();

  /// \brief  deserialise the state of the transform ref counts prior to saving the file
  void deserialiseTransformRefs();

  /// \brief Finds the corresponding translator for each decendant prim that has a corresponding Translator 
  ///        and calls preTearDown.
  /// \param[in] path of the point in the hierarchy that is potentially undergoing structural changes
  /// \param[out] outPathVector of SdfPaths that are decendants of 'path'
  void onPrePrimChanged(const SdfPath& path, SdfPathVector& outPathVector);

  /// \brief Re-Creates and updates the maya prim hierarchy starting from the specified primpath
  /// \param[in] primPath of the point in the hierarchy that is potentially undergoing structural changes
  /// \param[in] changedPaths are child paths that existed previously and may not be existing now.
  void onPrimResync(SdfPath primPath, const SdfPathVector& changedPaths);

  /// \brief This function starts the prim changed process within the proxyshape
  /// \param[in] changePath is point at which the scene is going to be modified.
  inline void primChangedAtPath(const SdfPath& changePath)
  {
    UsdPrim p = m_stage->GetPrimAtPath(changePath);

    if(!p.IsValid())
    {
      MGlobal::displayInfo("ProxyShape: Could not change prim at path since there was no valid prim at the passed in path");
      return;
    }
    m_compositionHasChanged = true;
    m_changedPath = changePath;
    onPrePrimChanged(m_changedPath, m_variantSwitchedPrims);
  }

  /// \brief  change the status of the composition changed status
  /// \param  hasObjectsChanged
  inline void setHaveObjectsChangedAtPath(bool hasObjectsChanged)
    { m_compositionHasChanged = hasObjectsChanged; }

  /// \brief  provides access to the selection list on this proxy shape
  /// \return the internal selection list
  SelectionList& selectionList()
    { return m_selectionList; }

  /// \brief  internal method used to correctly schedule changes to the selection list
  /// \param  v the state
  inline void setChangedSelectionState(bool v)
    { m_hasChangedSelection = v; }

  /// \brief Checks to see if a passed in path is Selectable. This takes into account
  ///        the selection restriction flag
  /// \param[in] path which will be checked to see if it is selectable
  /// \return true if the path is selectable
  bool isPathSelectable(const SdfPath& path) const;

  /// \brief Returns if the selection is restricted
  /// \return true if the restricted selection is enabled
  bool isSelectionRestricted() const
    {return m_isRestrictedSelectionEnabled; }

  /// \brief Enables the restriction of the selectable paths
  void restrictSelection()
    { m_isRestrictedSelectionEnabled = true; }

  /// \brief Disables the restriction of the selectable paths
  void unrestrictSelection()
    { m_isRestrictedSelectionEnabled = false; }

  /// \brief Returns the SelectionDatabase owned by the ProxyShape
  /// \return ASelectableDB owned by the ProxyShape
  AL::usdmaya::SelectableDB& selectableDB()
    { return m_selectableDB; }

  /// \brief Returns the SelectionDatabase owned by the ProxyShape
  /// \return A constant SelectableDB owned by the ProxyShape
  const AL::usdmaya::SelectableDB& selectableDB() const
    { return const_cast<ProxyShape*>(this)->selectableDB(); }

private:
  static void onSelectionChanged(void* ptr);
  bool removeAllSelectedNodes(SelectionUndoHelper& helper);
  void removeTransformRefs(const std::vector<std::pair<SdfPath, MObject>>& removedRefs, TransformReason reason);
  void insertTransformRefs(const std::vector<std::pair<SdfPath, MObject>>& removedRefs, TransformReason reason);

  void constructExcludedPrims();

  MObject makeUsdTransformChain_internal(
      const UsdPrim& usdPrim,
      MDagModifier& modifier,
      TransformReason reason,
      MDGModifier* modifier2 = 0,
      uint32_t* createCount = 0,
      MString* newPath = 0);

  void removeUsdTransformChain_internal(
      const UsdPrim& usdPrim,
      MDagModifier& modifier,
      TransformReason reason);

  MObject makeUsdTransformChain(
      UsdPrim usdPrim,
      const MPlug& outStage,
      const MPlug& outTime,
      const MObject& parentXForm,
      MDagModifier& modifier,
      TransformReason reason,
      MDGModifier* modifier2,
      uint32_t* createCount,
      MString* newPath = 0);

  void makeUsdTransformsInternal(
      const UsdPrim& usdPrim,
      const MObject& parentXForm,
      MDagModifier& modifier,
      TransformReason reason,
      MDGModifier* modifier2);

  void removeUsdTransformsInternal(
      const UsdPrim& usdPrim,
      MDagModifier& modifier,
      TransformReason reason);

  struct TransformReference
  {
    TransformReference(const MObject& node, const TransformReason reason);
    TransformReference(MObject mayaNode, Transform* node, uint32_t r, uint32_t s, uint32_t rc);
    Transform* m_transform;
    MObject node() const { return m_node; }

    bool decRef(const TransformReason reason);
    void incRef(const TransformReason reason);
    void checkIncRef(const TransformReason reason);
    bool checkRef(const TransformReason reason);

    void printRefCounts() const
    {
      std::cout
                << m_required << ":"
                << m_selectedTemp << ":"
                << m_selected << ":"
                << int(m_refCount) << std::endl;
    }
    uint32_t selected() const { return m_selected; }
    uint32_t required() const { return m_required; }
    uint32_t refCount() const { return m_refCount; }
    void prepSelect()
      { m_selectedTemp = m_selected; }
  private:
    MObject m_node;
    // ref counting values
    struct
    {
      uint64_t m_required:16;
      uint64_t m_selectedTemp:16;
      uint64_t m_selected:16;
      uint64_t m_refCount:16;
    };
  };


  /// if the USD stage contains a maya reference et-al, then we have a set of *REQUIRED* AL::usdmaya::nodes::Transform nodes.
  /// If we then later create a USD transform node (because we're bringing in all of them, or just a selection of them),
  /// then we must make sure that we don't end up duplicating paths. This map is use to store a LUT of the paths that
  /// must always exist, and never get deleted.
  typedef std::map<SdfPath, TransformReference>  TransformReferenceMap;
  TransformReferenceMap m_requiredPaths;


  /// it is possible to end up with some invalid data in here as a result of a variant switch. When it looks as though a
  /// schema prim is going to change type, in cases where a payload fails to resolve, we can end up with null prims in the
  /// stage. As a result, it's corresponding transform ref can fail to load.
  void cleanupTransformRefs();

  /// insert a new path into the requiredPaths map
  void makeTransformReference(const SdfPath& path, const MObject& node, TransformReason reason);

  /// selection can cause multiple transform chains to be removed. To ensure the ref counts are correctly correlated,
  /// we need to make sure we can remove
  void prepSelect();

  //--------------------------------------------------------------------------------------------------------------------
  /// \name   Virtual overrides
  //--------------------------------------------------------------------------------------------------------------------

  void postConstructor() override;
  MStatus compute(const MPlug& plug, MDataBlock& dataBlock) override;
  MStatus setDependentsDirty(const MPlug& plugBeingDirtied, MPlugArray& plugs) override;
  bool isBounded() const override;
  #if MAYA_API_VERSION < 201700
  MPxNode::SchedulingType schedulingType() const override { return kSerialize; }
  #else
  MPxNode::SchedulingType schedulingType() const override { return kSerial; }
  #endif
  MStatus preEvaluation(const MDGContext & context, const MEvaluationNode& evaluationNode) override;

  //--------------------------------------------------------------------------------------------------------------------
  /// \name   Compute methods
  //--------------------------------------------------------------------------------------------------------------------

  // split out compute methods.
  MStatus computeInStageDataCached(const MPlug& plug, MDataBlock& dataBlock);
  MStatus computeOutStageData(const MPlug& plug, MDataBlock& dataBlock);
  MStatus computeOutputTime(const MPlug& plug, MDataBlock& dataBlock, MTime&);
  MStatus computeDrivenAttributes(const MPlug& plug, MDataBlock& dataBlock, const MTime&);

  //--------------------------------------------------------------------------------------------------------------------
  /// \name   Utils
  //--------------------------------------------------------------------------------------------------------------------

  UsdPrim getUsdPrim(MDataBlock& dataBlock) const;
  SdfPathVector getExcludePrimPaths() const;
  UsdStagePopulationMask constructStagePopulationMask(const MString &paths) const;
  SdfPathVector getPrimPathsFromCommaJoinedString(const MString &paths) const;
  bool isStageValid() const;
  bool primHasExcludedParent(UsdPrim prim);
  bool initPrim(const uint32_t index, MDGContext& ctx);

  void reloadStage(MPlug& plug);
  void layerIdChanged(SdfNotice::LayerIdentifierDidChange const& notice, UsdStageWeakPtr const& sender);
  void onObjectsChanged(UsdNotice::ObjectsChanged const&, UsdStageWeakPtr const& sender);
  void variantSelectionListener(SdfNotice::LayersDidChange const& notice, UsdStageWeakPtr const& sender);
  void onEditTargetChanged(UsdNotice::StageEditTargetChanged const& notice, UsdStageWeakPtr const& sender);
  static void onAttributeChanged(MNodeMessage::AttributeMessage, MPlug&, MPlug&, void*);
  void validateTransforms();


  TfToken getTypeForPath(const SdfPath& path) override
    { return m_context->getTypeForPath(path); }

  bool getTypeInfo(TfToken type, bool& supportsUpdate, bool& requiresParent) override
    {
      auto translator = m_translatorManufacture.get(type);
      if(translator)
      {
        supportsUpdate = translator->supportsUpdate();
        requiresParent = translator->needsTransformParent();
      }
      return translator != 0;
    }

private:
  AL::usdmaya::SelectableDB m_selectableDB;
  HierarchyIterationLogics m_hierarchyIterationLogics;
  HierarchyIterationLogic m_findExcludedPrims;
  SelectionList m_selectionList;
  FindSelectablePrimsLogic m_findSelectablePrims;
  SdfPathVector m_selectedPaths;
  std::vector<SdfPath> m_paths;
  std::vector<UsdPrim> m_prims;
  TfNotice::Key m_objectsChangedNoticeKey;
  TfNotice::Key m_variantChangedNoticeKey;
  TfNotice::Key m_editTargetChanged;

  mutable std::map<UsdTimeCode, MBoundingBox> m_boundingBoxCache;
  MCallbackId m_beforeSaveSceneId;
  MCallbackId m_attributeChanged;
  MCallbackId m_onSelectionChanged;
  SdfPathVector m_excludedGeometry;
  SdfPathVector m_excludedTaggedGeometry;
  UsdStageRefPtr m_stage;
  SdfPath m_path;
  fileio::translators::TranslatorContextPtr m_context;
  fileio::translators::TranslatorManufacture m_translatorManufacture;
  SdfPath m_changedPath;
  SdfPathVector m_variantSwitchedPrims;
  UsdImagingGLHdEngine* m_engine = 0;

  uint32_t m_engineRefCount = 0;
  bool m_compositionHasChanged = false;
  bool m_drivenTransformsDirty = false;
  bool m_pleaseIgnoreSelection = false;
  bool m_hasChangedSelection = false;
  bool m_isRestrictedSelectionEnabled = false; //< If the restrictable selection is true, then the SelectableDB determines what prims are selectable or not.
};

//----------------------------------------------------------------------------------------------------------------------
} // nodes
} // usdmaya
} // AL
//----------------------------------------------------------------------------------------------------------------------
