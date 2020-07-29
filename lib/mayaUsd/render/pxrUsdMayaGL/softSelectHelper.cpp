//
// Copyright 2016 Pixar
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
#include "softSelectHelper.h"

#include <pxr/base/tf/stl.h>
#include <pxr/pxr.h>

#include <maya/MGlobal.h>
#include <maya/MItSelectionList.h>
#include <maya/MRichSelection.h>
#include <maya/MSelectionList.h>

PXR_NAMESPACE_OPEN_SCOPE

UsdMayaGLSoftSelectHelper::UsdMayaGLSoftSelectHelper()
    : _populated(false)
{
}

#define _PXRUSDMAYA_SOFTSELECT_COLORRAMP 1

void UsdMayaGLSoftSelectHelper::Reset()
{
    _populated = false;
    _dagPathsToWeight.clear();
}

void UsdMayaGLSoftSelectHelper::Populate()
{
    // only populate if we haven't already
    if (_populated) {
        return;
    }

    _PopulateWeights();
    _PopulateSoftSelectColorRamp();

    _populated = true;
}

void UsdMayaGLSoftSelectHelper::_PopulateWeights()
{
    // we don't want to fallback to the active selection if there is no sot
    // select
    bool           defaultToActiveSelection = false;
    MRichSelection softSelect;
    MGlobal::getRichSelection(softSelect, defaultToActiveSelection);
    MSelectionList selection;
    softSelect.getSelection(selection);

    for (MItSelectionList iter(selection, MFn::kInvalid); !iter.isDone(); iter.next()) {
        MDagPath dagPath;
        MObject  component;

        iter.getDagPath(dagPath, component);
        // component.isNull() indcates that we're selecting a whole object, as
        // opposed to a component.
        if (!component.isNull()) {
            continue;
        }

        float weight = 0.0f;
        _dagPathsToWeight[dagPath] = weight;
    }
}

void UsdMayaGLSoftSelectHelper::_PopulateSoftSelectColorRamp()
{
    // Since in we are not able to get the real distance/weight value, we don't
    // yet store the full color ramp.  We just get the first color which at
    // least gives feedback over which things will be influenced.
    bool    success = false;
    MString commandResult;

    // it's really unfortunate that we have to go through this instead of having
    // direct access to this.
    if (MGlobal::executeCommand("softSelect -query -softSelectColorCurve", commandResult)) {

        // parse only the first tuple.
        int   interp;
        float r, g, b;
        float position;
        if (sscanf(commandResult.asChar(), "%f,%f,%f,%f,%d", &r, &g, &b, &position, &interp) == 5) {
            _wireColor = MColor(r, g, b);
            success = true;
        }
    }

    if (!success) {
        _wireColor = MColor(0.f, 0.f, 1.f);
    }
}

bool UsdMayaGLSoftSelectHelper::GetWeight(const MDagPath& dagPath, float* weight) const
{
    return TfMapLookup(_dagPathsToWeight, dagPath, weight);
}

bool UsdMayaGLSoftSelectHelper::GetFalloffColor(const MDagPath& dagPath, MColor* falloffColor) const
{
    float weight = 0.f;
    if (GetWeight(dagPath, &weight)) {
        if (falloffColor != nullptr) {
            *falloffColor = _wireColor;
        }

        return true;
    }

    return false;
}

PXR_NAMESPACE_CLOSE_SCOPE
