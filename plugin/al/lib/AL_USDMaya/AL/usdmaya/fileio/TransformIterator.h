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

#include <pxr/usd/usd/prim.h>

#include <maya/MDagPath.h>

PXR_NAMESPACE_USING_DIRECTIVE

namespace AL {
namespace usdmaya {
namespace fileio {

//----------------------------------------------------------------------------------------------------------------------
/// \brief  An iterator to walk over the transforms within a USD file.
/// \ingroup   fileio
//----------------------------------------------------------------------------------------------------------------------
class TransformIterator
{
public:
    /// \brief  ctor. Initialises the iterator to the root of the stage
    /// \param  stage the stage to iterate over
    /// \param  parentPath the DAG path of the proxy shape
    /// \param  stopOnInstance if true, the iterator will not iterate through children of an
    /// instance
    AL_USDMAYA_PUBLIC
    TransformIterator(
        UsdStageRefPtr  stage,
        const MDagPath& parentPath = MDagPath(),
        bool            stopOnInstance = false);

    /// \brief  ctor. Initialises the iterator to the root of the stage
    /// \param  startPrim a prim in a stage where the iteration should start
    /// \param  startMayaPath the DAG path of the proxy shape
    /// \param  stopOnInstance if true, the iterator will not iterate through children of an
    /// instance
    AL_USDMAYA_PUBLIC
    TransformIterator(
        const UsdPrim&  startPrim,
        const MDagPath& startMayaPath,
        bool            stopOnInstance = false);

    /// \brief  return true if the iteration is complete
    /// \return true when the iteration is complete
    inline bool done() const { return m_primStack.empty(); }

    /// \brief  returns the current iteration depth
    /// \return the iteration depth
    inline size_t depth() const { return m_primStack.size(); }

    /// \brief  return the current prim
    /// \return the current prim
    inline const UsdPrim& prim() const { return (m_primStack.end() - 1)->m_prim; }

    /// \brief  return the current prim
    /// \return the current prim
    AL_USDMAYA_PUBLIC
    UsdPrim parentPrim() const;

    /// \brief  do not iterate over the children of this node
    AL_USDMAYA_PUBLIC
    void prune();

    /// \brief  move to the next item in the stage
    /// \return true if we managed to move to the next prim, false if we have finished traversal
    AL_USDMAYA_PUBLIC
    bool next();

    /// \brief  in order to keep the maya path in sync with the USD prim, at each iteration step you
    ///         should pass in the MObject of the node in maya that mirrors your place in the USD
    ///         hierarchy. Without this step the current dag path will not be valid.
    /// \param  newNode the node to append into the stack
    inline void append(const MObject newNode)
    {
        if (!m_primStack.empty())
            m_primStack[m_primStack.size() - 1].m_object = newNode;
    }

    /// \brief  returns the parent transform of the current node.
    /// \return the MObject of the parent node, r MObject::kNullObj if no valid parent
    inline MObject parent() const
    {
        if (m_primStack.size() > 1) {
            if ((m_primStack.end() - 2)->m_object != MObject::kNullObj) {
                return (m_primStack.end() - 2)->m_object;
            }
            if ((m_primStack.end() - 1)->m_object != MObject::kNullObj) {
                return (m_primStack.end() - 1)->m_object;
            }
        }
        if (m_primStack.size() == 1) {
            return (m_primStack.end() - 1)->m_object;
        }
        return m_parentPath.node();
    }

    /// \brief  returns the current maya path equivalent of the current USD prim.
    /// \return the maya dag path
    AL_USDMAYA_PUBLIC
    MDagPath currentPath() const;

private:
    struct StackRef
    {
        StackRef(const UsdPrim& prim);
        StackRef(const StackRef& prim);
        StackRef();
        UsdPrim                  m_prim;
        MObject                  m_object;
        UsdPrim::SiblingIterator m_begin;
        UsdPrim::SiblingIterator m_end;
        int32_t                  m_currentChild;
        int32_t                  m_numChildren;
    };

    std::vector<StackRef>             m_primStack;
    UsdStageRefPtr                    m_stage;
    MDagPath                          m_parentPath;
    TfHashSet<SdfPath, SdfPath::Hash> m_visitedMasterPrimPaths;
    bool                              m_stopOnInstance;
};

//----------------------------------------------------------------------------------------------------------------------
} // namespace fileio
} // namespace usdmaya
} // namespace AL
//----------------------------------------------------------------------------------------------------------------------
