//
// Copyright 2019 Autodesk
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
#include "Utils.h"

#include <cassert>
#include <memory>
#include <regex>
#include <stdexcept>
#include <string>
#include <unordered_map>

#include <maya/MGlobal.h>

#include <maya/MObjectHandle.h>
#include <maya/MFnDependencyNode.h>

#include <pxr/base/tf/hashset.h>
#include <pxr/usd/usd/stage.h>

#include <mayaUsd/nodes/proxyShapeBase.h>
#include <mayaUsd/ufe/ProxyShapeHandler.h>
#include <mayaUsd/ufe/UsdStageMap.h>
#include <mayaUsd/utils/util.h>

#include <ufe/pathSegment.h>

#include "private/Utils.h"

PXR_NAMESPACE_USING_DIRECTIVE

#ifndef MAYA_MSTRINGARRAY_ITERATOR_CATEGORY
// MStringArray::Iterator is not standard-compliant in Maya 2019, needs the
// following workaround.  Fixed in Maya 2020.  PPT, 20-Jun-2019.
namespace std
{
	template<>
	struct iterator_traits<MStringArray::Iterator> {
		typedef std::bidirectional_iterator_tag iterator_category;
	};
}
#endif

MAYAUSD_NS_DEF {
namespace ufe {

//------------------------------------------------------------------------------
// Global variables & macros
//------------------------------------------------------------------------------

extern UsdStageMap g_StageMap;
extern Ufe::Rtid g_MayaRtid;

// Cache of Maya node types we've queried before for inheritance from the
// gateway node type.
std::unordered_map<std::string, bool> g_GatewayType;

//------------------------------------------------------------------------------
// Utility Functions
//------------------------------------------------------------------------------

UsdStageWeakPtr getStage(const Ufe::Path& path)
{
	return g_StageMap.stage(path);
}

Ufe::Path stagePath(UsdStageWeakPtr stage)
{
	return g_StageMap.path(stage);
}

UsdPrim ufePathToPrim(const Ufe::Path& path)
{
	// Assume that there are only two segments in the path, the first a Maya
	// Dag path segment to the proxy shape, which identifies the stage, and
	// the second the USD segment.
	const Ufe::Path::Segments& segments = path.getSegments();
	TEST_USD_PATH(segments, path);

	UsdPrim prim;
	if (auto stage = getStage(Ufe::Path(segments[0])))
	{
		prim = stage->GetPrimAtPath(SdfPath(segments[1].string()));
	}
	return prim;
}

bool isRootChild(const Ufe::Path& path)
{
	auto segments = path.getSegments();
	TEST_USD_PATH(segments, path);
	return(segments[1].size() == 1);
}

UsdSceneItem::Ptr createSiblingSceneItem(const Ufe::Path& ufeSrcPath, const std::string& siblingName)
{
	auto ufeSiblingPath = ufeSrcPath.sibling(Ufe::PathComponent(siblingName));
	return UsdSceneItem::create(ufeSiblingPath, ufePathToPrim(ufeSiblingPath));
}

std::string uniqueName(const TfToken::HashSet& existingNames, std::string srcName)
{
	// Compiled regular expression to find a numerical suffix to a path component.
	// It searches for any number of characters followed by a single non-numeric,
	// then one or more digits at end of string.
	std::regex re("(.*)([^0-9])([0-9]+)$");
	std::string base{srcName};
	int suffix{1};
	std::smatch match;
	if (std::regex_match(srcName, match, re))
	{
		base = match[1].str() + match[2].str();
		suffix = std::stoi(match[3].str()) + 1;
	}
	std::string dstName = base + std::to_string(suffix);
	while (existingNames.count(TfToken(dstName)) > 0)
	{
		dstName = base + std::to_string(++suffix);
	}
	return dstName;
}

std::string uniqueChildName(const UsdPrim& usdParent, const std::string& name)
{
	if (!usdParent.IsValid()) return std::string();

	TfToken::HashSet childrenNames;

	// The prim GetChildren method used the UsdPrimDefaultPredicate which includes
	// active prims. We also need the inactive ones.
	//
	// const Usd_PrimFlagsConjunction UsdPrimDefaultPredicate = 
	//			UsdPrimIsActive && UsdPrimIsDefined && 
	//			UsdPrimIsLoaded && !UsdPrimIsAbstract;
	// Note: removed 'UsdPrimIsLoaded' from the predicate. When it is present the
	//		 filter doesn't properly return the inactive prims. UsdView doesn't
	//		 use loaded either in _computeDisplayPredicate().
	//
	// Note: our UsdHierarchy uses instance proxies, so we also use them here.
	for (auto child : usdParent.GetFilteredChildren(UsdTraverseInstanceProxies(UsdPrimIsDefined && !UsdPrimIsAbstract)))
	{
		childrenNames.insert(child.GetName());
	}
	std::string childName{name};
	if (childrenNames.find(TfToken(childName)) != childrenNames.end())
	{
		childName = uniqueName(childrenNames, childName);
	}
	return childName;
}

bool isAGatewayType(const std::string& mayaNodeType)
{
	// If we've seen this node type before, return the cached value.
	auto iter = g_GatewayType.find(mayaNodeType);
	if (iter != std::end(g_GatewayType))
	{
		return iter->second;
	}

	// Note: we are calling the MEL interpreter to determine the inherited types,
	//		 but we are then caching the result. So MEL will only be called once
	//		 for each node type.
	// Not seen before, so ask Maya.
	// When the inherited flag is used, the command returns a string array containing
	// the names of all the base node types inherited by the the given node.
	MString cmd;
	MStringArray inherited;
	bool isInherited = false;
	cmd.format("nodeType -inherited -isTypeName ^1s", mayaNodeType.c_str());
	if (MS::kSuccess == MGlobal::executeCommand(cmd, inherited))
	{
		MString gatewayNodeType(ProxyShapeHandler::gatewayNodeType().c_str());
		auto iter2 = std::find(inherited.begin(), inherited.end(), gatewayNodeType);
		isInherited = (iter2 != inherited.end());
		g_GatewayType[mayaNodeType] = isInherited;
	}
	return isInherited;
}

Ufe::Path dagPathToUfe(const MDagPath& dagPath)
{
	// This function can only create UFE Maya scene items with a single
	// segment, as it is only given a Dag path as input.
	return Ufe::Path(dagPathToPathSegment(dagPath));
}

Ufe::PathSegment dagPathToPathSegment(const MDagPath& dagPath)
{
	std::string fullPathName = dagPath.fullPathName().asChar();
	return Ufe::PathSegment("world" + fullPathName, g_MayaRtid, '|');
}

UsdTimeCode getTime(const Ufe::Path& path)
{
    // Path should not be empty.
    if (!TF_VERIFY(!path.empty())) {
        return UsdTimeCode::Default();
    }

    // Get the time from the proxy shape.  This will be the tail component of
    // the first path segment.
    auto proxyShapePath = Ufe::Path(path.getSegments()[0]);

    // Keep a single-element path to MObject cache, as all USD prims in a stage
    // share the same proxy shape.
    static std::pair<Ufe::Path, MObjectHandle> cache;

    MObject proxyShapeObj;

    if (cache.first == proxyShapePath && cache.second.isValid()) {
        proxyShapeObj = cache.second.object();
    }
    else {
        // Not found in the cache, or no longer valid.  Get the proxy shape
        // MObject from its path, and put it in the cache.  Pop the head of the
        // UFE path to get rid of "|world", which is implicit in Maya.
        auto proxyShapeDagPath = UsdMayaUtil::nameToDagPath(
            proxyShapePath.popHead().string());
        TF_VERIFY(proxyShapeDagPath.isValid());
        proxyShapeObj = proxyShapeDagPath.node();
        cache = std::pair<Ufe::Path, MObjectHandle>(
            proxyShapePath, MObjectHandle(proxyShapeObj));
    }
    
    // Get time from the proxy shape.
    MFnDependencyNode fn(proxyShapeObj);
    auto proxyShape = dynamic_cast<MayaUsdProxyShapeBase*>(fn.userNode());
    TF_VERIFY(proxyShape);

    return proxyShape->getTime();
}

} // namespace ufe
} // namespace MayaUsd
