#include <AL/usdmaya/SelectableDB.h>

namespace AL {
namespace usdmaya {

bool SelectableDB::isPathSelectable(const SdfPath& path) const
{
  for(SdfPath selectablePath : m_selectablePaths)
  {
    if(path.HasPrefix(selectablePath))
    {
      return true;
    }
  }

  return false;
}

void SelectableDB::removePathsAsSelectable(const SdfPathVector& paths)
{
  bool needsSort = false;
  for(SdfPath path : paths)
  {
    needsSort |= removeSelectablePath(path);
  }

  if(needsSort)
  {
    sort();
  }
}

void SelectableDB::removePathAsSelectable(const SdfPath& path)
{
  if(removeSelectablePath(path))
  {
    sort();
  }
}

void SelectableDB::addPathsAsSelectable(const SdfPathVector& paths)
{
  bool needsSort = false;
  for(SdfPath path : paths)
  {
    needsSort |= addSelectablePath(path);
  }

  if(needsSort)
  {
    sort();
  }
}

void SelectableDB::addPathAsSelectable(const SdfPath& path)
{
  if(addSelectablePath(path))
  {
    sort();
  }
}

bool SelectableDB::removeSelectablePath(const SdfPath& path)
{
  auto foundPathEntry = std::find(m_selectablePaths.begin(), m_selectablePaths.end(), path);
  if(foundPathEntry != m_selectablePaths.end())
  {
    m_selectablePaths.erase(foundPathEntry);
    return true;
  }
  return false;
}

bool SelectableDB::addSelectablePath(const SdfPath& path)
{
  if(!std::binary_search(m_selectablePaths.begin(), m_selectablePaths.end(), path))
  {
    m_selectablePaths.push_back(path);
    return true;
  }
  return false;
}

//----------------------------------------------------------------------------------------------------------------------
} // usdmaya
} // AL
//----------------------------------------------------------------------------------------------------------------------
