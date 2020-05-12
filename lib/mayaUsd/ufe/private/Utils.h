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
#pragma once

#include <ufe/path.h>

#include <pxr/usd/usd/prim.h>
#include <pxr/base/tf/token.h>
#include <pxr/usd/usdGeom/xformCommonAPI.h>

#include <mayaUsd/base/api.h>

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

//! Apply restriction rules on the given prim
void applyCommandRestriction(const UsdPrim& prim, const std::string& commandName);

} // namespace ufe
} // namespace MayaUsd
