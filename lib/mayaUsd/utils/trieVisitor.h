//
// Copyright 2023 Autodesk
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
#ifndef MAYA_USD_UTIL_TRIE_VISITOR_H
#define MAYA_USD_UTIL_TRIE_VISITOR_H

#include <mayaUsd/base/api.h>

#include <ufe/sceneSegmentHandler.h>
#include <ufe/trie.h>

#include <functional>

namespace MAYAUSD_NS_DEF {

/// TrieVisitor allows visiting all nodes of a UFE Trie and receiving the full,
/// correctly built UFE path of each node.
///
/// Note: this cannot be moved to UsdUfe since it needs to know about Maya run-time
///       to build the UFE path segments with teh correct run-time ID.

template <class T> struct TrieVisitor
{
    using TrieVistorFunction = std::function<void(const Ufe::Path&, Ufe::TrieNode<T>& node)>;
    using TrieNodePtr = typename Ufe::TrieNode<T>::Ptr;

    /// \brief Visit each node of the \p trie, calling the given \p function.
    ///
    ///        By default, only vist node with data, controlled by \p allNodes
    ///        This visit function assumes the root UFE path is in a Maya segment
    ///        and switches of run-times alternate between Maya and USD.
    MAYAUSD_CORE_PUBLIC
    static void
    visit(const Ufe::Trie<T>& trie, const TrieVistorFunction& function, bool allNodes = false);

    /// \brief Visit the trie \p node anchored below \p parentPath and its children,
    ///        calling the given \p function.
    ///
    ///        By default, only vist node with data, controlled by \p allNodes
    ///        This visit function assumes the root UFE path is in a Maya segment
    ///        and switches of run-times alternate between Maya and USD.
    MAYAUSD_CORE_PUBLIC
    static void visit(
        const Ufe::Path&          parentPath,
        const TrieNodePtr&        node,
        const TrieVistorFunction& function,
        bool                      allNodes = false);
};

template <class T>
inline void
TrieVisitor<T>::visit(const Ufe::Trie<T>& trie, const TrieVistorFunction& function, bool allNodes)
{
    visit(Ufe::Path(), trie.root(), function, allNodes);
}

template <class T>
inline void TrieVisitor<T>::visit(
    const Ufe::Path&          parentPath,
    const TrieNodePtr&        node,
    const TrieVistorFunction& function,
    bool                      allNodes)
{
    if (!node)
        return;

    // Create the full UFE path for thr given node. Detect switch between
    // the Maya and USD runtimes.
    Ufe::Path nodePath;
    {
        const Ufe::PathComponent nodeComp = node->component();
        if (nodeComp) {
            if (parentPath.empty()) {
                nodePath = Ufe::Path(Ufe::PathSegment(nodeComp, ufe::getMayaRunTimeId(), '|'));
            } else if (Ufe::SceneSegmentHandler::isGateway(parentPath)) {
                if (parentPath.runTimeId() == ufe::getUsdRunTimeId()) {
                    nodePath
                        = parentPath + Ufe::PathSegment(nodeComp, ufe::getMayaRunTimeId(), '|');
                } else {
                    nodePath = parentPath + Ufe::PathSegment(nodeComp, ufe::getUsdRunTimeId(), '/');
                }
            } else {
                nodePath = parentPath + nodeComp;
            }
        }
    }

    // Visit the node if we visit all nodes or if it contains data.
    if (allNodes || node->hasData()) {
        function(nodePath, *node);
    }

    // Visit all children nodes.
    for (const Ufe::PathComponent& childComp : node->childrenComponents()) {
        TrieNodePtr childNode = (*node)[childComp];
        visit(nodePath, childNode, function, allNodes);
    }
} // namespace MAYAUSD_NS_DEF

} // namespace MAYAUSD_NS_DEF

#endif // MAYA_USD_UTIL_TRIE_VISITOR_H
