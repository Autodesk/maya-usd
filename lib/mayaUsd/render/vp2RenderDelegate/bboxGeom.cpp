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
#include "bboxGeom.h"

#include <pxr/base/arch/threads.h>
#include <pxr/base/tf/diagnostic.h>

PXR_NAMESPACE_OPEN_SCOPE

//! Constructor. Call from main thread only.
HdVP2BBoxGeom::HdVP2BBoxGeom()
    : _range(GfVec3d(-0.5, -0.5, -0.5), GfVec3d(0.5, 0.5, 0.5))
{
    // MVertexBuffer::commit() & MIndexBuffer::commit() can work only when being
    // called from main thread.
    TF_VERIFY(ArchIsMainThread(), "Creating HdVP2BBoxGeom from worker threads");

    const MHWRender::MVertexBufferDescriptor vbDesc("", MGeometry::kPosition, MGeometry::kFloat, 3);

    _positionBuffer.reset(new MHWRender::MVertexBuffer(vbDesc));

    if (void* buffer = _positionBuffer->acquire(8, true)) {
        constexpr float vertexData[] = {
            -0.5f, -0.5f, -0.5f, // vtx 0
            -0.5f, -0.5f, 0.5f,  // vtx 1
            -0.5f, 0.5f,  -0.5f, // vtx 2
            -0.5f, 0.5f,  0.5f,  // vtx 3
            0.5f,  -0.5f, -0.5f, // vtx 4
            0.5f,  -0.5f, 0.5f,  // vtx 5
            0.5f,  0.5f,  -0.5f, // vtx 6
            0.5f,  0.5f,  0.5f   // vtx 7
        };

        memcpy(buffer, vertexData, sizeof(vertexData));

        _positionBuffer->commit(buffer);
    }

    _indexBuffer.reset(new MHWRender::MIndexBuffer(MGeometry::kUnsignedInt32));

    if (void* buffer = _indexBuffer->acquire(24, true)) {
        constexpr unsigned int indexData[] = {
            0, 4, // edge 0
            1, 5, // edge 1
            2, 6, // edge 2
            3, 7, // edge 3
            0, 2, // edge 4
            1, 3, // edge 5
            4, 6, // edge 6
            5, 7, // edge 7
            0, 1, // edge 8
            2, 3, // edge 9
            4, 5, // edge 10
            6, 7  // edge 11
        };

        memcpy(buffer, indexData, sizeof(indexData));

        _indexBuffer->commit(buffer);
    }
}

PXR_NAMESPACE_CLOSE_SCOPE
