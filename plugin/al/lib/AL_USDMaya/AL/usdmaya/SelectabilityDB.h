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

#include "AL/usdmaya/Api.h"
#include "AL/usdmaya/ForwardDeclares.h"

#include <pxr/pxr.h>
#include <pxr/usd/sdf/path.h>

PXR_NAMESPACE_USING_DIRECTIVE

namespace AL {
namespace usdmaya {

///---------------------------------------------------------------------------------------------------------------------
/// \brief  Logic that stores a sorted list of paths which represent Selectable points in the USD
/// hierarchy
///---------------------------------------------------------------------------------------------------------------------
class SelectabilityDB
{
    // at little unpleasant design wise, however it means the proxy shape can directly modify the
    // internal list of unselectable paths. I have looked at moving some of that logic into this
    // class, however that would incur a performance penalty (lock & excluded prim processing can be
    // done at the same time).
    friend class nodes::ProxyShape;

public:
    SelectabilityDB()
        : m_unselectablePaths()
    {
        m_unselectablePaths.reserve(128);
    }

    //--------------------------------------------------------------------------------------------------------------------
    /// \brief  Determines this path is unselectable
    /// \param  path that you want to determine if it's unselectable
    //--------------------------------------------------------------------------------------------------------------------
    AL_USDMAYA_PUBLIC
    bool isPathUnselectable(const SdfPath& path) const;

    //--------------------------------------------------------------------------------------------------------------------
    /// \brief  Determines whether there is an internal entry for the path specified (and only this
    /// path!). If you wish
    ///         to determine selectability, call isPathUnselectable instead.
    /// \param  path the path to check to see if exists
    /// \return true if the path is contained, false if not.
    //--------------------------------------------------------------------------------------------------------------------
    bool containsPath(const SdfPath& path) const
    {
        auto foundPathEntry
            = std::lower_bound(m_unselectablePaths.begin(), m_unselectablePaths.end(), path);
        if (foundPathEntry != m_unselectablePaths.end() && path == *foundPathEntry) {
            return true;
        }
        return false;
    }

    //--------------------------------------------------------------------------------------------------------------------
    /// \brief  Adds a list of paths to the selectable list
    /// \param  paths which will be added as selectable. All children paths will be also
    /// unselectable
    //--------------------------------------------------------------------------------------------------------------------
    AL_USDMAYA_PUBLIC
    void addPathsAsUnselectable(const SdfPathVector& paths);

    //--------------------------------------------------------------------------------------------------------------------
    /// \brief  Sets a list of paths to the selectable list
    /// \param  paths which will be added as selectable. All children paths will be also
    /// unselectable
    //--------------------------------------------------------------------------------------------------------------------
    void setPathsAsUnselectable(const SdfPathVector& paths)
    {
        m_unselectablePaths = paths;
        std::sort(m_unselectablePaths.begin(), m_unselectablePaths.end());
    }

    //--------------------------------------------------------------------------------------------------------------------
    /// \brief  Adds a path to the unselectable list
    /// \param  path which will be added as unselectable. All children paths will be also
    /// unselectable
    //--------------------------------------------------------------------------------------------------------------------
    AL_USDMAYA_PUBLIC
    void addPathAsUnselectable(const SdfPath& path);

    //--------------------------------------------------------------------------------------------------------------------
    /// \brief  Gets the currently explictly tracked unseletable paths
    //--------------------------------------------------------------------------------------------------------------------
    inline const SdfPathVector& getUnselectablePaths() const { return m_unselectablePaths; }

    ///-------------------------------------------------------------------------------------------------------------------
    /// \brief  Removes a list of paths from the selectable list if the exist.
    /// \param  paths the paths to remove from the selectable list
    ///-------------------------------------------------------------------------------------------------------------------
    AL_USDMAYA_PUBLIC
    void removePathsAsUnselectable(const SdfPathVector& paths);

    //--------------------------------------------------------------------------------------------------------------------
    /// \brief  Remove a path from the selectable list if the exist.
    /// \param  path the path to remove from the selectable list
    //--------------------------------------------------------------------------------------------------------------------
    AL_USDMAYA_PUBLIC
    void removePathAsUnselectable(const SdfPath& path);

private:
    bool addUnselectablePath(const SdfPath& path);
    bool removeUnselectablePath(const SdfPath& path);

private:
    SdfPathVector m_unselectablePaths;
};

//----------------------------------------------------------------------------------------------------------------------
} // namespace usdmaya
} // namespace AL
//----------------------------------------------------------------------------------------------------------------------