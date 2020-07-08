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
#include "ProxyShapeTranslator.h"

#include "AL/maya/utils/Utils.h"
#include "AL/usdmaya/fileio/translators/TranslatorBase.h"
#include "AL/usdmaya/nodes/ProxyShape.h"

#include <mayaUsd/fileio/jobs/jobArgs.h>
#include <mayaUsd/fileio/primWriterRegistry.h>
#include <mayaUsd/utils/util.h>

#include <pxr/base/tf/token.h>
#include <pxr/pxr.h>
#include <pxr/usd/kind/registry.h>
#include <pxr/usd/sdf/copyUtils.h>
#include <pxr/usd/sdf/schema.h>
#include <pxr/usd/sdf/types.h>
#include <pxr/usd/usd/modelAPI.h>
#include <pxr/usd/usdGeom/xformable.h>

#include <maya/MTime.h>

PXR_NAMESPACE_OPEN_SCOPE

/* static */
bool AL_USDMayaTranslatorProxyShape::Create(
    const UsdMayaPrimWriterArgs& args,
    UsdMayaPrimWriterContext*    context)
{
    UsdStageRefPtr stage = context->GetUsdStage();
    SdfPath        authorPath = context->GetAuthorPath();
    UsdTimeCode    usdTime = context->GetTimeCode();

    context->SetExportsGprims(false);
    context->SetPruneChildren(true);
    context->SetModelPaths({ authorPath });

    UsdPrim prim = stage->DefinePrim(authorPath);
    if (!prim) {
        MString errorMsg("Failed to create prim for USD reference proxyShape at path: ");
        errorMsg += MString(authorPath.GetText());
        MGlobal::displayError(errorMsg);
        return false;
    }

    // only write references when time is default
    if (!usdTime.IsDefault()) {
        return true;
    }

    const MDagPath&  currPath = args.GetMDagPath();
    const MFnDagNode proxyShapeNode(currPath);
    auto             proxyShape = (AL::usdmaya::nodes::ProxyShape*)proxyShapeNode.userNode();

    std::string refPrimPathStr;
    MPlug       usdRefPrimPathPlg = proxyShape->primPathPlug();
    if (!usdRefPrimPathPlg.isNull()) {
        refPrimPathStr = AL::maya::utils::convert(usdRefPrimPathPlg.asString());
    }

    // Get proxy shape stage and graft session layer onto exported layer.
    // Do this before authoring anything to prim because CopySpec will
    // replace any scene description.
    UsdStageRefPtr shapeStage = proxyShape->usdStage();
    if (shapeStage) {
        SdfPath srcPrimPath;
        if (!refPrimPathStr.empty()) {
            srcPrimPath = SdfPath(refPrimPathStr);
        } else {
            srcPrimPath = shapeStage->GetDefaultPrim().GetPath();
        }
        if (shapeStage->GetSessionLayer()->GetPrimAtPath(srcPrimPath)) {
            // Use custom Fn ShouldGraftValue to non-destructively copy specs.
            // This will preserve Xform type if transform writer has already run on
            // the prim because we are merging xform + shape.
            SdfCopySpec(
                shapeStage->GetSessionLayer(),
                srcPrimPath,
                stage->GetRootLayer(),
                authorPath,
                _ShouldGraftValue,
                _ShouldGraftChildren);
        }
    }

    // Guard against a situation where the prim being referenced has
    // xformOp's specified in its xformOpOrder but the reference assembly
    // in Maya has an identity transform. We would normally skip writing out
    // the xformOpOrder, but that isn't correct since we would inherit the
    // xformOpOrder, which we don't want.
    // Instead, always write out an empty xformOpOrder if the transform writer
    // did not write out an xformOpOrder in its constructor. This guarantees
    // that we get an identity transform as expected (instead of inheriting).
    bool                        resetsXformStack;
    UsdGeomXformable            xformable(prim);
    std::vector<UsdGeomXformOp> orderedXformOps = xformable.GetOrderedXformOps(&resetsXformStack);
    if (orderedXformOps.empty() && !resetsXformStack) {
        xformable.CreateXformOpOrderAttr().Block();
    }

    MPlug usdRefFilepathPlg = proxyShape->filePathPlug();
    if (!usdRefFilepathPlg.isNull()) {
        UsdReferences refs = prim.GetReferences();
        std::string   refAssetPath(AL::maya::utils::convert(usdRefFilepathPlg.asString()));

        std::string resolvedRefPath = stage->ResolveIdentifierToEditTarget(refAssetPath);

        if (!resolvedRefPath.empty()) {
            // If an offset has been applied to the proxyShape, we use the values to
            // construct the reference offset so the resulting stage will be look the
            // same.
            auto           timeOffsetPlug = proxyShape->timeOffsetPlug();
            auto           timeScalarPlug = proxyShape->timeScalarPlug();
            SdfLayerOffset offset(
                timeOffsetPlug.asMTime().as(MTime::uiUnit()),
                // AL_USDMaya interprets the scalar such that 2.0 means
                // fast-forward / play back twice as fast, while the usd spec
                // interprets that as play in slow-motion / half-speed
                1.0 / timeScalarPlug.asDouble());

            if (refPrimPathStr.empty()) {
                refs.AddReference(refAssetPath, offset);
            } else {
                SdfPath refPrimPath(refPrimPathStr);
                refs.AddReference(SdfReference(refAssetPath, refPrimPath, offset));
            }
        } else {
            MString errorMsg("Could not resolve reference '");
            errorMsg += refAssetPath.c_str();
            errorMsg += "'; creating placeholder Xform for <";
            errorMsg += authorPath.GetText();
            errorMsg += ">";
            MGlobal::displayWarning(errorMsg);
            prim.SetDocumentation(errorMsg.asChar());
        }
    }

    bool makeInstanceable = args.GetExportRefsAsInstanceable();
    if (makeInstanceable) {
        // When bug/128076 is addressed, the IsGroup() check will become
        // unnecessary and obsolete.
        // XXX This test also needs to fail if there are sub-root overs
        // on the referenceAssembly!
        TfToken kind;
        UsdModelAPI(prim).GetKind(&kind);
        if (!prim.HasAuthoredInstanceable()
            && !KindRegistry::GetInstance().IsA(kind, KindTokens->group)) {
            prim.SetInstanceable(true);
        }
    }

    return true;
}

PXRUSDMAYA_DEFINE_WRITER(AL_usdmaya_ProxyShape, args, context)
{
    return AL_USDMayaTranslatorProxyShape::Create(args, context);
}

PXR_NAMESPACE_CLOSE_SCOPE
