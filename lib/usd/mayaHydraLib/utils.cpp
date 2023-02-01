//
// Copyright 2020 Autodesk
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
#include "utils.h"

#include <pxr/base/tf/stringUtils.h>
#include <pxr/base/tf/token.h>
#include <pxr/pxr.h>

#include <maya/MFnDependencyNode.h>
#include <maya/MHWGeometry.h>
#include <maya/MObject.h>
#include <maya/MPlugArray.h>
#include <maya/MStatus.h>

#include <sstream>


namespace MAYAHYDRA_NS_DEF {

namespace MayaAttrs = PXR_NS::MayaAttrs;

MObject GetConnectedFileNode(const MObject& obj, const TfToken& paramName)
{
    MStatus           status;
    MFnDependencyNode node(obj, &status);
    if (ARCH_UNLIKELY(!status)) {
        return MObject::kNullObj;
    }
    return GetConnectedFileNode(node, paramName);
}

MObject GetConnectedFileNode(const MFnDependencyNode& node, const TfToken& paramName)
{
    MPlugArray conns;
    node.findPlug(paramName.GetText(), true).connectedTo(conns, true, false);
    if (conns.length() == 0) {
        return MObject::kNullObj;
    }
    const auto ret = conns[0].node();
    if (ret.apiType() == MFn::kFileTexture) {
        return ret;
    }
    return MObject::kNullObj;
}

TfToken GetFileTexturePath(const MFnDependencyNode& fileNode)
{
    if (fileNode.findPlug(MayaAttrs::file::uvTilingMode, true).asShort() != 0) {
        const TfToken ret {
            fileNode.findPlug(MayaAttrs::file::fileTextureNamePattern, true).asString().asChar()
        };
        return ret.IsEmpty()
            ? TfToken { fileNode.findPlug(MayaAttrs::file::computedFileTextureNamePattern, true)
                            .asString()
                            .asChar() }
            : ret;
    } else {
        const TfToken ret { MRenderUtil::exactFileTextureName(fileNode.object()).asChar() };
        return ret.IsEmpty() ? TfToken { fileNode.findPlug(MayaAttrs::file::fileTextureName, true)
                                             .asString()
                                             .asChar() }
                             : ret;
    }
}

/// This is the delimiter that Maya uses to identify levels of hierarchy in the
/// Maya DAG.
constexpr char MayaDagDelimiter[] = "|";

/// This is the delimiter that Maya uses to separate levels of namespace in
/// Maya node names.
constexpr char MayaNamespaceDelimiter[] = ":";

/// Strip \p nsDepth namespaces from \p nodeName.
///
/// This will turn "taco:foo:bar" into "foo:bar" for \p nsDepth == 1, or
/// "taco:foo:bar" into "bar" for \p nsDepth > 1.
/// If \p nsDepth is -1, all namespaces are stripped.
static std::string stripNamespaces(const std::string& nodeName, const int nsDepth = -1)
{
    if (nodeName.empty() || nsDepth == 0) {
        return nodeName;
    }

    std::stringstream ss;

    const std::vector<std::string> nodeNameParts = PXR_NS::TfStringSplit(nodeName, MayaDagDelimiter);

    const bool isAbsolute = PXR_NS::TfStringStartsWith(nodeName, MayaDagDelimiter);

    for (size_t i = 0u; i < nodeNameParts.size(); ++i) {
        if (i == 0u && isAbsolute) {
            // If nodeName was absolute, the first element in nodeNameParts
            // will be empty, so just skip it. The output path will be made
            // absolute with the next iteration.
            continue;
        }

        if (i != 0u) {
            ss << MayaDagDelimiter;
        }

        const std::vector<std::string> nsNameParts
            = PXR_NS::TfStringSplit(nodeNameParts[i], MayaNamespaceDelimiter);

        const size_t nodeNameIndex = nsNameParts.size() - 1u;

        auto startIter = nsNameParts.begin();
        if (nsDepth < 0) {
            // If nsDepth is negative, we don't keep any namespaces, so advance
            // startIter to the last element in the vector, which is just the
            // node name.
            startIter += nodeNameIndex;
        } else {
            // Otherwise we strip as many namespaces as possible up to nsDepth,
            // but no more than what would leave us with just the node name.
            startIter += std::min(static_cast<size_t>(nsDepth), nodeNameIndex);
        }

        ss << PXR_NS::TfStringJoin(startIter, nsNameParts.end(), MayaNamespaceDelimiter);
    }

    return ss.str();
}

std::string SanitizeName(const std::string& name) { return PXR_NS::TfStringReplace(name, ":", "_"); }

// XXX: see logic in UsdMayaTransformWriter.  It's unfortunate that this
// logic is in 2 places.  we should merge.
static bool _IsShape(const MDagPath& dagPath)
{
    if (dagPath.hasFn(MFn::kTransform)) {
        return false;
    }

    // go to the parent
    MDagPath parentDagPath = dagPath;
    parentDagPath.pop();
    if (!parentDagPath.hasFn(MFn::kTransform)) {
        return false;
    }

    unsigned int numberOfShapesDirectlyBelow = 0;
    parentDagPath.numberOfShapesDirectlyBelow(numberOfShapesDirectlyBelow);
    return (numberOfShapesDirectlyBelow == 1);
}

/// Converts the given Maya node name \p nodeName into an SdfPath.
///
/// Elements of the path will be sanitized such that it is a valid SdfPath.
/// This means it will replace Maya's namespace delimiter (':') with
/// underscores ('_').
SdfPath NodeNameToSdfPath(const std::string& nodeName, const bool doStripNamespaces)
{
    std::string pathString = nodeName;

    if (doStripNamespaces) {
        // Drop namespaces instead of making them part of the path.
        pathString = stripNamespaces(pathString);
    }

    std::replace(
        pathString.begin(),
        pathString.end(),
        MayaDagDelimiter[0],
        PXR_NS::SdfPathTokens->childDelimiter.GetString()[0]);
    std::replace(pathString.begin(), pathString.end(), MayaNamespaceDelimiter[0], '_');

    return SdfPath(pathString);
}

SdfPath DagPathToSdfPath(
    const MDagPath& dagPath,
    const bool      mergeTransformAndShape,
    const bool      stripNamespaces)
{
    SdfPath usdPath = NodeNameToSdfPath(dagPath.fullPathName().asChar(), stripNamespaces);

    if (mergeTransformAndShape && _IsShape(dagPath)) {
        usdPath = usdPath.GetParentPath();
    }

    return usdPath;
}

SdfPath RenderItemToSdfPath(
    const MRenderItem& ri,
    const bool         mergeTransformAndShape,
    const bool         stripNamespaces)
{
    // auto path = ri.sourceDagPath().fullPathName();
    return NodeNameToSdfPath(
        (ri.name() + std::to_string(ri.InternalObjectId()).c_str()).asChar(), stripNamespaces);
}
} // namespace MAYAHYDRA_NS_DEF
