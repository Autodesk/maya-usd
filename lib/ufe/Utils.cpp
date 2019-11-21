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
#include "private/Utils.h"
#include "UsdStageMap.h"
#include "ProxyShapeHandler.h"

#include "pxr/base/tf/hashset.h"
#include "pxr/usd/usd/stage.h"

#include "maya/MGlobal.h"
#include "maya/MSelectionList.h"

#include <cassert>
#include <string>
#include <unordered_map>
#include <regex>
#include <memory>
#include <stdexcept>

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

SdfLayerHandle defPrimSpecLayer(const UsdPrim& prim)
{
	// Iterate over the layer stack, starting at the highest-priority layer.
	// The source layer is the one in which there exists a def primSpec, not
	// an over.

	SdfLayerHandle defLayer;
	auto layerStack = prim.GetStage()->GetLayerStack();
	for (auto layer : layerStack)
	{
		auto primSpec = layer->GetPrimAtPath(prim.GetPath());
		if (primSpec && (primSpec->GetSpecifier() == SdfSpecifierDef))
		{
			defLayer = layer;
			break;
		}
	}
	return defLayer;
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

std::string uniqueChildName(const Ufe::SceneItem::Ptr& parent, const Ufe::Path& childPath)
{
	auto usdParent = std::dynamic_pointer_cast<UsdSceneItem>(parent);
#if !defined(NDEBUG)
	assert(usdParent);
#endif
	if (!usdParent) return "";

	TfToken::HashSet childrenNames;
	for (auto child : usdParent->prim().GetChildren())
	{
		childrenNames.insert(child.GetName());
	}
	std::string childName = childPath.back().string();
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

MDagPath nameToDagPath(const std::string& name)
{
	MSelectionList selection;
	selection.add(MString(name.c_str()));
	MDagPath dag;
	MStatus status = selection.getDagPath(0, dag);
	CHECK_MSTATUS(status);
	return dag;
}

} // namespace ufe
} // namespace MayaUsd
