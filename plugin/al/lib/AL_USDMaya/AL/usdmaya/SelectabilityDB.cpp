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
    auto begin = m_unselectablePaths.begin();
    auto end = m_unselectablePaths.end();
    auto foundPathEntry = end;
    auto root = SdfPath::AbsoluteRootPath();
    auto temp(path);

    while (temp != root) {
        foundPathEntry = std::lower_bound(begin, foundPathEntry, temp);
        if (foundPathEntry != end && temp == *foundPathEntry) {
            return true;
        }
        temp = temp.GetParentPath();
    }
    return false;
}

//----------------------------------------------------------------------------------------------------------------------
void SelectabilityDB::removePathsAsUnselectable(const SdfPathVector& paths)
{
    for (SdfPath path : paths) {
        removeUnselectablePath(path);
    }

    auto end = m_unselectablePaths.end();
    auto start = m_unselectablePaths.begin();
    for (SdfPath path : paths) {
        auto temp = std::lower_bound(start, end, path);
        if (temp != end) {
            if (*temp == path) {
                temp = m_unselectablePaths.erase(temp);
            }
            start = temp;
        }
    }
}

//----------------------------------------------------------------------------------------------------------------------
void SelectabilityDB::removePathAsUnselectable(const SdfPath& path)
{
    removeUnselectablePath(path);
}

//----------------------------------------------------------------------------------------------------------------------
void SelectabilityDB::addPathsAsUnselectable(const SdfPathVector& paths)
{
    auto end = m_unselectablePaths.end();
    auto start = m_unselectablePaths.begin();
    for (auto iter = paths.begin(), last = paths.end(); iter != last; ++iter) {
        start = std::lower_bound(start, end, *iter);

        // If we've hit the end, we can simply append the remaining elements in one go.
        if (start == end) {
            m_unselectablePaths.insert(end, iter, last);
            return;
        }

        if (*start != *iter) {
            m_unselectablePaths.insert(start, *iter);
        }
    }
}

//----------------------------------------------------------------------------------------------------------------------
void SelectabilityDB::addPathAsUnselectable(const SdfPath& path) { addUnselectablePath(path); }

//----------------------------------------------------------------------------------------------------------------------
bool SelectabilityDB::removeUnselectablePath(const SdfPath& path)
{
    auto end = m_unselectablePaths.end();
    auto foundPathEntry = std::lower_bound(m_unselectablePaths.begin(), end, path);
    if (foundPathEntry != end && *foundPathEntry == path) {
        m_unselectablePaths.erase(foundPathEntry);
        return true;
    }
    return false;
}

//----------------------------------------------------------------------------------------------------------------------
bool SelectabilityDB::addUnselectablePath(const SdfPath& path)
{
    auto end = m_unselectablePaths.end();
    auto iter = std::lower_bound(m_unselectablePaths.begin(), end, path);
    if (iter != end) {
        if (*iter != path) {
            m_unselectablePaths.insert(iter, path);
            return true;
        }
    } else {
        m_unselectablePaths.push_back(path);
        return true;
    }
    return false;
}

//----------------------------------------------------------------------------------------------------------------------
} // namespace usdmaya
} // namespace AL
//----------------------------------------------------------------------------------------------------------------------