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
#include "maya/MPxDrawOverride.h"
#include "AL/usdmaya/ForwardDeclares.h"

#include "pxr/pxr.h"
#include "pxr/usdImaging/usdImaging/version.h"

#include "AL/usd/utils/ForwardDeclares.h"
#if (USD_IMAGING_API_VERSION >= 7)
# include "pxr/usdImaging/usdImagingGL/hdEngine.h"
#else
# include "pxr/usdImaging/usdImaging/hdEngine.h"
#endif

PXR_NAMESPACE_USING_DIRECTIVE


namespace AL {
namespace usdmaya {
namespace nodes {

//----------------------------------------------------------------------------------------------------------------------
/// \brief  This class provides the draw override for the USD proxy shape node.
/// \ingroup nodes
//----------------------------------------------------------------------------------------------------------------------
class ProxyDrawOverride
  : public MHWRender::MPxDrawOverride
{
public:

  /// \brief  ctor
  /// \param  obj the object this override will be rendering
  ProxyDrawOverride(const MObject& obj);

  /// \brief  dtor
  ~ProxyDrawOverride();

  /// \brief  static creator method
  /// \param  obj the handle to pass to the constructor
  /// \return the new draw override instance
  static MHWRender::MPxDrawOverride* creator(const MObject& obj);

  /// \brief  Called by Maya to determine if the drawable object is bounded or not.
  /// \param  objPath The path to the object being drawn
  /// \param  cameraPath  The path to the camera that is being used to draw
  /// \return true if the object is bounded
  bool isBounded(
      const MDagPath& objPath,
      const MDagPath& cameraPath) const override;

  /// \brief  Called by Maya whenever the bounding box of the drawable object is needed.
  /// \param  objPath The path to the object being drawn
  /// \param  cameraPath  The path to the camera that is being used to draw
  /// \return the objects bounding box
  MBoundingBox boundingBox(
      const MDagPath& objPath,
      const MDagPath& cameraPath) const override;

  /// \brief  Called by Maya whenever the object is dirty and needs to update for draw.
  /// \param  objPath The path to the object being drawn
  /// \param  cameraPath  The path to the camera that is being used to draw
  /// \param  frameContext  Frame level context information
  /// \param  oldData Data cached by the previous draw of the instance
  /// \return Pointer to data to be passed to the draw callback method
  MUserData* prepareForDraw(
      const MDagPath& objPath,
      const MDagPath& cameraPath,
      const MHWRender::MFrameContext& frameContext,
      MUserData* oldData) override;

  /// \brief  draw classification string for this override
  static MString kDrawDbClassification;

  /// \brief  draw registration id for this override
  static MString kDrawRegistrantId;

  /// \brief  The draw callback, performs the actual rendering for the draw override.
  /// \param  context the current draw context
  /// \param  data the user data generated om the prepareForDraw method
  static void draw(const MHWRender::MDrawContext& context, const MUserData* data);

  /// \brief  utility function to get a pointer to the proxy shape node given the specified path
  /// \param  objPath the path to the shape you wish to query
  /// \return returns a pointer to the proxy shape node at the path (or null if not found)
  static ProxyShape* getShape(const MDagPath& objPath);

  /// \brief  We support the legacy and VP2 core profile rendering.
  /// \return MHWRender::kOpenGL | MHWRender::kOpenGLCoreProfile
  MHWRender::DrawAPI supportedDrawAPIs() const override
    { return MHWRender::kOpenGL | MHWRender::kOpenGLCoreProfile; }

#if MAYA_API_VERSION >= 201700
  /// \brief  ensure this draw override participates in post fx
  /// \return false
  bool excludedFromPostEffects() const override
    { return false; }
#endif
};

//----------------------------------------------------------------------------------------------------------------------
} // nodes
} // usdmaya
} // AL
//----------------------------------------------------------------------------------------------------------------------
