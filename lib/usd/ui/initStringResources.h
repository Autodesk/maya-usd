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
#ifndef INIT_STRING_RESOURCES_H
#define INIT_STRING_RESOURCES_H

#include <mayaUsd/mayaUsd.h>
#include <mayaUsdUI/ui/api.h>

#include <maya/MStatus.h>

namespace MAYAUSD_NS_DEF {

// register all string
MAYAUSD_UI_PUBLIC MStatus initStringResources();

} // namespace MAYAUSD_NS_DEF

#endif // INIT_STRING_RESOURCES_H
