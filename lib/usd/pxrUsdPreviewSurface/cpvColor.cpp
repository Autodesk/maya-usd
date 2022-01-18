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

const MString CPVColor::name("cpvColor");
const MString CPVColor::userClassification("utility/color:");
const MString CPVColor::drawClassification("drawdb/shader/utility/color/");

const MTypeId CPVColor::id(0x58000098);
MObject       CPVColor::aOutColor;
MObject       CPVColor::aOutAlpha;
MObject       CPVColor::aOutOpacity;

void* CPVColor::creator() { return new CPVColor(); }

CPVColor::CPVColor() { }

CPVColor::~CPVColor() { }

MStatus CPVColor::initialize()
{
    // Define implicit shading network attributes
    MFnNumericAttribute nAttr;
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
    CHECK_MSTATUS(nAttr.setHidden(true));
    aOutOpacity = nAttr.create("outOpacity", "oo", MFnNumericData::kFloat);
    CHECK_MSTATUS(nAttr.setKeyable(false));
    CHECK_MSTATUS(nAttr.setStorable(false));
    CHECK_MSTATUS(nAttr.setReadable(true));
    CHECK_MSTATUS(nAttr.setWritable(false));

    // Add attributes
    CHECK_MSTATUS(addAttribute(aOutColor));
    CHECK_MSTATUS(addAttribute(aOutAlpha));
    CHECK_MSTATUS(addAttribute(aOutOpacity));

    return MS::kSuccess;
}

MPxNode::SchedulingType CPVColor::schedulingType() const { return SchedulingType::kParallel; }

MStatus CPVColor::compute(const MPlug& plug, MDataBlock& block)
{
    if ((plug != aOutColor) && (plug.parent() != aOutColor) && (plug != aOutAlpha)
        && (plug != aOutOpacity))
        return MS::kUnknownParameter;

    // Complement alpha for opacity attribute
    MDataHandle outAlphaHandle = block.outputValue(aOutAlpha);
    MDataHandle outOpacityHandle = block.outputValue(aOutOpacity);
    float&      outOpacity = outOpacityHandle.asFloat();
    outOpacity = 1 - outAlphaHandle.asFloat();
    outOpacityHandle.setClean();
    outAlphaHandle.setClean();

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
