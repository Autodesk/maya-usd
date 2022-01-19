//
// Copyright 2021 Animal Logic
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

#ifndef PXRUSDPREVIEWSURFACE_CPVCOLOR_H
#define PXRUSDPREVIEWSURFACE_CPVCOLOR_H

#include "api.h"

#include <pxr/pxr.h>

#include <maya/MPxNode.h>
#include <maya/MPxShadingNodeOverride.h>

PXR_NAMESPACE_OPEN_SCOPE

class CPVColor : public MPxNode
{
public:
    PXRUSDPREVIEWSURFACE_API
    static void* creator();

    PXRUSDPREVIEWSURFACE_API
    CPVColor();

    PXRUSDPREVIEWSURFACE_API
    ~CPVColor() override;

    PXRUSDPREVIEWSURFACE_API
    static MStatus initialize();

    PXRUSDPREVIEWSURFACE_API
    SchedulingType schedulingType() const override;

    PXRUSDPREVIEWSURFACE_API
    static MStatus initialize();

    PXRUSDPREVIEWSURFACE_API
    static const MTypeId id;

private:
    static MObject aUVCoord;

    static MObject aOutColor;

    static MObject aOutAlpha;
};

class CPVColorShadingNodeOverride : public MHWRender::MPxShadingNodeOverride
{
public:
    PXRUSDPREVIEWSURFACE_API
    static MHWRender::MPxShadingNodeOverride* creator(const MObject& obj);

    PXRUSDPREVIEWSURFACE_API
    ~CPVColorShadingNodeOverride() override;

    PXRUSDPREVIEWSURFACE_API
    MHWRender::DrawAPI supportedDrawAPIs() const override;

    PXRUSDPREVIEWSURFACE_API
    MString fragmentName() const override;

private:
    CPVColorShadingNodeOverride(const MObject& obj);
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
