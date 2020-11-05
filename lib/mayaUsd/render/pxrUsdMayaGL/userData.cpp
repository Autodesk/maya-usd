//
// Copyright 2018 Pixar
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
#include "userData.h"

#include <pxr/pxr.h>

#include <maya/MUserData.h>

PXR_NAMESPACE_OPEN_SCOPE

/// Note that we set deleteAfterUse=false when calling the MUserData
/// constructor. This ensures that the draw data survives across multiple draw
/// passes in Viewport 2.0 (e.g. a shadow pass and a color pass).
PxrMayaHdUserData::PxrMayaHdUserData()
    : MUserData(/* deleteAfterUse = */ false)
{
}

/* virtual */
PxrMayaHdUserData::~PxrMayaHdUserData() { }

PXR_NAMESPACE_CLOSE_SCOPE
