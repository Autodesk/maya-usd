//
// Copyright 2016 Pixar
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

#if defined(MAYAUSD_VERSION)
    #error This file should not be included when MAYAUSD_VERSION is defined
#endif

#include <mayaHydraLib/usd/util.h>
#include <mayaHydraLib/utils.h>

#include <maya/MHWGeometry.h>

#include <sstream>

#include <pxr/base/tf/stringUtils.h>

PXR_NAMESPACE_USING_DIRECTIVE

std::string UsdMayaUtil::stripNamespaces(const std::string& nodeName, const int nsDepth)
{
    if (nodeName.empty() || nsDepth == 0) {
        return nodeName;
    }

    std::stringstream ss;

    const std::vector<std::string> nodeNameParts
        = TfStringSplit(nodeName, UsdMayaUtil::MayaDagDelimiter);

    const bool isAbsolute = TfStringStartsWith(nodeName, UsdMayaUtil::MayaDagDelimiter);

    for (size_t i = 0u; i < nodeNameParts.size(); ++i) {
        if (i == 0u && isAbsolute) {
            // If nodeName was absolute, the first element in nodeNameParts
            // will be empty, so just skip it. The output path will be made
            // absolute with the next iteration.
            continue;
        }

        if (i != 0u) {
            ss << UsdMayaUtil::MayaDagDelimiter;
        }

        const std::vector<std::string> nsNameParts
            = TfStringSplit(nodeNameParts[i], UsdMayaUtil::MayaNamespaceDelimiter);

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

        ss << TfStringJoin(
            startIter, nsNameParts.end(), UsdMayaUtil::MayaNamespaceDelimiter.c_str());
    }

    return ss.str();
}

std::string UsdMayaUtil::SanitizeName(const std::string& name)
{
    return TfStringReplace(name, ":", "_");
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

SdfPath UsdMayaUtil::MayaNodeNameToSdfPath(const std::string& nodeName, const bool stripNamespaces)
{
    std::string pathString = nodeName;

    if (stripNamespaces) {
        // Drop namespaces instead of making them part of the path.
        pathString = UsdMayaUtil::stripNamespaces(pathString);
    }

    std::replace(
        pathString.begin(),
        pathString.end(),
        UsdMayaUtil::MayaDagDelimiter[0],
        SdfPathTokens->childDelimiter.GetString()[0]);
    std::replace(pathString.begin(), pathString.end(), UsdMayaUtil::MayaNamespaceDelimiter[0], '_');

    return SdfPath(pathString);
}

SdfPath UsdMayaUtil::MDagPathToUsdPath(
    const MDagPath& dagPath,
    const bool      mergeTransformAndShape,
    const bool      stripNamespaces)
{
    SdfPath usdPath
        = UsdMayaUtil::MayaNodeNameToSdfPath(dagPath.fullPathName().asChar(), stripNamespaces);

    if (mergeTransformAndShape && _IsShape(dagPath)) {
        usdPath = usdPath.GetParentPath();
    }

    return usdPath;
}

SdfPath UsdMayaUtil::RenderItemToUsdPath(
	const MRenderItem& ri,
	const bool      mergeTransformAndShape,
	const bool      stripNamespaces)
{	
	//auto path = ri.sourceDagPath().fullPathName();
	return UsdMayaUtil::MayaNodeNameToSdfPath(		
		(ri.name() + std::to_string(ri.InternalObjectId()).c_str()).asChar(),
		stripNamespaces);
}
