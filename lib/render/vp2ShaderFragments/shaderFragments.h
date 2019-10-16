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

#ifndef HD_VP2_SHADER_FRAGMENTS
#define HD_VP2_SHADER_FRAGMENTS

#include "pxr/pxr.h"

#include <maya/MStatus.h>

PXR_NAMESPACE_OPEN_SCOPE

/*! \brief  
    \class  HdVP2ShaderFragments
*/
class HdVP2ShaderFragments
{
public:
    // Loads all fragments into VP2
    static MStatus registerFragments();

    // Unload all fragments from VP2
    static MStatus deregisterFragments();
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // HD_VP2_SHADER_FRAGMENTS
