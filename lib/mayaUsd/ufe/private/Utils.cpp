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
#include "Utils.h"

#include <pxr/base/tf/stringUtils.h>
#include <pxr/usd/usdGeom/xformable.h>

#include <maya/MGlobal.h>
#include <ufe/log.h>

#include <memory>
#include <string>

PXR_NAMESPACE_USING_DIRECTIVE

namespace MAYAUSD_NS_DEF {
namespace ufe {

//------------------------------------------------------------------------------
// Private helper functions
//------------------------------------------------------------------------------

UsdGeomXformCommonAPI convertToCompatibleCommonAPI(const UsdPrim& prim)
{
    // As we are using USD's XformCommonAPI which supports only the following xformOps :
    //    ["xformOp:translate", "xformOp:translate:pivot", "xformOp:rotateXYZ", "xformOp:scale",
    //    "!invert!xformOp:translate:pivot"]
    // We are extending the supported xform Operations with :
    //    ["xformOp:rotateX", "xformOp:rotateY", "xformOp:rotateZ"]
    // Where we convert these into xformOp:rotateXYZ.

    static TfToken rotX("xformOp:rotateX");
    static TfToken rotY("xformOp:rotateY");
    static TfToken rotZ("xformOp:rotateZ");
    static TfToken rotXYZ("xformOp:rotateXYZ");
    static TfToken scale("xformOp:scale");
    static TfToken trans("xformOp:translate");
    static TfToken pivot("xformOp:translate:pivot");
    static TfToken notPivot("!invert!xformOp:translate:pivot");

    auto xformable = UsdGeomXformable(prim);
    bool resetsXformStack;
    auto xformOps = xformable.GetOrderedXformOps(&resetsXformStack);
    xformable.ClearXformOpOrder();
    auto primXform = UsdGeomXformCommonAPI(prim);
    for (const auto& op : xformOps) {
        auto opName = op.GetOpName();

        // RotateX, RotateY, RotateZ
        if ((opName == rotX) || (opName == rotY) || (opName == rotZ)) {
            float retValue;
            if (op.Get<float>(&retValue)) {
                if (opName == rotX)
                    primXform.SetRotate(GfVec3f(retValue, 0, 0));
                else if (opName == rotY)
                    primXform.SetRotate(GfVec3f(0, retValue, 0));
                else if (opName == rotZ)
                    primXform.SetRotate(GfVec3f(0, 0, retValue));
            }
        }
        // RotateXYZ
        else if (opName == rotXYZ) {
            GfVec3f retValue;
            if (op.Get<GfVec3f>(&retValue)) {
                primXform.SetRotate(retValue);
            }
        }
        // Scale
        else if (opName == scale) {
            GfVec3f retValue;
            if (op.Get<GfVec3f>(&retValue)) {
                primXform.SetScale(retValue);
            }
        }
        // Translate
        else if (opName == trans) {
            GfVec3d retValue;
            if (op.Get<GfVec3d>(&retValue)) {
                primXform.SetTranslate(retValue);
            }
        }
        // Scale / rotate pivot
        else if (opName == pivot) {
            GfVec3f retValue;
            if (op.Get<GfVec3f>(&retValue)) {
                primXform.SetPivot(retValue);
            }
        }
        // Scale / rotate pivot inverse
        else if (opName == notPivot) {
            // automatically added, nothing to do.
        }
        // Not compatible
        else {
            // Restore old
            auto result = xformable.SetXformOpOrder(xformOps);
            TF_AXIOM(result);
            std::string err
                = TfStringPrintf("Incompatible xform op %s:", op.GetOpName().GetString().c_str());
            throw std::runtime_error(err.c_str());
        }
    }
    return primXform;
}

//------------------------------------------------------------------------------
// Operations: translate, rotate, scale, pivot
//------------------------------------------------------------------------------

void translateOp(const UsdPrim& prim, const Ufe::Path& path, double x, double y, double z)
{
    auto primXform = UsdGeomXformCommonAPI(prim);
    if (!primXform.SetTranslate(GfVec3d(x, y, z))) {
        // This could mean that we have an incompatible xformOp in the stack
        try {
            primXform = convertToCompatibleCommonAPI(prim);
            if (!primXform.SetTranslate(GfVec3d(x, y, z)))
                throw std::runtime_error("Unable to SetTranslate after conversion to CommonAPI.");
        } catch (const std::exception& e) {
            std::string err = TfStringPrintf(
                "Failed to translate prim %s - %s", path.string().c_str(), e.what());
            UFE_LOG(err.c_str());
            throw;
        }
    }
}

void rotateOp(const UsdPrim& prim, const Ufe::Path& path, double x, double y, double z)
{
    auto primXform = UsdGeomXformCommonAPI(prim);
    if (!primXform.SetRotate(GfVec3f(x, y, z))) {
        // This could mean that we have an incompatible xformOp in the stack
        try {
            primXform = convertToCompatibleCommonAPI(prim);
            if (!primXform.SetRotate(GfVec3f(x, y, z)))
                throw std::runtime_error("Unable to SetRotate after conversion to CommonAPI.");
        } catch (const std::exception& e) {
            std::string err
                = TfStringPrintf("Failed to rotate prim %s - %s", path.string().c_str(), e.what());
            UFE_LOG(err.c_str());
            throw;
        }
    }
}

void scaleOp(const UsdPrim& prim, const Ufe::Path& path, double x, double y, double z)
{
    auto primXform = UsdGeomXformCommonAPI(prim);
    if (!primXform.SetScale(GfVec3f(x, y, z))) {
        // This could mean that we have an incompatible xformOp in the stack
        try {
            primXform = convertToCompatibleCommonAPI(prim);
            if (!primXform.SetScale(GfVec3f(x, y, z)))
                throw std::runtime_error("Unable to SetScale after conversion to CommonAPI.");
        } catch (const std::exception& e) {
            std::string err
                = TfStringPrintf("Failed to scale prim %s - %s", path.string().c_str(), e.what());
            UFE_LOG(err.c_str());
            throw;
        }
    }
}

void rotatePivotTranslateOp(
    const UsdPrim&   prim,
    const Ufe::Path& path,
    double           x,
    double           y,
    double           z)
{
    auto primXform = UsdGeomXformCommonAPI(prim);
    if (!primXform.SetPivot(GfVec3f(x, y, z))) {
        // This could mean that we have an incompatible xformOp in the stack
        try {
            primXform = convertToCompatibleCommonAPI(prim);
            if (!primXform.SetPivot(GfVec3f(x, y, z)))
                throw std::runtime_error("Unable to SetPivot after conversion to CommonAPI.");
        } catch (const std::exception& e) {
            std::string err = TfStringPrintf(
                "Failed to set pivot for prim %s - %s", path.string().c_str(), e.what());
            UFE_LOG(err.c_str());
            throw;
        }
    }
}

} // namespace ufe
} // namespace MAYAUSD_NS_DEF
