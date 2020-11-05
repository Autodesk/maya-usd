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

#include <mayaUsd/base/api.h>

#include <pxr/base/tf/staticTokens.h>
#include <pxr/pxr.h>

#include <maya/MStatus.h>

PXR_NAMESPACE_OPEN_SCOPE

#define MAYAUSD_CORE_PUBLIC_USD_PREVIEW_SURFACE_TOKENS \
    ((CoreFragmentGraphName, "UsdPreviewSurfaceCore")) \
    ((SurfaceFragmentGraphName, "UsdPreviewSurface"))

TF_DECLARE_PUBLIC_TOKENS(
    HdVP2ShaderFragmentsTokens,
    MAYAUSD_CORE_PUBLIC,
    MAYAUSD_CORE_PUBLIC_USD_PREVIEW_SURFACE_TOKENS);

/*! \brief  Registration/deregistration of HdVP2 shader fragments.
    \class  HdVP2ShaderFragments
*/
class HdVP2ShaderFragments
{
public:
    //! Register all HdVP2 fragments
    MAYAUSD_CORE_PUBLIC
    static MStatus registerFragments();

    //! Deregister all HdVP2 fragments
    MAYAUSD_CORE_PUBLIC
    static MStatus deregisterFragments();
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // HD_VP2_SHADER_FRAGMENTS
