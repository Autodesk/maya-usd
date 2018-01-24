#pragma once

#include "pxr/pxr.h"
#include "pxr/usd/usd/prim.h"
#include "pxr/usd/sdf/path.h"
#include <algorithm>

PXR_NAMESPACE_USING_DIRECTIVE

namespace AL {
namespace usdmaya {

///---------------------------------------------------------------------------------------------------------------------
/// \brief  Logic that stores a sorted list of paths which represent Selectable points in the USD hierarchy
///---------------------------------------------------------------------------------------------------------------------
class SelectabilityDB {
public:

  ///-------------------------------------------------------------------------------------------------------------------
  /// \brief  Determines this path is unselectable
  /// \param  path that you want to determine if it's unselectable
  ///-------------------------------------------------------------------------------------------------------------------
  bool isPathUnselectable(const SdfPath& path) const;

  ///-------------------------------------------------------------------------------------------------------------------
  /// \brief  Adds a list of paths to the selectable list
  /// \param  paths which will be added as selectable. All children paths will be also unselectable
  ///-------------------------------------------------------------------------------------------------------------------
  void addPathsAsUnselectable(const SdfPathVector& paths);

  ///-------------------------------------------------------------------------------------------------------------------
  /// \brief  Adds a path to the unselectable list
  /// \param  path which will be added as unselectable. All children paths will be also unselectable
  ///-------------------------------------------------------------------------------------------------------------------
  void addPathAsUnselectable(const SdfPath& path);

  ///-------------------------------------------------------------------------------------------------------------------
  /// \brief  Gets the currently explictly tracked unseletable paths
  ///-------------------------------------------------------------------------------------------------------------------
  inline const SdfPathVector& getUnselectablePaths() const { return m_unselectablePaths; }

  ///-------------------------------------------------------------------------------------------------------------------
  /// \brief  Removes a list of paths from the selectable list if the exist.
  /// \param  paths the paths to remove from the selectable list
  ///-------------------------------------------------------------------------------------------------------------------
  void removePathsAsUnselectable(const SdfPathVector& paths);

  ///-------------------------------------------------------------------------------------------------------------------
  /// \brief  Remove a path from the selectable list if the exist.
  /// \param  path the path to remove from the selectable list
  ///-------------------------------------------------------------------------------------------------------------------
  void removePathAsUnselectable(const SdfPath& path);

private:
  inline void sort(){std::sort(m_unselectablePaths.begin(), m_unselectablePaths.end());}
  bool addUnselectablePath(const SdfPath& path);
  bool removeUnselectablePath(const SdfPath& path);

private:
  SdfPathVector m_unselectablePaths;
};

//----------------------------------------------------------------------------------------------------------------------
} // usdmaya
} // AL
//----------------------------------------------------------------------------------------------------------------------
