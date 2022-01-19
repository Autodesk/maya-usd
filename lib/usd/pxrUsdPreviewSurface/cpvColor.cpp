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

#include "cpvColor.h"

#include <maya/MFnNumericAttribute.h>

PXR_NAMESPACE_OPEN_SCOPE

static const MString cpvInputFragmentName("mayaCPVInput");

const MTypeId CPVColor::id(0x58000098);
MObject       CPVColor::aUVCoord;
MObject       CPVColor::aOutColor;
MObject       CPVColor::aOutAlpha;

void* CPVColor::creator() { return new CPVColor(); }

CPVColor::CPVColor() { }

CPVColor::~CPVColor() { }

MStatus CPVColor::initialize()
{
    // Define implicit shading network attributes
    MFnNumericAttribute nAttr;
    MObject             uCoord = nAttr.create("uCoord", "u", MFnNumericData::kFloat);
    MObject             vCoord = nAttr.create("vCoord", "v", MFnNumericData::kFloat);
    aUVCoord = nAttr.create("uvCoord", "uv", uCoord, vCoord);
    CHECK_MSTATUS(nAttr.setKeyable(true));
    CHECK_MSTATUS(nAttr.setStorable(true));
    CHECK_MSTATUS(nAttr.setReadable(true));
    CHECK_MSTATUS(nAttr.setWritable(true));
    CHECK_MSTATUS(nAttr.setHidden(true));
    aOutColor = nAttr.createColor("outColor", "oc");
    CHECK_MSTATUS(nAttr.setKeyable(false));
    CHECK_MSTATUS(nAttr.setStorable(false));
    CHECK_MSTATUS(nAttr.setReadable(true));
    CHECK_MSTATUS(nAttr.setWritable(false));
    aOutAlpha = nAttr.create("outAlpha", "oa", MFnNumericData::kFloat);
    CHECK_MSTATUS(nAttr.setKeyable(false));
    CHECK_MSTATUS(nAttr.setStorable(false));
    CHECK_MSTATUS(nAttr.setReadable(true));
    CHECK_MSTATUS(nAttr.setWritable(false));

    // Add attributes and setup attribute affecting relationships
    CHECK_MSTATUS(addAttribute(aUVCoord));
    CHECK_MSTATUS(addAttribute(aOutColor));
    CHECK_MSTATUS(addAttribute(aOutAlpha));
    CHECK_MSTATUS(attributeAffects(aUVCoord, aOutColor));
    CHECK_MSTATUS(attributeAffects(aUVCoord, aOutAlpha));

MPxNode::SchedulingType CPVColor::schedulingType() const { return SchedulingType::kParallel; }
    return MS::kSuccess;
}

MHWRender::MPxShadingNodeOverride* CPVColorShadingNodeOverride::creator(const MObject& obj)
{
    return new CPVColorShadingNodeOverride(obj);
}

CPVColorShadingNodeOverride::CPVColorShadingNodeOverride(const MObject& obj)
    : MPxShadingNodeOverride(obj)
{
}

CPVColorShadingNodeOverride::~CPVColorShadingNodeOverride() { }

MHWRender::DrawAPI CPVColorShadingNodeOverride::supportedDrawAPIs() const
{
    return MHWRender::kOpenGL | MHWRender::kDirectX11 | MHWRender::kOpenGLCoreProfile;
}

MString CPVColorShadingNodeOverride::fragmentName() const
{
    // Delegate the maya standard CPV input fragment to drive output
    return cpvInputFragmentName;
}

PXR_NAMESPACE_CLOSE_SCOPE
