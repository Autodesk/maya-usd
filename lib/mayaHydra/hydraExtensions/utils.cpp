//
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
#include "utils.h"

#include <pxr/base/gf/quath.h>
#include <pxr/base/gf/vec2f.h>
#include <pxr/base/tf/stringUtils.h>
#include <pxr/base/tf/token.h>
#include <pxr/base/vt/array.h>
#include <pxr/pxr.h>
#include <pxr/usd/sdf/assetPath.h>
#include <pxr/usd/sdf/path.h>

#include <maya/MFnDependencyNode.h>
#include <maya/MHWGeometry.h>
#include <maya/MObject.h>
#include <maya/MPlugArray.h>
#include <maya/MStatus.h>
#include <ufe/runTimeMgr.h>

#include <cctype>
#include <sstream>

namespace MAYAHYDRA_NS_DEF {

namespace MayaAttrs = PXR_NS::MayaAttrs;

PXR_NAMESPACE_USING_DIRECTIVE

/*  Return in the std::string outValueAsString the VtValue type and value
 *  written as text for debugging purpose.
 */
void ConvertVtValueAsText(const VtValue& val, std::string& outValueAsString)
{
    if (val.IsEmpty()) {
        outValueAsString = "No Value!";
        return;
    }

    std::ostringstream ss;
    if (val.IsHolding<bool>()) {
        const bool v = val.UncheckedGet<bool>();
        ss << "bool : ";
        ss << v;
    } else if (val.IsHolding<TfToken>()) {
        const TfToken elem = val.UncheckedGet<TfToken>();
        ss << "TfToken : " << elem.GetText();
    } else if (val.IsHolding<VtArray<int>>()) {
        auto arrayType = val.UncheckedGet<VtArray<int>>();
        ss << "VtArray<int> : (";
        for (auto elem : arrayType) {
            ss << std::to_string(elem) << " , ";
        }
        ss << ")";
    } else if (val.IsHolding<VtArray<float>>()) {
        auto arrayType = val.UncheckedGet<VtArray<float>>();
        ss << "VtArray<float> : (";
        for (auto elem : arrayType) {
            ss << std::to_string(elem) << " , ";
        }
        ss << ")";
    } else if (val.IsHolding<float>()) {
        const float v = val.UncheckedGet<float>();
        ss << "float : ";
        ss << v;
    } else if (val.IsHolding<int>()) {
        const int v = val.UncheckedGet<int>();
        ss << "int : ";
        ss << v;
    } else if (val.IsHolding<GfVec2f>()) {
        const GfVec2f v = val.UncheckedGet<GfVec2f>();
        ss << "GfVec2f : (" << v[0] << " , " << v[1] << ")";
    } else if (val.IsHolding<GfVec3f>()) {
        const GfVec3f v = val.UncheckedGet<GfVec3f>();
        ss << "GfVec3f : (" << v[0] << " , " << v[1] << " , " << v[2] << ")";
    } else if (val.IsHolding<GfVec3d>()) {
        const auto v = val.UncheckedGet<GfVec3d>();
        ss << "GfVec3d : (" << v[0] << " , " << v[1] << " , " << v[2] << ")";
    } else if (val.IsHolding<SdfAssetPath>()) {
        const SdfAssetPath v = val.UncheckedGet<SdfAssetPath>();
        const std::string  assetPath = v.GetAssetPath();
        ss << "SdfAssetPath : \"" << assetPath << "\"";
    } else if (val.IsHolding<VtArray<SdfPath>>()) {
        auto arrayType = val.UncheckedGet<VtArray<SdfPath>>();
        ss << "VtArray<SdfPath> : (";
        for (auto elem : arrayType) {
            ss << elem.GetText() << " , ";
        }
        ss << ")";
    } else if (val.IsHolding<VtArray<GfVec3f>>()) {
        auto arrayType = val.UncheckedGet<VtArray<GfVec3f>>();
        ss << "VtArray<GfVec3f> : (";
        for (auto elem : arrayType) {
            auto strVec3f = "(" + std::to_string(elem[0]) + ", " + std::to_string(elem[1]) + ", "
                + std::to_string(elem[2]) + ")";
            ss << strVec3f + " , ";
        }
        ss << ")";
    } else if (val.IsHolding<VtArray<GfVec3d>>()) {
        auto arrayType = val.UncheckedGet<VtArray<GfVec3d>>();
        ss << "VtArray<GfVec3d> : (";
        for (auto elem : arrayType) {
            auto strVec3f = "(" + std::to_string(elem[0]) + ", " + std::to_string(elem[1]) + ", "
                + std::to_string(elem[2]) + ")";
            ss << strVec3f + " , ";
        }
        ss << ")";
    } else if (val.IsHolding<VtArray<GfQuath>>()) {
        auto arrayType = val.UncheckedGet<VtArray<GfQuath>>();
        ss << "VtArray<GfQuath> : (";
        for (auto elem : arrayType) {
            auto quathh = "(" + std::to_string(elem.GetReal()) + ", "
                + std::to_string(elem.GetImaginary()[0]) + ", "
                + std::to_string(elem.GetImaginary()[1]) + ", "
                + std::to_string(elem.GetImaginary()[2]) + ")";
            ss << quathh + " , ";
        }
        ss << ")";
    } else if (val.IsHolding<GfQuath>()) {
        auto elem = val.UncheckedGet<GfQuath>();
        auto quathh = "(" + std::to_string(elem.GetReal()) + ", "
            + std::to_string(elem.GetImaginary()[0]) + ", " + std::to_string(elem.GetImaginary()[1])
            + ", " + std::to_string(elem.GetImaginary()[2]) + ")";
        ss << "GfQuath : " << quathh;
    } else if (val.IsHolding<GfMatrix4d>()) {
        auto mat4d = val.UncheckedGet<GfMatrix4d>();

        double data[4][4];
        mat4d.Get(data);
        auto strMat4d = std::string("(") + "{" + std::to_string(data[0][0]) + ", "
            + std::to_string(data[0][1]) + ", " + std::to_string(data[0][2]) + ", "
            + std::to_string(data[0][3]) + "}, " + "{" + std::to_string(data[1][0]) + ", "
            + std::to_string(data[1][1]) + ", " + std::to_string(data[1][2]) + ", "
            + std::to_string(data[1][3]) + "}, " + "{" + std::to_string(data[2][0]) + ", "
            + std::to_string(data[2][1]) + ", " + std::to_string(data[2][2]) + ", "
            + std::to_string(data[2][3]) + "}, " + "{" + std::to_string(data[3][0]) + ", "
            + std::to_string(data[3][1]) + ", " + std::to_string(data[3][2]) + ", "
            + std::to_string(data[3][3]) + "}" + ")";
        ss << "GfMatrix4d : " << strMat4d;
    }

    outValueAsString = ss.str();
    if (outValueAsString.size() > 0) {
        return;
    }

    // Unknown
    outValueAsString = " * Unknown Type *";
}

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

// This is the delimiter that Maya uses to identify levels of hierarchy in the
// Maya DAG.
constexpr char MayaDagDelimiter[] = "|";

// This is the delimiter that Maya uses to separate levels of namespace in
// Maya node names.
constexpr char MayaNamespaceDelimiter[] = ":";

// Strip \p nsDepth namespaces from \p nodeName.
//
// This will turn "taco:foo:bar" into "foo:bar" for \p nsDepth == 1, or
// "taco:foo:bar" into "bar" for \p nsDepth > 1.
// If \p nsDepth is -1, all namespaces are stripped.
static std::string stripNamespaces(const std::string& nodeName, const int nsDepth = -1)
{
    if (nodeName.empty() || nsDepth == 0) {
        return nodeName;
    }

    std::stringstream ss;

    const std::vector<std::string> nodeNameParts
        = PXR_NS::TfStringSplit(nodeName, MayaDagDelimiter);

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

std::string SanitizeName(const std::string& name)
{
    return PXR_NS::TfStringReplace(name, ":", "_");
}

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

// Converts the given Maya node name \p nodeName into an SdfPath.
//
// Elements of the path will be sanitized such that it is a valid SdfPath.
// This means it will replace Maya's namespace delimiter (':') with
// underscores ('_').
// A SdfPath in Pixar USD is considered invalid if it does not conform to the rules for path names.
// Some common issues that can make a path invalid include: Starting with a number : Path names
// must start with a letter, not a number. Including spaces or special characters : Path names can
// only contain letters, numbers, and the characters _, -, and : .
const std::string& SanitizeNameForSdfPath(std::string& pathString, const bool doStripNamespaces)
{
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
    std::replace(pathString.begin(), pathString.end(), ',', '_');
    std::replace(pathString.begin(), pathString.end(), ';', '_');

    return pathString;
}

SdfPath DagPathToSdfPath(
    const MDagPath& dagPath,
    const bool      mergeTransformAndShape,
    const bool      stripNamespaces)
{
    std::string name = dagPath.fullPathName().asChar();
    SdfPath     usdPath(SanitizeNameForSdfPath(name, stripNamespaces));

    if (mergeTransformAndShape && _IsShape(dagPath)) {
        usdPath = usdPath.GetParentPath();
    }

    return usdPath;
}

SdfPath RenderItemToSdfPath(
    const MRenderItem& ri,
    const bool         stripNamespaces)
{
    std::string internalObjectId(
        "_" + std::to_string(ri.InternalObjectId())); // preventively prepend item id by underscore
    std::string name(ri.name().asChar() + internalObjectId);
    // Try to sanitize maya path to be used as an sdf path.
    SanitizeNameForSdfPath(name, stripNamespaces);
    // Path names must start with a letter, not a number
    // If a number is found, prepend the path with an underscore
    char digit = name[0];
    if (std::isdigit(digit)) {
        name.insert(0, "_");
    }

    SdfPath sdfPath(name);
    if (!TF_VERIFY(
            !sdfPath.IsEmpty(),
            "Render item using invalid SdfPath '%s'. Using item's id instead.",
            name.c_str())) {
        // If failed to include render item's name as an SdfPath simply use the item id.
        return SdfPath(internalObjectId);
    }
    return sdfPath;
}

} // namespace MAYAHYDRA_NS_DEF
