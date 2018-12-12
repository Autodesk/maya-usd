//
// Copyright 2017 Animal Logic
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.//
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

#include <gtest/gtest.h>

#include "maya/MAngle.h"
#include "maya/MDistance.h"
#include "maya/MFnDependencyNode.h"
#include "maya/MPlug.h"
#include "maya/MString.h"
#include "maya/MTime.h"
#include "AL/usdmaya/nodes/ProxyShape.h"
#include "AL/usdmaya/test/testHelpers.h"

#include <cstdint>
#include <iostream>
#include <functional>

#ifdef TRACE_ASSIGNMENT
# define TRACE(X) X;
#else
# define TRACE(X)
#endif

#if 0
# define AL_OUTPUT_TEST_NAME(X) std::cerr << X << std::endl;
#else
# define AL_OUTPUT_TEST_NAME(X)
#endif


#if 0
# define AL_USDMAYA_UNTESTED EXPECT_TRUE(false)
#else
# define AL_USDMAYA_UNTESTED
#endif


  /// \brief  Creates a ProxyShape with the contents generated from the buildUsdStage function object. It will export it into the specified temp_location and
  ///         feed this temp-file into a ProxyShape
  /// \param[in] buildUsdStage function that will populate a USDStage and return a UsdStageRefPtr
  /// \param[in] tempPath to where the contents of the populated stage is written out to. This file is then read into the ProxyShape.
  /// \return AL::usdmaya::nodes::ProxyShape
extern AL::usdmaya::nodes::ProxyShape* CreateMayaProxyShape(std::function<UsdStageRefPtr()> buildUsdStage, const std::string& tempPath, MObject* shapeParent=nullptr);

/// \brief  Creates a ProxyShape with the contents generated from the buildUsdStage function object.
//          It will export it into the specified temp_location and
///         feed this temp-file into a ProxyShape
/// \param[in] rootLayerPath function that will populate a USDStage and return a UsdStageRefPtr
/// \return AL::usdmaya::nodes::ProxyShape
extern AL::usdmaya::nodes::ProxyShape* CreateMayaProxyShape(const std::string& rootLayerPath);

/// \brief  Creates a ProxyShape with the a single root node Mesh typed prim that contains geometry that represents a sphere.
/// \return AL::usdmaya::nodes::ProxyShape
AL::usdmaya::nodes::ProxyShape* SetupProxyShapeWithMesh();

/// \brief  Creates a ProxyShape with multiple root node Mesh typed prim that contains geometry that represents a sphere.
/// \return AL::usdmaya::nodes::ProxyShape
extern AL::usdmaya::nodes::ProxyShape* SetupProxyShapeWithMultipleMeshes();

