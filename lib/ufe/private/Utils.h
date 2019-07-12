// ===========================================================================
// Copyright 2019 Autodesk, Inc. All rights reserved.
//
// Use of this software is subject to the terms of the Autodesk license
// agreement provided at the time of installation or download, or which
// otherwise accompanies this software in either electronic or hard copy form.
// ===========================================================================
#pragma once

#include "../../base/api.h"

#include "ufe/path.h"

#include "pxr/usd/usd/prim.h"
#include "pxr/base/tf/token.h"
#include "pxr/usd/usdGeom/xformCommonAPI.h"

PXR_NAMESPACE_USING_DIRECTIVE

MAYAUSD_NS_DEF {
namespace ufe {

//------------------------------------------------------------------------------
// Private globals and macros
//------------------------------------------------------------------------------
const std::string kIllegalUSDPath = "Illegal USD run-time path %s.";

#if !defined(NDEBUG)
	#define TEST_USD_PATH(SEG, PATH) \
		assert(SEG.size() == 2); \
		if (SEG.size() != 2) \
			TF_WARN(kIllegalUSDPath.c_str(), PATH.string().c_str());
#else
	#define TEST_USD_PATH(SEG, PATH) \
		if (SEG.size() != 2) \
			TF_WARN(kIllegalUSDPath.c_str(), PATH.string().c_str());
#endif

//------------------------------------------------------------------------------
// Private helper functions
//------------------------------------------------------------------------------

//! Extended support for the xform operations.
UsdGeomXformCommonAPI convertToCompatibleCommonAPI(const UsdPrim& prim);

//------------------------------------------------------------------------------
// Operations: translate, rotate, scale, pivot
//------------------------------------------------------------------------------

//! Absolute translation of the given prim.
void translateOp(const UsdPrim& prim, const Ufe::Path& path, double x, double y, double z);

//! Absolute rotation (degrees) of the given prim.
void rotateOp(const UsdPrim& prim, const Ufe::Path& path, double x, double y, double z);

//! Absolute scale of the given prim.
void scaleOp(const UsdPrim& prim, const Ufe::Path& path, double x, double y, double z);

//! Absolute translation of the given prim's pivot point.
void rotatePivotTranslateOp(const UsdPrim& prim, const Ufe::Path& path, double x, double y, double z);

} // namespace ufe
} // namespace MayaUsd
