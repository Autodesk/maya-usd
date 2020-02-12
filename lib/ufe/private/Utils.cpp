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

#include <ufe/log.h>

#include <pxr/base/tf/stringUtils.h>
#include <pxr/usd/usdGeom/xformable.h>

#include <string>
#include <memory>

PXR_NAMESPACE_USING_DIRECTIVE

MAYAUSD_NS_DEF {
namespace ufe {

//------------------------------------------------------------------------------
// Private helper functions
//------------------------------------------------------------------------------

UsdGeomXformCommonAPI convertToCompatibleCommonAPI(const UsdPrim& prim)
{
	// As we are using USD's XformCommonAPI which supports only the following xformOps :
	//    ["xformOp:translate", "xformOp:translate:pivot", "xformOp:rotateXYZ", "xformOp:scale", "!invert!xformOp:translate:pivot"]
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
	for (const auto& op : xformOps)
	{
		auto opName = op.GetOpName();

		// RotateX, RotateY, RotateZ
		if ((opName == rotX) || (opName == rotY) || (opName == rotZ))
		{
			float retValue;
			if (op.Get<float>(&retValue))
			{
				if (opName == rotX)
					primXform.SetRotate(GfVec3f(retValue, 0, 0));
				else if (opName == rotY)
					primXform.SetRotate(GfVec3f(0, retValue, 0));
				else if (opName == rotZ)
					primXform.SetRotate(GfVec3f(0, 0, retValue));
			}
		}
		// RotateXYZ
		else if (opName == rotXYZ)
		{
			GfVec3f retValue;
			if (op.Get<GfVec3f>(&retValue))
			{
				primXform.SetRotate(retValue);
			}
		}
		// Scale
		else if (opName == scale)
		{
			GfVec3f retValue;
			if (op.Get<GfVec3f>(&retValue))
			{
				primXform.SetScale(retValue);
			}
		}
		// Translate
		else if (opName == trans)
		{
			GfVec3d retValue;
			if (op.Get<GfVec3d>(&retValue))
			{
				primXform.SetTranslate(retValue);
			}
		}
		// Scale / rotate pivot
		else if (opName == pivot)
		{
			GfVec3f retValue;
			if (op.Get<GfVec3f>(&retValue))
			{
				primXform.SetPivot(retValue);
			}
		}
		// Scale / rotate pivot inverse
		else if (opName == notPivot)
		{
			// automatically added, nothing to do.
		}
		// Not compatible
		else
		{
			// Restore old
			xformable.SetXformOpOrder(xformOps);
			std::string err = TfStringPrintf("Incompatible xform op %s:", op.GetOpName().GetString().c_str());
			throw std::runtime_error(err.c_str());
		}
	}
	return primXform;
}

} // namespace ufe
} // namespace MayaUsd
