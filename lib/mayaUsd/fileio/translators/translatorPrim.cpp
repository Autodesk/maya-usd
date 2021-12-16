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
#include "translatorPrim.h"

#include <mayaUsd/fileio/translators/translatorUtil.h>
#include <mayaUsd/fileio/utils/readUtil.h>
#include <mayaUsd/utils/util.h>

#include <pxr/usd/usdGeom/imageable.h>

#include <maya/MFnAnimCurve.h>
#include <maya/MFnDagNode.h>

PXR_NAMESPACE_OPEN_SCOPE

void UsdMayaTranslatorPrim::Read(
    const UsdPrim&               prim,
    MObject                      mayaNode,
    const UsdMayaPrimReaderArgs& args,
    UsdMayaPrimReaderContext*    context)
{
    UsdGeomImageable primSchema(prim);
    MTime::Unit      timeUnit = MTime::uiUnit();
    double timeSampleMultiplier = (context != nullptr) ? context->GetTimeSampleMultiplier() : 1.0;
    if (!primSchema) {
        TF_CODING_ERROR("Prim %s is not UsdGeomImageable.", prim.GetPath().GetText());
        return;
    }

    // Gather visibility
    // If timeInterval is non-empty, pick the first available sample in the
    // timeInterval or default.
    UsdTimeCode         visTimeSample = UsdTimeCode::EarliestTime();
    std::vector<double> visTimeSamples;
    size_t              visNumTimeSamples = 0;
    if (!args.GetTimeInterval().IsEmpty()) {
        primSchema.GetVisibilityAttr().GetTimeSamplesInInterval(
            args.GetTimeInterval(), &visTimeSamples);
        visNumTimeSamples = visTimeSamples.size();
        if (visNumTimeSamples > 0) {
            visTimeSample = visTimeSamples[0];
        }
    }

    MStatus           status;
    MFnDependencyNode depFn(mayaNode);
    TfToken           visibilityTok;

    if (primSchema.GetVisibilityAttr().Get(&visibilityTok, visTimeSample)) {
        UsdMayaUtil::setPlugValue(depFn, "visibility", visibilityTok != UsdGeomTokens->invisible);
    }

    // == Animation ==
    if (visNumTimeSamples > 0) {
        size_t       numTimeSamples = visNumTimeSamples;
        MDoubleArray valueArray(numTimeSamples);

        // Populate the channel arrays
        for (unsigned int ti = 0; ti < visNumTimeSamples; ++ti) {
            primSchema.GetVisibilityAttr().Get(&visibilityTok, visTimeSamples[ti]);
            valueArray[ti] = static_cast<double>(visibilityTok != UsdGeomTokens->invisible);
        }

        // == Write to maya node ==
        MFnDagNode   depFn(mayaNode);
        MPlug        plg;
        MFnAnimCurve animFn;

        // Construct the time array to be used for all the keys
        MTimeArray timeArray;
        timeArray.setLength(numTimeSamples);
        for (unsigned int ti = 0; ti < numTimeSamples; ++ti) {
            timeArray.set(MTime(visTimeSamples[ti] * timeSampleMultiplier, timeUnit), ti);
        }

        // Add the keys
        plg = depFn.findPlug("visibility");
        if (!plg.isNull()) {
            MObject animObj = animFn.create(plg, nullptr, &status);
            animFn.addKeys(&timeArray, &valueArray);
            if (context) {
                context->RegisterNewMayaNode(animFn.name().asChar(), animObj); // used for undo/redo
            }
        }
    }

    // Process UsdGeomImageable typed schema (note that purpose is uniform).
    UsdMayaReadUtil::ReadSchemaAttributesFromPrim<UsdGeomImageable>(
        prim, mayaNode, { UsdGeomTokens->purpose });

    // Process API schema attributes and strongly-typed metadata.
    UsdMayaReadUtil::ReadMetadataFromPrim(args.GetIncludeMetadataKeys(), prim, mayaNode);

    // XXX What about all the "user attributes" that PrimWriter exports???
}

PXR_NAMESPACE_CLOSE_SCOPE
