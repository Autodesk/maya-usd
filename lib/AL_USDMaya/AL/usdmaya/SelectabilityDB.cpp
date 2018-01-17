#include <AL/usdmaya/SelectabilityDB.h>

namespace AL {
namespace usdmaya {

bool SelectabilityDB::isPathUnselectable(const SdfPath& path) const
{
  for(SdfPath unselectablePath : m_unselectablePaths)
  {
    if(path.HasPrefix(unselectablePath))
    {
      return true;
    }
  }

  return false;
}

void SelectabilityDB::removePathsAsUnselectable(const SdfPathVector& paths)
{
  bool needsSort = false;
  for(SdfPath path : paths)
  {
    needsSort |= removeUnselectablePath(path);
  }

  if(needsSort)
  {
    sort();
  }
}

void SelectabilityDB::removePathAsUnselectable(const SdfPath& path)
{
  if(removeUnselectablePath(path))
  {
    sort();
  }
}

void SelectabilityDB::addPathsAsUnselectable(const SdfPathVector& paths)
{
  bool needsSort = false;
  for(SdfPath path : paths)
  {
    needsSort |= addUnselectablePath(path);
  }

  if(needsSort)
  {
    sort();
  }
}

void SelectabilityDB::addPathAsUnselectable(const SdfPath& path)
{
  if(addUnselectablePath(path))
  {
    sort();
  }
}

bool SelectabilityDB::removeUnselectablePath(const SdfPath& path)
{
  auto foundPathEntry = std::find(m_unselectablePaths.begin(), m_unselectablePaths.end(), path);
  if(foundPathEntry != m_unselectablePaths.end())
  {
    m_unselectablePaths.erase(foundPathEntry);
    return true;
  }
  return false;
}

bool SelectabilityDB::addUnselectablePath(const SdfPath& path)
{
  if(!std::binary_search(m_unselectablePaths.begin(), m_unselectablePaths.end(), path))
  {
    m_unselectablePaths.push_back(path);
    return true;
  }
  return false;
}

//----------------------------------------------------------------------------------------------------------------------
} // usdmaya
} // AL
//----------------------------------------------------------------------------------------------------------------------
