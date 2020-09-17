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

std::string UsdUIInfoHandler::getLongRunTimeLabel() const
{
	return "Universal Scene Description";
}


} // namespace ufe
} // namespace MayaUsd
