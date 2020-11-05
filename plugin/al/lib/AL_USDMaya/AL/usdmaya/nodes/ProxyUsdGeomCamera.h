//
// Copyright 2017 Animal Logic
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
#pragma once

#include "AL/usdmaya/Api.h"

#include <AL/maya/utils/MayaHelperMacros.h>
#include <AL/maya/utils/NodeHelper.h>

#include <pxr/usd/usdGeom/camera.h>

#include <maya/MPxNode.h>

PXR_NAMESPACE_USING_DIRECTIVE

namespace AL {
namespace usdmaya {
namespace nodes {

//----------------------------------------------------------------------------------------------------------------------
/// \brief  The camera proxy node defines all attributes of the UsdGeomCamera as Maya attributes and
/// allows reading and
///         writing directly to those attributes. This node can also be connected to the attributes
///         of a Maya camera to drive its attributes.
/// \ingroup nodes
//----------------------------------------------------------------------------------------------------------------------
class ProxyUsdGeomCamera
    : public MPxNode
    , public maya::utils::NodeHelper
{
public:
    /// \brief The enumeration values of the UsdGeomCamera projection attribute.
    enum class Projection
    {
        Perspective,
        Orthographic,
    };

    /// \brief The enumeration values of the UsdGeomCamera stereoRole attribute.
    enum class StereoRole
    {
        Mono,
        Left,
        Right,
    };

public:
    AL_MAYA_DECLARE_NODE();

protected:
    /// \brief Returns the UsdGeomCamera this node is proxying.
    AL_USDMAYA_PUBLIC
    UsdGeomCamera getCamera() const;

    /// \brief Returns the UsdTimeCode at which attributes are being accessed.
    AL_USDMAYA_PUBLIC
    UsdTimeCode getTime() const;

public:
    /// \brief  ctor
    AL_USDMAYA_PUBLIC
    ProxyUsdGeomCamera() = default;

    /// \brief  dtor
    AL_USDMAYA_PUBLIC
    ~ProxyUsdGeomCamera() = default;

    /// \brief Writes attribute values from Maya to the UsdGeomCamera this node is proxying.
    AL_USDMAYA_PUBLIC
    bool setInternalValue(const MPlug& plug, const MDataHandle& dataHandle) override;

    /// \brief Reads attributes from the UsdGeomCamera this node is proxying when requested by Maya.
    AL_USDMAYA_PUBLIC
    bool getInternalValue(const MPlug& plug, MDataHandle& dataHandle) override;

    AL_DECL_ATTRIBUTE(path);
    AL_DECL_ATTRIBUTE(stage);
    AL_DECL_ATTRIBUTE(time);

    // Schema
    AL_DECL_ATTRIBUTE(clippingRange);
    AL_DECL_ATTRIBUTE(focalLength);
    AL_DECL_ATTRIBUTE(focusDistance);
    AL_DECL_ATTRIBUTE(fStop);
    AL_DECL_ATTRIBUTE(horizontalAperture);       // mm
    AL_DECL_ATTRIBUTE(verticalAperture);         // mm
    AL_DECL_ATTRIBUTE(filmOffset);               // inch
    AL_DECL_ATTRIBUTE(horizontalApertureOffset); // mm
    AL_DECL_ATTRIBUTE(verticalApertureOffset);   // mm
    AL_DECL_ATTRIBUTE(projection);
    AL_DECL_ATTRIBUTE(shutterClose);
    AL_DECL_ATTRIBUTE(shutterOpen);
    AL_DECL_ATTRIBUTE(stereoRole);

    // Maya
    AL_DECL_ATTRIBUTE(nearClipPlane);
    AL_DECL_ATTRIBUTE(farClipPlane);
    AL_DECL_ATTRIBUTE(orthographic);
    AL_DECL_ATTRIBUTE(cameraApertureMm);       // mm
    AL_DECL_ATTRIBUTE(cameraApertureInch);     // inch
    AL_DECL_ATTRIBUTE(horizontalFilmAperture); // inch
    AL_DECL_ATTRIBUTE(horizontalFilmOffset);   // inch
    AL_DECL_ATTRIBUTE(verticalFilmAperture);   // inch
    AL_DECL_ATTRIBUTE(verticalFilmOffset);     // inch
};

//----------------------------------------------------------------------------------------------------------------------
} // namespace nodes
} // namespace usdmaya
} // namespace AL
//----------------------------------------------------------------------------------------------------------------------
