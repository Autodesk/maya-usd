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
#include <AL/usdmaya/SelectabilityDB.h>

namespace AL {
namespace usdmaya {

//----------------------------------------------------------------------------------------------------------------------
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

//----------------------------------------------------------------------------------------------------------------------
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

//----------------------------------------------------------------------------------------------------------------------
void SelectabilityDB::removePathAsUnselectable(const SdfPath& path)
{
  if(removeUnselectablePath(path))
  {
    sort();
  }
}

//----------------------------------------------------------------------------------------------------------------------
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

//----------------------------------------------------------------------------------------------------------------------
void SelectabilityDB::addPathAsUnselectable(const SdfPath& path)
{
  if(addUnselectablePath(path))
  {
    sort();
  }
}

//----------------------------------------------------------------------------------------------------------------------
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

//----------------------------------------------------------------------------------------------------------------------
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
