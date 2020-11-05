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
#include "AL/usdmaya/fileio/TransformIterator.h"

#include "AL/usdmaya/DebugCodes.h"

#include <pxr/usd/usd/stage.h>

#include <maya/MFnDagNode.h>

#include <iostream>

namespace AL {
namespace usdmaya {
namespace fileio {

//----------------------------------------------------------------------------------------------------------------------
TransformIterator::TransformIterator(
    UsdStageRefPtr  stage,
    const MDagPath& parentPath,
    bool            stopOnInstance)
    : m_primStack()
    , m_stage(stage)
    , m_stopOnInstance(stopOnInstance)
{
    TF_DEBUG(ALUSDMAYA_TRANSLATORS)
        .Msg(
            "TransformIterator::TransformIterator parent path: %s\n",
            parentPath.fullPathName().asChar());

    m_primStack.reserve(128);
    UsdPrim psuedoPrim = stage->GetPseudoRoot();
    m_primStack.push_back(StackRef(psuedoPrim));
    if (parentPath.length())
        append(parentPath.node());
    else
        append(MObject::kNullObj);
    next(); // skip the psuedo root.
    if (parentPath.length())
        append(parentPath.node());
    else
        append(MObject::kNullObj);
    m_parentPath = parentPath;
}

//----------------------------------------------------------------------------------------------------------------------
TransformIterator::TransformIterator(
    const UsdPrim&  usdStartPrim,
    const MDagPath& mayaStartPath,
    bool            stopOnInstance)
    : m_primStack()
    , m_stage(usdStartPrim.GetStage())
    , m_stopOnInstance(stopOnInstance)
{
    m_primStack.reserve(128);
    m_primStack.push_back(StackRef(usdStartPrim));
    append(mayaStartPath.node());
    m_parentPath = mayaStartPath;
}

//----------------------------------------------------------------------------------------------------------------------
MDagPath TransformIterator::currentPath() const
{
    TF_DEBUG(ALUSDMAYA_TRANSLATORS).Msg("TransformIterator::currentPath\n");
    MDagPath p = m_parentPath;
    for (auto it = m_primStack.begin(); it != m_primStack.end(); ++it) {
        if (it->m_object != MObject::kNullObj) {
            {
                MFnDagNode fn(it->m_object);
                fn.getPath(p);
            }
        }
    }
    return p;
}

//----------------------------------------------------------------------------------------------------------------------
void TransformIterator::prune() { m_primStack.pop_back(); }

//----------------------------------------------------------------------------------------------------------------------
bool TransformIterator::next()
{
    if (done()) {
        return false;
    }
    do {
        StackRef& r = *(m_primStack.end() - 1);
        if (r.m_prim.IsInstance() && !m_stopOnInstance) {
            if (!m_stopOnInstance) {
                UsdPrim master = r.m_prim.GetMaster();
                m_primStack.push_back(StackRef(master));
                m_visitedMasterPrimPaths.insert(master.GetPath());
                StackRef& p = *(m_primStack.end() - 2);
                StackRef& c = *(m_primStack.end() - 1);
                c.m_object = p.m_object;
                next();
            }
            return !done();
        } else if (r.m_begin == r.m_end) {
            m_primStack.pop_back();
            if (m_primStack.empty()) {
                break;
            }

            {
                StackRef& b = *(m_primStack.end() - 1);
                if (b.m_prim.IsInstance() && !m_stopOnInstance) {
                    m_primStack.pop_back();
                }
            }
        } else {
            m_primStack.push_back(StackRef(*(r.m_begin++)));
            return !done();
        }
    } while (!done());
    return !done();
}

//----------------------------------------------------------------------------------------------------------------------
TransformIterator::StackRef::StackRef(const UsdPrim& prim)
    : m_prim(prim)
    , m_object(MObject::kNullObj)
    , m_begin()
    , m_end()
    , m_currentChild(-1)
    , m_numChildren(0)
{
    if (prim) {
        auto begin = prim.GetChildren().begin();
        m_begin = begin;
        m_end = prim.GetChildren().end();
        while (begin != m_end) {
            m_numChildren++;
            ++begin;
        }
    } else {
        std::cout << "StackRef::StackRef initialized with null prim" << std::endl;
    }
}

//----------------------------------------------------------------------------------------------------------------------
TransformIterator::StackRef::StackRef(const StackRef& prim)
    : m_prim(prim.m_prim)
    , m_object(prim.m_object)
    , m_begin(prim.m_begin)
    , m_end(prim.m_end)
    , m_currentChild(prim.m_currentChild)
    , m_numChildren(prim.m_numChildren)
{
}

//----------------------------------------------------------------------------------------------------------------------
TransformIterator::StackRef::StackRef()
    : m_prim()
    , m_object(MObject::kNullObj)
    , m_begin()
    , m_end()
    , m_currentChild(-1)
    , m_numChildren(0)
{
}

//----------------------------------------------------------------------------------------------------------------------
UsdPrim TransformIterator::parentPrim() const
{
    UsdPrim parentPrim = m_stage->GetPseudoRoot();
    if (m_primStack.size() > 1) {
        parentPrim = (m_primStack.end() - 2)->m_prim;
        if (parentPrim.IsMaster()) {
            if (m_primStack.size() > 2) {
                parentPrim = (m_primStack.end() - 3)->m_prim;
            }
        }
    }
    return parentPrim;
}

//----------------------------------------------------------------------------------------------------------------------
} // namespace fileio
} // namespace usdmaya
} // namespace AL
//----------------------------------------------------------------------------------------------------------------------