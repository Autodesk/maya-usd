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
#include "UsdUIInfoHandler.h"
#include "UsdSceneItem.h"

#include <maya/MDoubleArray.h>
#include <maya/MGlobal.h>

#include <map>

MAYAUSD_NS_DEF {
namespace ufe {

UsdUIInfoHandler::UsdUIInfoHandler()
	: Ufe::UIInfoHandler()
{}

UsdUIInfoHandler::~UsdUIInfoHandler()
{
}

/*static*/
UsdUIInfoHandler::Ptr UsdUIInfoHandler::create()
{
	return std::make_shared<UsdUIInfoHandler>();
}

//------------------------------------------------------------------------------
// Ufe::UIInfoHandler overrides
//------------------------------------------------------------------------------

bool UsdUIInfoHandler::treeViewCellInfo(const Ufe::SceneItem::Ptr& item, Ufe::CellInfo& info) const
{
	bool changed = false;
	UsdSceneItem::Ptr usdItem = std::dynamic_pointer_cast<UsdSceneItem>(item);
#if !defined(NDEBUG)
	assert(usdItem);
#endif
	if (usdItem)
	{
		if (!usdItem->prim().IsActive())
		{
			changed = true;
			info.fontStrikeout = true;
			MDoubleArray outlinerInvisibleColor;
			if (MGlobal::executeCommand("displayRGBColor -q \"outlinerInvisibleColor\"", outlinerInvisibleColor) &&
				(outlinerInvisibleColor.length() == 3))
			{
				double rgb[3];
				outlinerInvisibleColor.get(rgb);
				info.textFgColor.set(static_cast<float>(rgb[0]), static_cast<float>(rgb[1]), static_cast<float>(rgb[2]));
			}
			else
			{
				info.textFgColor.set(0.403922f, 0.403922f, 0.403922f);
			}
		}
	}

	return changed;
}

std::string UsdUIInfoHandler::treeViewIcon(const Ufe::SceneItem::Ptr& item) const
{
	// Special case for nullptr input.
	if (!item) {
		return "out_USD_UsdTyped.png";	// Default USD icon
	}

	// We support these node types directly.
	static const std::map<std::string, std::string> supportedTypes{
		{"",					"out_USD_Def.png"},					// No node type
		{"BlendShape",			"out_USD_BlendShape.png"},
		{"Camera",				"out_USD_Camera.png"},
		{"Capsule",				"out_USD_Capsule.png"},
		{"Cone",				"out_USD_Cone.png"},
		{"Cube",				"out_USD_Cube.png"},
		{"Cylinder",			"out_USD_Cylinder.png"},
		{"GeomSubset",			"out_USD_GeomSubset.png"},
		{"LightFilter",			"out_USD_LightFilter.png"},
		{"LightPortal",			"out_USD_LightPortal.png"},
		{"mayaReference",		"out_USD_mayaReference.png"},
		{"AL_MayaReference",	"out_USD_mayaReference.png"},		// Same as mayaRef
		{"Mesh",				"out_USD_Mesh.png"},
		{"NurbsPatch",			"out_USD_NurbsPatch.png"},
		{"PointInstancer",		"out_USD_PointInstancer.png"},
		{"Points",				"out_USD_Points.png"},
		{"Scope",				"out_USD_Scope.png"},
		{"SkelAnimation",		"out_USD_SkelAnimation.png"},
		{"Skeleton",			"out_USD_Skeleton.png"},
		{"SkelRoot",			"out_USD_SkelRoot.png"},
		{"Sphere",				"out_USD_Sphere.png"},
		{"Volume",				"out_USD_Volume.png"}
	};

	const auto search = supportedTypes.find(item->nodeType());
	if (search != supportedTypes.cend()) {
		return search->second;
	}

	// No specific node type icon was found.
	return "";
}

std::string UsdUIInfoHandler::getLongRunTimeLabel() const
{
	return "Universal Scene Description";
}


} // namespace ufe
} // namespace MayaUsd
