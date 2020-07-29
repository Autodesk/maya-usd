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
#include "render_param.h"

#include <maya/MProfiler.h>

PXR_NAMESPACE_OPEN_SCOPE

/*! \brief  Begin update before rendering of VP2 starts.
 */
void HdVP2RenderParam::BeginUpdate(MSubSceneContainer& container, UsdTimeCode frame)
{
    _container = &container;
    _frame = frame;
}

/*! \brief  End update & clear access to render item container which pass this point won't be valid.
 */
void HdVP2RenderParam::EndUpdate() { _container = nullptr; }

PXR_NAMESPACE_CLOSE_SCOPE
