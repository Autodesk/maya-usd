//
// Copyright 2026 Autodesk
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
#ifndef USDUFE_MATERIALUTILS_H
#define USDUFE_MATERIALUTILS_H

#include <usdUfe/base/api.h>

#include <ufe/path.h>
#include <ufe/sceneItem.h>

#include <string>
#include <vector>

namespace USDUFE_NS_DEF {

//! \brief A creatable surface shader entry for material assignment menus.
struct USDUFE_PUBLIC RendererMaterialMenuEntry
{
    std::string renderer;
    std::string label;
    std::string identifier;
};

//! \brief Returns creatable surface shader types grouped by renderer/source type.
USDUFE_PUBLIC
std::vector<RendererMaterialMenuEntry> getMaterialsFromRenderers();

//! \brief Returns Sdf paths of all UsdShadeMaterial prims in the stage containing \p contextPath.
USDUFE_PUBLIC
std::vector<std::string> getMaterialsInStage(const Ufe::Path& contextPath);

//! \brief Returns whether material assignment menus should be shown for \p sceneItem.
USDUFE_PUBLIC
bool canAssignMaterialToNodeType(const Ufe::SceneItem::Ptr& sceneItem);

} // namespace USDUFE_NS_DEF

#endif // USDUFE_MATERIALUTILS_H
