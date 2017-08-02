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
class SelectableDB {
public:

  ///-------------------------------------------------------------------------------------------------------------------
  /// \brief  Determines this path is selectable
  /// \param  path that you want to determine if it's selectable
  ///-------------------------------------------------------------------------------------------------------------------
  bool isPathSelectable(const SdfPath& path) const;

  ///-------------------------------------------------------------------------------------------------------------------
  /// \brief  Adds a list of paths to the selectable list
  /// \param  paths which will be added as selectable. All children paths will be also selectable
  ///-------------------------------------------------------------------------------------------------------------------
  void addPathsAsSelectable(const SdfPathVector& paths);

  ///-------------------------------------------------------------------------------------------------------------------
  /// \brief  Adds a path to the selectable list
  /// \param  paths which will be added as selectable. All children paths will be also selectable
  ///-------------------------------------------------------------------------------------------------------------------
  void addPathAsSelectable(const SdfPath& path);

  ///-------------------------------------------------------------------------------------------------------------------
  /// \brief  Gets the currently explictly tracked seletable paths
  ///-------------------------------------------------------------------------------------------------------------------
  inline const SdfPathVector& getSelectablePaths() const { return m_selectablePaths; }

  ///-------------------------------------------------------------------------------------------------------------------
  /// \brief  Removes a list of paths from the selectable list if the exist.
  ///-------------------------------------------------------------------------------------------------------------------
  void removePathsAsSelectable(const SdfPathVector& paths);

  ///-------------------------------------------------------------------------------------------------------------------
  /// \brief  Remove a path from the selectable list if the exist.
  ///-------------------------------------------------------------------------------------------------------------------
  void removePathAsSelectable(const SdfPath& path);

private:
  inline void sort(){std::sort(m_selectablePaths.begin(), m_selectablePaths.end());}
  bool addSelectablePath(const SdfPath& path);
  bool removeSelectablePath(const SdfPath& path);

private:
  SdfPathVector m_selectablePaths;
};

//----------------------------------------------------------------------------------------------------------------------
} // usdmaya
} // AL
//----------------------------------------------------------------------------------------------------------------------
