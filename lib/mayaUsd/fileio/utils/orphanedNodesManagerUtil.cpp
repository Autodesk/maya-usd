//
// Copyright 2022 Autodesk
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
#include "orphanedNodesManagerUtil.h"

#include <maya/MGlobal.h>

namespace MAYAUSD_NS_DEF {

namespace {

using PullVariantInfo = OrphanedNodesManager::PullVariantInfo;
using VariantSetDescriptor = OrphanedNodesManager::VariantSetDescriptor;
using VariantSelection = OrphanedNodesManager::VariantSelection;

Ufe::Path trieNodeToPullePrimUfePath(Ufe::TrieNode<PullVariantInfo>::Ptr trieNode);

void addIndent(std::string& buf, int indent)
{
    for (int i = 0; i < indent; ++i)
        buf += "    ";
}

void toText(std::string& buf, const char* pfix, const char* text, int indent, bool eol)
{
    addIndent(buf, indent);
    if (pfix && pfix[0]) {
        buf += pfix;
        buf += ": ";
    }
    buf += text;
    if (eol)
        buf += "\n";
}

void toText(std::string& buf, const char* pfix, const std::string& text, int indent, bool eol)
{
    toText(buf, pfix, text.c_str(), indent, eol);
}

void toText(std::string& buf, const char* pfix, const MDagPath& dagPath, int indent, bool eol)
{
    toText(buf, pfix, dagPath.fullPathName().asChar(), indent, eol);
}

void toText(std::string& buf, const char* pfix, const Ufe::Path& ufePath, int indent, bool eol)
{
    toText(buf, pfix, ufePath.string(), indent, eol);
}

void toText(std::string& buf, const VariantSelection& sel, int indent, bool eol)
{
    toText(buf, "Variant  ", sel.variantSetName, indent, eol);
    toText(buf, "Selection", sel.variantSelection, indent, eol);
}

void toText(std::string& buf, const VariantSetDescriptor& descriptor, int indent, bool eol)
{
    toText(buf, "Variant selections", descriptor.path, indent, eol);

    indent += 1;

    for (const auto& variantSel : descriptor.variantSelections) {
        toText(buf, variantSel, indent, eol);
    }

    indent -= 1;
}

void toText(std::string& buf, const PullVariantInfo& variantInfo, int indent, bool eol)
{
    toText(buf, "", "{", indent, eol);
    indent += 1;

    toText(buf, "Edited  Maya  root", variantInfo.editedAsMayaRoot, indent, eol);
    toText(buf, "proxy  shape  path", variantInfo.proxyShapePath, indent, eol);
    toText(buf, "pulled parent path", variantInfo.pulledParentPath, indent, eol);

    for (const auto& desc : variantInfo.variantSetDescriptors)
        toText(buf, desc, indent, eol);

    indent -= 1;
    toText(buf, "", "}", indent, eol);
}

} // namespace

void OrphanedNodesManagerPullInfoToText(
    std::string&                               buffer,
    const Ufe::TrieNode<PullVariantInfo>::Ptr& trieNode,
    int                                        indent,
    bool                                       eol)
{
    if (!trieNode)
        return;

    Ufe::TrieNode<PullVariantInfo>& node = *trieNode;

    toText(buffer, "", node.component().string(), indent, eol);

    if (node.hasData()) {
        toText(buffer, node.data(), indent, eol);
    }

    for (const auto& childComp : node.childrenComponents()) {
        OrphanedNodesManagerPullInfoToText(buffer, node[childComp], indent + 1, eol);
    }

    if (eol)
        buffer += "\n";
}

void printOrphanedNodesManagerPullInfo(
    const Ufe::TrieNode<PullVariantInfo>::Ptr& trieNode,
    int                                        indent,
    bool                                       eol)
{
    std::string buffer("Trie ==========================================\n");
    OrphanedNodesManagerPullInfoToText(buffer, trieNode, indent, eol);
    MGlobal::displayInfo(buffer.c_str());
}

} // namespace MAYAUSD_NS_DEF
