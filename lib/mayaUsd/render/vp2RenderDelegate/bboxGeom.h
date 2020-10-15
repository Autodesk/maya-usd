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
#ifndef HD_VP2_BBOX_GEOM
#define HD_VP2_BBOX_GEOM

#include <pxr/base/gf/range3d.h>
#include <pxr/pxr.h>

#include <maya/MHWGeometry.h>

#include <memory>

PXR_NAMESPACE_OPEN_SCOPE

/*! \brief  Geometry used for bounding box display in VP2.
    \class  HdVP2BBoxGeom

    The class defines a unit wire cube centered at origin. It can be used to
    provide shared geometry for all Rprims to display bounding box in VP2.
    The class can only be instantiated from main thread.
*/
class HdVP2BBoxGeom final
{
public:
    HdVP2BBoxGeom();
    ~HdVP2BBoxGeom() = default;

    const MHWRender::MVertexBuffer* GetPositionBuffer() const { return _positionBuffer.get(); }

    const MHWRender::MIndexBuffer* GetIndexBuffer() const { return _indexBuffer.get(); }

    const GfRange3d& GetRange() const { return _range; }

private:
    std::unique_ptr<MVertexBuffer> _positionBuffer; //!< Position buffer of the geometry
    std::unique_ptr<MIndexBuffer>  _indexBuffer;    //!< Index buffer of the geometry
    GfRange3d                      _range;          //!< Range of the geometry
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // HD_VP2_BBOX_GEOM
