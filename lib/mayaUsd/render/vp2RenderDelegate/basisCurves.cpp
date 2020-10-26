//
// Copyright 2018 Pixar
// Copyright 2020 Autodesk
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
#include "basisCurves.h"

#include <maya/MMatrix.h>
#include <maya/MProfiler.h>
#include <maya/MSelectionMask.h>

#include <pxr/base/gf/matrix4d.h>
#include <pxr/base/gf/matrix4f.h>
#include <pxr/base/vt/value.h>
#include <pxr/imaging/hd/repr.h>
#include <pxr/imaging/hd/sceneDelegate.h>
#include <pxr/imaging/hd/tokens.h>
#include <pxr/imaging/hd/version.h>

#include "bboxGeom.h"
#include "debugCodes.h"
#include "draw_item.h"
#include "instancer.h"
#include "material.h"
#include "render_delegate.h"
#include "tokens.h"

// Complete tessellation shader support is avaiable for basisCurves complexity levels
#if MAYA_API_VERSION >= 20210000
#define HDVP2_ENABLE_BASISCURVES_TESSELLATION
#endif

PXR_NAMESPACE_OPEN_SCOPE

namespace {

    //! Required primvars when there is no material binding.
    const TfTokenVector sFallbackShaderPrimvars = {
        HdTokens->displayColor,
        HdTokens->displayOpacity,
        HdTokens->normals,
        HdTokens->widths
    };

    const MColor kOpaqueGray(.18f, .18f, .18f, 1.0f);                  //!< The default 18% gray color
    const unsigned int kNumColorChannels = 4;                          //!< The number of color channels

    //! Cached strings for efficiency
    const MString kPositionsStr("positions");
    const MString kNormalsStr("normals");
    const MString kWidthStr("U0_1");
    const MString kDiffuseColorStr("diffuseColor");
    const MString kSolidColorStr("solidColor");

    //! A primvar vertex buffer data map indexed by primvar name.
    using PrimvarBufferDataMap = std::unordered_map<
        TfToken,
        void*,
        TfToken::HashFunctor
    >;

    //! \brief  Helper struct used to package all the changes into single commit task
    //!         (such commit task will be executed on main-thread)
    struct CommitState {
        HdVP2DrawItem::RenderItemData& _drawItemData;

        //! If valid, new index buffer data to commit
        int*    _indexBufferData{ nullptr };
        //! If valid, new color buffer data to commit
        void*   _colorBufferData{ nullptr };
        //! If valid, new normals buffer data to commit
        void*   _normalsBufferData{ nullptr };
        //! If valid, new primvar buffer data to commit
        PrimvarBufferDataMap _primvarBufferDataMap;

        //! If valid, world matrix to set on the render item
        MMatrix* _worldMatrix{ nullptr };

        //! If valid, bounding box to set on the render item
        MBoundingBox* _boundingBox{ nullptr };

        //! if valid, enable or disable the render item
        bool* _enabled{ nullptr };

        //! if valid, set the primitive type on the render item
        MHWRender::MGeometry::Primitive* _primitiveType{ nullptr };
        //! if valid, set the primitive stride on the render item
        int* _primitiveStride{ nullptr };

        //! Instancing doesn't have dirty bits, every time we do update, we must update instance transforms
        MMatrixArray _instanceTransforms;

        //! Color array to support per-instance color and selection highlight.
        MFloatArray _instanceColors;

        //! If valid, new shader instance to set
        MHWRender::MShaderInstance* _shader{ nullptr };

        //! Is this object transparent
        bool _isTransparent{ false };

        //! If true, associate geometric buffers to the render item and trigger consolidation/instancing update
        bool _geometryDirty{ false };

        //! Construct valid commit state
        CommitState(HdVP2DrawItem& item) : _drawItemData(item.GetRenderItemData())
        {}

        //! No default constructor, we need draw item and dirty bits.
        CommitState() = delete;
    };

    template <typename T> VtArray<T>
    InterpolateVarying(
        size_t numVerts,
        VtIntArray const & vertexCounts,
        TfToken wrap,
        TfToken basis,
        VtArray<T> const & authoredValues)
    {
        VtArray<T> outputValues(numVerts);

        size_t srcIndex = 0;
        size_t dstIndex = 0;

        if (wrap == HdTokens->periodic) {
            // XXX : Add support for periodic curves
            TF_WARN("Varying data is only supported for non-periodic curves.");
        }

        TF_FOR_ALL(itVertexCount, vertexCounts) {
            int nVerts = *itVertexCount;

            // Handling for the case of potentially incorrect vertex counts 
            if(nVerts < 1) {
                continue;
            }

            if(basis == HdTokens->catmullRom || basis == HdTokens->bSpline) {
                // For splines with a vstep of 1, we are doing linear interpolation 
                // between segments, so all we do here is duplicate the first and 
                // last outputValues. Since these are never acutally used during 
                // drawing, it would also work just to set the to 0.
                outputValues[dstIndex] = authoredValues[srcIndex];
                ++dstIndex;
                for (int i = 1; i < nVerts - 2; ++i){
                    outputValues[dstIndex] = authoredValues[srcIndex];
                    ++dstIndex; ++srcIndex;
                }
                outputValues[dstIndex] = authoredValues[srcIndex];
                ++dstIndex;
                outputValues[dstIndex] = authoredValues[srcIndex];
                ++dstIndex; ++srcIndex;
            }
            else if (basis == HdTokens->bezier){
                // For bezier splines, we map the linear values to cubic values
                // the begin value gets mapped to the first two vertices and
                // the end value gets mapped to the last two vertices in a segment.
                // shaders can choose to access value[1] and value[2] when linearly
                // interpolating a value, which happens to match up with the
                // indexing to use for catmullRom and bSpline basis.
                int vStep = 3;
                outputValues[dstIndex] = authoredValues[srcIndex];
                ++dstIndex; // don't increment the srcIndex
                outputValues[dstIndex] = authoredValues[srcIndex];
                ++dstIndex; ++ srcIndex;

                // vstep - 1 control points will have an interpolated value
                for(int i = 2; i < nVerts - 2; i += vStep) {
                    outputValues[dstIndex] = authoredValues[srcIndex];
                    ++ dstIndex; // don't increment the srcIndex
                    outputValues[dstIndex] = authoredValues[srcIndex];
                    ++ dstIndex; // don't increment the srcIndex
                    outputValues[dstIndex] = authoredValues[srcIndex];
                    ++ dstIndex; ++ srcIndex; 
                }
                outputValues[dstIndex] = authoredValues[srcIndex];
                ++dstIndex; // don't increment the srcIndex
                outputValues[dstIndex] = authoredValues[srcIndex];
                ++dstIndex; ++ srcIndex;
            }
            else {
                TF_WARN("Unsupported basis: '%s'", basis.GetText());
            }
        }
        TF_VERIFY(srcIndex == authoredValues.size());
        TF_VERIFY(dstIndex == numVerts);
    
        return outputValues;
    }

    VtValue _BuildCubicIndexArray(const HdBasisCurvesTopology& topology)
    {
        /* 
        Here's a diagram of what's happening in this code:

        For open (non periodic, wrap = false) curves:

          bezier (vStep = 3)
          0------1------2------3------4------5------6 (vertex index)
          [======= seg0 =======]
                               [======= seg1 =======]


          bspline / catmullRom (vStep = 1)
          0------1------2------3------4------5------6 (vertex index)
          [======= seg0 =======]
                 [======= seg1 =======]
                        [======= seg2 =======]
                               [======= seg3 =======]


        For closed (periodic, wrap = true) curves:

           periodic bezier (vStep = 3)
           0------1------2------3------4------5------0 (vertex index)
           [======= seg0 =======]
                                [======= seg1 =======]


           periodic bspline / catmullRom (vStep = 1)
           0------1------2------3------4------5------0------1------2 (vertex index)
           [======= seg0 =======]
                  [======= seg1 =======]
                         [======= seg2 =======]
                                [======= seg3 =======]
                                       [======= seg4 =======]
                                              [======= seg5 =======]
        */
        std::vector<GfVec4i> indices;

        const VtArray<int> vertexCounts = topology.GetCurveVertexCounts();
        bool wrap = topology.GetCurveWrap() == HdTokens->periodic;
        int vStep;
        TfToken basis = topology.GetCurveBasis();
        if(basis == HdTokens->bezier) {
            vStep = 3;
        }
        else {
            vStep = 1;
        }

        int vertexIndex = 0;
        int curveIndex = 0;
        TF_FOR_ALL(itCounts, vertexCounts) {
            int count = *itCounts;
            // The first segment always eats up 4 verts, not just vstep, so to
            // compensate, we break at count - 3.
            int numSegs;

            // If we're closing the curve, make sure that we have enough
            // segments to wrap all the way back to the beginning.
            if (wrap) {
                numSegs = count / vStep;
            } else {
                numSegs = ((count - 4) / vStep) + 1;
            }

            for(int i = 0;i < numSegs; ++i) {

                // Set up curve segments based on curve basis
                GfVec4i seg;
                int offset = i*vStep;
                for(int v = 0;v < 4; ++v) {
                    // If there are not enough verts to round out the segment
                    // just repeat the last vert.
                    seg[v] = wrap 
                        ? vertexIndex + ((offset + v) % count)
                        : vertexIndex + std::min(offset + v, (count -1));
                }
                indices.push_back(seg);
            }
            vertexIndex += count;
            curveIndex++;
        }

        VtVec4iArray finalIndices(indices.size());
        VtIntArray const &curveIndices = topology.GetCurveIndices();

        // If have topology has indices set, map the generated indices
        // with the given indices.
        if (curveIndices.empty())
        {
            std::copy(indices.begin(), indices.end(), finalIndices.begin());
        }
        else
        {
            size_t lineCount = indices.size();
            int maxIndex = curveIndices.size() - 1;

            for (size_t lineNum = 0; lineNum < lineCount; ++lineNum)
            {
                const GfVec4i &line = indices[lineNum];

                int i0 = std::min(line[0], maxIndex);
                int i1 = std::min(line[1], maxIndex);
                int i2 = std::min(line[2], maxIndex);
                int i3 = std::min(line[3], maxIndex);

                int v0 = curveIndices[i0];
                int v1 = curveIndices[i1];
                int v2 = curveIndices[i2];
                int v3 = curveIndices[i3];

                finalIndices[lineNum].Set(v0, v1, v2, v3);
            }
        }

        return VtValue(finalIndices);
    }

    VtValue _BuildLinesIndexArray(const HdBasisCurvesTopology& topology)
    {
        std::vector<GfVec2i> indices;
        VtArray<int> vertexCounts = topology.GetCurveVertexCounts();

        int vertexIndex = 0;
        int curveIndex = 0;
        TF_FOR_ALL(itCounts, vertexCounts) {
            for(int i = 0; i < *itCounts; i+= 2) {
                indices.push_back(GfVec2i(vertexIndex, vertexIndex + 1));
                vertexIndex += 2;
            }
            curveIndex++;
        }

        VtVec2iArray finalIndices(indices.size());
        VtIntArray const &curveIndices = topology.GetCurveIndices();

        // If have topology has indices set, map the generated indices
        // with the given indices.
        if (curveIndices.empty())
        {
            std::copy(indices.begin(), indices.end(), finalIndices.begin());
        }
        else
        {
            size_t lineCount = indices.size();
            int maxIndex = curveIndices.size() - 1;

            for (size_t lineNum = 0; lineNum < lineCount; ++lineNum)
            {
                const GfVec2i &line = indices[lineNum];

                int i0 = std::min(line[0], maxIndex);
                int i1 = std::min(line[1], maxIndex);

                int v0 = curveIndices[i0];
                int v1 = curveIndices[i1];

                finalIndices[lineNum].Set(v0, v1);
            }
        }

        return VtValue(finalIndices);
    }

    VtValue _BuildLineSegmentIndexArray(const HdBasisCurvesTopology& topology)
    {
        const TfToken basis = topology.GetCurveBasis();
        const bool skipFirstAndLastSegs = (basis == HdTokens->catmullRom);

        std::vector<GfVec2i> indices;
        const VtArray<int> vertexCounts = topology.GetCurveVertexCounts();
        bool wrap = topology.GetCurveWrap() == HdTokens->periodic;
        int vertexIndex = 0; // Index of next vertex to emit
        int curveIndex = 0;  // Index of next curve to emit
        // For each curve
        TF_FOR_ALL(itCounts, vertexCounts) {
            int v0 = vertexIndex;
            int v1;
            // Store first vert index incase we are wrapping
            const int firstVert = v0;
            ++ vertexIndex;
            for(int i = 1;i < *itCounts; ++i) {
                v1 = vertexIndex;
                ++ vertexIndex;
                if (!skipFirstAndLastSegs || (i > 1 && i < (*itCounts)-1)) {
                    indices.push_back(GfVec2i(v0, v1));
                }
                v0 = v1;
            }
            if(wrap) {
                indices.push_back(GfVec2i(v0, firstVert));
            }
            ++curveIndex;
        }

        VtVec2iArray finalIndices(indices.size());
        VtIntArray const &curveIndices = topology.GetCurveIndices();

        // If have topology has indices set, map the generated indices
        // with the given indices.
        if (curveIndices.empty())
        {
            std::copy(indices.begin(), indices.end(), finalIndices.begin());
        }
        else
        {
            size_t lineCount = indices.size();
            int maxIndex = curveIndices.size() - 1;

            for (size_t lineNum = 0; lineNum < lineCount; ++lineNum)
            {
                const GfVec2i &line = indices[lineNum];

                int i0 = std::min(line[0], maxIndex);
                int i1 = std::min(line[1], maxIndex);

                int v0 = curveIndices[i0];
                int v1 = curveIndices[i1];

                finalIndices[lineNum].Set(v0, v1);
            }
        }

        return VtValue(finalIndices);
    }

    VtVec3fArray _BuildInterpolatedArray(
        const HdBasisCurvesTopology& topology,
        const VtVec3fArray& authoredData)
    {
        // We need to interpolate primvar depending on its type
        size_t numVerts = topology.CalculateNeededNumberOfControlPoints();

        VtVec3fArray result(numVerts);
        size_t size = authoredData.size();

        if(size == 1) {
            // Uniform data
            const GfVec3f& elem = authoredData[0];
            for(size_t i = 0; i < numVerts; ++ i) {
                result[i] = elem;
            }
        }
        else if(size == numVerts) {
            // Vertex data
            result = authoredData;
        }
        else if(size == topology.CalculateNeededNumberOfVaryingControlPoints()) {
            // Varying data
            result = InterpolateVarying<GfVec3f>(numVerts,
                topology.GetCurveVertexCounts(),
                topology.GetCurveWrap(),
                topology.GetCurveBasis(),
                authoredData);
        }
        else {
            // Fallback
            const GfVec3f elem(1.0f, 0.0f, 0.0f);
            for(size_t i = 0; i < numVerts; ++ i) {
                result[i] = elem;
            }
            TF_WARN("Incorrect number of primvar data, using default GfVec3f(0,0,0) for rendering.");
        }

        return result;
    }

    VtFloatArray _BuildInterpolatedArray(
        const HdBasisCurvesTopology& topology,
        const VtFloatArray& authoredData)
    {
        // We need to interpolate primvar depending on its type
        size_t numVerts = topology.CalculateNeededNumberOfControlPoints();

        VtFloatArray result(numVerts);
        size_t size = authoredData.size();

        if(size == 1) {
            // Uniform or missing data
            float elem = authoredData[0];
            for(size_t i = 0; i < numVerts; ++ i) {
                result[i] = elem;
            }
        }
        else if(size == numVerts) {
            // Vertex data
            result = authoredData;
        }
        else if(size == topology.CalculateNeededNumberOfVaryingControlPoints()) {
            // Varying data
            result = InterpolateVarying<float>(numVerts,
                topology.GetCurveVertexCounts(),
                topology.GetCurveWrap(),
                topology.GetCurveBasis(),
                authoredData);
        }
        else {
            // Fallback
            for(size_t i = 0; i < numVerts; ++ i) {
                result[i] = 1.0;
            }
            TF_WARN("Incorrect number of primvar data, using default 1.0 for rendering.");
        }

        return result;
    }

    //! Helper utility function to adapt Maya API changes.
    void setWantConsolidation(MHWRender::MRenderItem& renderItem, bool state)
    {
#if MAYA_API_VERSION >= 20190000
        renderItem.setWantConsolidation(state);
#else
        renderItem.setWantSubSceneConsolidation(state);
#endif
    }
} // anonymous namespace

//! \brief  Constructor
HdVP2BasisCurves::HdVP2BasisCurves(
    HdVP2RenderDelegate *delegate,
    SdfPath const &id,
    SdfPath const &instancerId)
: HdBasisCurves(id, instancerId)
, _delegate(delegate)
, _rprimId(id.GetText())
{
    const MHWRender::MVertexBufferDescriptor desc(
        "",
        MHWRender::MGeometry::kPosition,
        MHWRender::MGeometry::kFloat,
        3);

    _curvesSharedData._positionsBuffer.reset(new MHWRender::MVertexBuffer(desc));
}

//! \brief  Synchronize VP2 state with scene delegate state based on dirty bits and repr
void HdVP2BasisCurves::Sync(
    HdSceneDelegate *delegate,
    HdRenderParam   *renderParam,
    HdDirtyBits     *dirtyBits,
    TfToken const   &reprToken)
{
    // We don't create a repr for the selection token because this token serves
    // for selection state update only. Return early to reserve dirty bits so
    // they can be used to sync regular reprs later.
    if (reprToken == HdVP2ReprTokens->selection) {
        return;
    }

    // We don't update the repr if it is hidden by the render tags (purpose)
    // of the ProxyRenderDelegate. In additional, we need to hide any already
    // existing render items because they should not be drawn.
    auto* const          param = static_cast<HdVP2RenderParam*>(_delegate->GetRenderParam());
    ProxyRenderDelegate& drawScene = param->GetDrawScene();
    if (!drawScene.DrawRenderTag(delegate->GetRenderIndex().GetRenderTag(GetId()))) {
        _HideAllDrawItems(reprToken);
        *dirtyBits &= ~( HdChangeTracker::DirtyRenderTag
#ifdef ENABLE_RENDERTAG_VISIBILITY_WORKAROUND
         | HdChangeTracker::DirtyVisibility
#endif
          );
        return;
    }

    MProfilingScope profilingScope(HdVP2RenderDelegate::sProfilerCategory,
        MProfiler::kColorC_L2, _rprimId.asChar(), "HdVP2BasisCurves::Sync");

    const SdfPath& id = GetId();

    if (*dirtyBits & HdChangeTracker::DirtyMaterialId) {
        _SetMaterialId(delegate->GetRenderIndex().GetChangeTracker(),
            delegate->GetMaterialId(id));
    }

    if (HdChangeTracker::IsPrimvarDirty(*dirtyBits, id, HdTokens->normals) ||
        HdChangeTracker::IsPrimvarDirty(*dirtyBits, id, HdTokens->primvar)) {
        const HdVP2Material* material = static_cast<const HdVP2Material*>(
            delegate->GetRenderIndex().GetSprim(
                HdPrimTypeTokens->material, GetMaterialId())
        );

        const TfTokenVector& requiredPrimvars =
            (material && material->GetSurfaceShader()) ?
            material->GetRequiredPrimvars() : sFallbackShaderPrimvars;

        _UpdatePrimvarSources(delegate, *dirtyBits, requiredPrimvars);
    }

    if (*dirtyBits & HdChangeTracker::DirtyDisplayStyle) {
        _curvesSharedData._displayStyle = GetDisplayStyle(delegate);
    }

    if (HdChangeTracker::IsTopologyDirty(*dirtyBits, id)) {
        _curvesSharedData._topology = GetBasisCurvesTopology(delegate);
    }

    // Prepare position buffer. It is shared among all draw items so it should
    // be updated only once when it gets dirty.
    if (HdChangeTracker::IsPrimvarDirty(*dirtyBits, id, HdTokens->points)) {
        const VtValue value = delegate->Get(id, HdTokens->points);
        _curvesSharedData._points = value.Get<VtVec3fArray>();

        const size_t numVertices = _curvesSharedData._points.size();

        const HdBasisCurvesTopology& topology = _curvesSharedData._topology;
        const size_t numControlPoints = topology.CalculateNeededNumberOfControlPoints();

        if (!topology.HasIndices() && numVertices != numControlPoints) {
            TF_WARN("Topology and vertices do not match for BasisCurve %s",
                id.GetName().c_str());
        }

        void* bufferData = _curvesSharedData._positionsBuffer->acquire(numVertices, true);
        if (bufferData) {
            const size_t numBytes = sizeof(GfVec3f) * numVertices;
            memcpy(bufferData, _curvesSharedData._points.cdata(), numBytes);

            // Capture class member for lambda
            MHWRender::MVertexBuffer* const positionsBuffer =
                _curvesSharedData._positionsBuffer.get();
            const MString& rprimId = _rprimId;

            _delegate->GetVP2ResourceRegistry().EnqueueCommit(
                [positionsBuffer, bufferData, rprimId]() {
                    MProfilingScope profilingScope(
                        HdVP2RenderDelegate::sProfilerCategory,
                        MProfiler::kColorC_L2,
                        rprimId.asChar(),
                        "CommitPositions");

                    positionsBuffer->commit(bufferData);
                }
            );
        }
    }

    if (HdChangeTracker::IsExtentDirty(*dirtyBits, id)) {
        _sharedData.bounds.SetRange(delegate->GetExtent(id));
    }

    if (HdChangeTracker::IsTransformDirty(*dirtyBits, id)) {
        _sharedData.bounds.SetMatrix(delegate->GetTransform(id));
    }

    if (HdChangeTracker::IsVisibilityDirty(*dirtyBits, id)) {
        _sharedData.visible = delegate->GetVisible(id);
    }

    if (*dirtyBits & (HdChangeTracker::DirtyRenderTag
#ifdef ENABLE_RENDERTAG_VISIBILITY_WORKAROUND
        |HdChangeTracker::DirtyVisibility
#endif
    )) {
        _curvesSharedData._renderTag = delegate->GetRenderTag(id);
    }

    *dirtyBits = HdChangeTracker::Clean;

    // Draw item update is controlled by its own dirty bits.
    _UpdateRepr(delegate, reprToken);
}

/*! \brief  Update the draw item

    This call happens on worker threads and results of the change are collected
    in CommitState and enqueued for Commit on main-thread using CommitTasks
*/
void
HdVP2BasisCurves::_UpdateDrawItem(
    HdSceneDelegate *sceneDelegate,
    HdVP2DrawItem *drawItem,
    HdBasisCurvesReprDesc const &desc)
{
    const MHWRender::MRenderItem* renderItem = drawItem->GetRenderItem();
    if (ARCH_UNLIKELY(!renderItem)) {
        return;
    }

    const HdDirtyBits itemDirtyBits = drawItem->GetDirtyBits();

    CommitState stateToCommit(*drawItem);
    HdVP2DrawItem::RenderItemData& drawItemData = stateToCommit._drawItemData;

    const SdfPath& id = GetId();

    auto* const param = static_cast<HdVP2RenderParam*>(_delegate->GetRenderParam());
    ProxyRenderDelegate& drawScene = param->GetDrawScene();

    const HdRenderIndex& renderIndex = sceneDelegate->GetRenderIndex();

    const auto& primvarSourceMap = _curvesSharedData._primvarSourceMap;

    const HdBasisCurvesTopology& topology = _curvesSharedData._topology;
    const TfToken type = topology.GetCurveType();
    const TfToken wrap = topology.GetCurveWrap();
    const TfToken basis = topology.GetCurveBasis();

#if defined(HDVP2_ENABLE_BASISCURVES_TESSELLATION)
    const int refineLevel = _curvesSharedData._displayStyle.refineLevel;
#else
    const int refineLevel = 0;
#endif

    const MHWRender::MGeometry::DrawMode drawMode = renderItem->drawMode();

    // The bounding box item uses a globally-shared geometry data therefore it
    // doesn't need to extract index data from topology. Points use non-indexed
    // draw.
    const bool isBoundingBoxItem = (drawMode == MHWRender::MGeometry::kBoundingBox);
    const bool isPointSnappingItem = (renderItem->primitive() == MHWRender::MGeometry::kPoints);
    const bool requiresIndexUpdate = !isBoundingBoxItem && !isPointSnappingItem;

    // Prepare index buffer.
    if (requiresIndexUpdate && (itemDirtyBits & HdChangeTracker::DirtyTopology)) {

        const bool forceLines =
            (refineLevel <= 0) || (drawMode == MHWRender::MGeometry::kWireframe);

        VtValue result;

        if (!forceLines && type == HdTokens->cubic) {
            result = _BuildCubicIndexArray(topology);
        }
        else if (wrap == HdTokens->segmented) {
            result = _BuildLinesIndexArray(topology);
        }
        else {
            result = _BuildLineSegmentIndexArray(topology);
        }

        const void* indexData = nullptr;
        unsigned int numIndices = 0;

        if (result.IsHolding<VtVec2iArray>()) {
            indexData = result.UncheckedGet<VtVec2iArray>().cdata();
            numIndices = result.GetArraySize() * 2;
        }
        else if (result.IsHolding<VtVec4iArray>()) {
            indexData = result.UncheckedGet<VtVec4iArray>().cdata();
            numIndices = result.GetArraySize() * 4;
        }

        if (drawItemData._indexBuffer && numIndices > 0) {
            stateToCommit._indexBufferData = static_cast<int*>(
                drawItemData._indexBuffer->acquire(numIndices, true));

            if (indexData != nullptr && stateToCommit._indexBufferData != nullptr) {
                memcpy(stateToCommit._indexBufferData, indexData,
                    numIndices * sizeof(int));
            }
        }
    }

    if (desc.geomStyle == HdBasisCurvesGeomStylePatch) {
        // Prepare normals buffer.
        if (itemDirtyBits &
            (HdChangeTracker::DirtyNormals | HdChangeTracker::DirtyDisplayStyle)) {
            VtVec3fArray normals;

            const auto it = primvarSourceMap.find(HdTokens->normals);
            if (it != primvarSourceMap.end()) {
                const VtValue& value = it->second.data;
                if (ARCH_LIKELY(value.IsHolding<VtVec3fArray>())) {
                    normals = value.UncheckedGet<VtVec3fArray>();
                }
            }

            // Using a zero vector to indicate requirement of camera-facing
            // normals when there is no authored normals.
            if (normals.empty()) {
                normals.push_back(GfVec3f(0.0f, 0.0f, 0.0f));
            }

            normals = _BuildInterpolatedArray(topology, normals);

            if (!drawItemData._normalsBuffer) {
                const MHWRender::MVertexBufferDescriptor vbDesc("",
                    MHWRender::MGeometry::kNormal,
                    MHWRender::MGeometry::kFloat,
                    3);

                drawItemData._normalsBuffer.reset(
                    new MHWRender::MVertexBuffer(vbDesc));
            }

            unsigned int numNormals = normals.size();
            if (drawItemData._normalsBuffer && numNormals > 0) {
                stateToCommit._normalsBufferData =
                    drawItemData._normalsBuffer->acquire(numNormals, true);

                if (stateToCommit._normalsBufferData != nullptr) {
                    memcpy(stateToCommit._normalsBufferData, normals.cdata(),
                        numNormals * sizeof(GfVec3f));
                }
            }
        }

        // Prepare widths buffer.
        if ((refineLevel > 0) &&
            (itemDirtyBits & (HdChangeTracker::DirtyPrimvar |
                              HdChangeTracker::DirtyDisplayStyle))) {
            VtFloatArray widths;

            const auto it = primvarSourceMap.find(HdTokens->widths);
            if (it != primvarSourceMap.end()) {
                const VtValue& value = it->second.data;
                if (value.IsHolding<VtFloatArray>()) {
                    widths = value.UncheckedGet<VtFloatArray>();
                }
            }

            if (widths.empty()) {
                widths.push_back(1.0f);
            }

            widths = _BuildInterpolatedArray(topology, widths);

            MHWRender::MVertexBuffer* widthsBuffer =
                drawItemData._primvarBuffers[HdTokens->widths].get();

            if (!widthsBuffer) {
                const MHWRender::MVertexBufferDescriptor vbDesc("",
                    MHWRender::MGeometry::kTexture,
                    MHWRender::MGeometry::kFloat,
                    1);

                widthsBuffer = new MHWRender::MVertexBuffer(vbDesc);
                drawItemData._primvarBuffers[HdTokens->widths].reset(widthsBuffer);
            }

            unsigned int numWidths = widths.size();
            if (widthsBuffer && numWidths > 0) {
                void* bufferData = widthsBuffer->acquire(numWidths, true);
                stateToCommit._primvarBufferDataMap[HdTokens->widths] = bufferData;

                if (bufferData != nullptr) {
                    memcpy(bufferData, widths.cdata(), numWidths * sizeof(float));
                }
            }
        }

        // Prepare color buffer.
        if (itemDirtyBits & (HdChangeTracker::DirtyMaterialId |
                             DirtySelectionHighlight)) {
            const HdVP2Material* material = static_cast<const HdVP2Material*>(
                renderIndex.GetSprim(HdPrimTypeTokens->material, GetMaterialId())
            );

            if (material) {
                MHWRender::MShaderInstance* shader = material->GetSurfaceShader();
                if (shader != nullptr && shader != drawItemData._shader) {
                    drawItemData._shader = shader;
                    stateToCommit._shader = shader;
                    stateToCommit._isTransparent = shader->isTransparent();
                }

                auto primitiveType = MHWRender::MGeometry::kLines;
                int primitiveStride = 0;

                if (primitiveType != drawItemData._primitiveType ||
                    primitiveStride != drawItemData._primitiveStride) {

                    drawItemData._primitiveType = primitiveType;
                    stateToCommit._primitiveType = &drawItemData._primitiveType;

                    drawItemData._primitiveStride = primitiveStride;
                    stateToCommit._primitiveStride = &drawItemData._primitiveStride;
                }
            }
        }

        if (itemDirtyBits & (HdChangeTracker::DirtyPrimvar |
                             HdChangeTracker::DirtyDisplayStyle |
                             DirtySelectionHighlight)) {
            VtVec3fArray colorArray;
            VtFloatArray alphaArray;

            auto itColor = primvarSourceMap.find(HdTokens->displayColor);
            if (itColor != primvarSourceMap.end()) {
                const VtValue& value = itColor->second.data;
                if (value.IsHolding<VtVec3fArray>() && value.GetArraySize() > 0) {
                    colorArray = value.UncheckedGet<VtVec3fArray>();
                }
            }

            auto itOpacity = primvarSourceMap.find(HdTokens->displayOpacity);
            if (itOpacity != primvarSourceMap.end()) {
                const VtValue& value = itOpacity->second.data;
                if (value.IsHolding<VtFloatArray>() && value.GetArraySize() > 0) {
                    alphaArray = value.UncheckedGet<VtFloatArray>();

                    // It is possible that all elements in the opacity array are 1.
                    // Due to the performance indication about transparency, we have to
                    // traverse the array and enable transparency only when needed.
                    if (!stateToCommit._isTransparent) {
                        const size_t numAlphas = alphaArray.size();
                        for (size_t i = 0; i < numAlphas; i++) {
                            if (alphaArray[i] < 0.999f) {
                                stateToCommit._isTransparent = true;
                                break;
                            }
                        }
                    }
                }
            }

            // If color/opacity is not found, the 18% gray color will be used
            // to match the default color of Hydra Storm.
            if (colorArray.empty()) {
                colorArray.push_back(GfVec3f(0.18f, 0.18f, 0.18f));
            }

            if (alphaArray.empty()) {
                alphaArray.push_back(1.0f);
            }

            if (colorArray.size() == 1 && alphaArray.size() == 1) {
                // Use fallback shader if there is no material binding or we
                // failed to create a shader instance from the material.
                if (!stateToCommit._shader) {
                    const GfVec3f& color = colorArray[0];
                    const MColor clr(color[0], color[1], color[2], alphaArray[0]);

                    MHWRender::MShaderInstance* shader;
                    MHWRender::MGeometry::Primitive primitiveType;
                    int primitiveStride;

                    if (refineLevel <= 0) {
                        shader = _delegate->Get3dSolidShader(clr);
                        primitiveType = MHWRender::MGeometry::kLines;
                        primitiveStride = 0;
                    } else {
                        shader = _delegate->GetBasisCurvesFallbackShader(type, basis, clr);
                        primitiveType = MHWRender::MGeometry::kPatch;
                        primitiveStride = (type == HdTokens->linear ? 2 : 4);
                    }

                    if (shader != nullptr && shader != drawItemData._shader) {
                        drawItemData._shader = shader;
                        stateToCommit._shader = shader;
                    }

                    if (primitiveType != drawItemData._primitiveType ||
                        primitiveStride != drawItemData._primitiveStride) {
                        drawItemData._primitiveType = primitiveType;
                        stateToCommit._primitiveType = &drawItemData._primitiveType;

                        drawItemData._primitiveStride = primitiveStride;
                        stateToCommit._primitiveStride = &drawItemData._primitiveStride;
                    }
                }
            }
            else {
                colorArray = _BuildInterpolatedArray(topology, colorArray);
                alphaArray = _BuildInterpolatedArray(topology, alphaArray);

                const size_t numColors = colorArray.size();
                const size_t numAlphas = alphaArray.size();
                const size_t numVertices = std::min(numColors, numAlphas);

                if (numColors != numAlphas) {
                    TF_CODING_ERROR("color and opacity do not match for BasisCurve %s",
                        id.GetName().c_str());
                }

                // Fill color and opacity into the float4 color stream.
                if (!drawItemData._colorBuffer) {
                    const MHWRender::MVertexBufferDescriptor vbDesc("",
                        MHWRender::MGeometry::kColor,
                        MHWRender::MGeometry::kFloat,
                        4);

                    drawItemData._colorBuffer.reset(
                        new MHWRender::MVertexBuffer(vbDesc));
                }

                float* bufferData = static_cast<float*>(
                    drawItemData._colorBuffer->acquire(numVertices, true));

                if (bufferData) {
                    unsigned int offset = 0;
                    for (size_t v = 0; v < numVertices; v++) {
                        const GfVec3f& color = colorArray[v];
                        bufferData[offset++] = color[0];
                        bufferData[offset++] = color[1];
                        bufferData[offset++] = color[2];

                        bufferData[offset++] = alphaArray[v];
                    }

                    stateToCommit._colorBufferData = bufferData;
                }

                // Use 3d CPV solid-color shader if there is no material binding or
                // we failed to create a shader instance from the material.
                if (!stateToCommit._shader) {
                    MHWRender::MShaderInstance* shader;
                    MHWRender::MGeometry::Primitive primitiveType;
                    int primitiveStride;

                    if (refineLevel <= 0) {
                        shader = _delegate->Get3dCPVSolidShader();
                        primitiveType = MHWRender::MGeometry::kLines;
                        primitiveStride = 0;
                    } else {
                        shader = _delegate->GetBasisCurvesCPVShader(type, basis);
                        primitiveType = MHWRender::MGeometry::kPatch;
                        primitiveStride = (type == HdTokens->linear ? 2 : 4);
                    }

                    if (shader != nullptr && shader != drawItemData._shader) {
                        drawItemData._shader = shader;
                        stateToCommit._shader = shader;
                    }

                    if (primitiveType != drawItemData._primitiveType ||
                        primitiveStride != drawItemData._primitiveStride) {

                        drawItemData._primitiveType = primitiveType;
                        stateToCommit._primitiveType = &drawItemData._primitiveType;

                        drawItemData._primitiveStride = primitiveStride;
                        stateToCommit._primitiveStride = &drawItemData._primitiveStride;
                    }
                }
            }
        }
    }

    // Local bounds
    const GfRange3d& range = _sharedData.bounds.GetRange();

    // Bounds are updated through MPxSubSceneOverride::setGeometryForRenderItem()
    // which is expensive, so it is updated only when it gets expanded in order
    // to reduce calling frequence.
    if (itemDirtyBits & HdChangeTracker::DirtyExtent) {
        const GfRange3d& rangeToUse
            = isBoundingBoxItem ? _delegate->GetSharedBBoxGeom().GetRange() : range;

        bool boundingBoxExpanded = false;

        const GfVec3d& min = rangeToUse.GetMin();
        const MPoint pntMin(min[0], min[1], min[2]);
        if (!drawItemData._boundingBox.contains(pntMin)) {
            drawItemData._boundingBox.expand(pntMin);
            boundingBoxExpanded = true;
        }

        const GfVec3d& max = rangeToUse.GetMax();
        const MPoint pntMax(max[0], max[1], max[2]);
        if (!drawItemData._boundingBox.contains(pntMax)) {
            drawItemData._boundingBox.expand(pntMax);
            boundingBoxExpanded = true;
        }

        if (boundingBoxExpanded) {
            stateToCommit._boundingBox = &drawItemData._boundingBox;
        }
    }

    // Local-to-world transformation
    MMatrix& worldMatrix = drawItemData._worldMatrix;
    _sharedData.bounds.GetMatrix().Get(worldMatrix.matrix);

    // The bounding box draw item uses a globally-shared unit wire cube as the
    // geometry and transfers scale and offset of the bounds to world matrix.
    if (isBoundingBoxItem) {
        if ((itemDirtyBits & (HdChangeTracker::DirtyExtent | HdChangeTracker::DirtyTransform)) &&
            !range.IsEmpty()) {
            const GfVec3d midpoint = range.GetMidpoint();
            const GfVec3d size = range.GetSize();

            MPoint midp(midpoint[0], midpoint[1], midpoint[2]);
            midp *= worldMatrix;

            auto& m = worldMatrix.matrix;
            m[0][0] *= size[0]; m[0][1] *= size[0]; m[0][2] *= size[0]; m[0][3] *= size[0];
            m[1][0] *= size[1]; m[1][1] *= size[1]; m[1][2] *= size[1]; m[1][3] *= size[1];
            m[2][0] *= size[2]; m[2][1] *= size[2]; m[2][2] *= size[2]; m[2][3] *= size[2];
            m[3][0]  = midp[0]; m[3][1]  = midp[1]; m[3][2]  = midp[2]; m[3][3]  = midp[3];

            stateToCommit._worldMatrix = &drawItemData._worldMatrix;
        }
    }
    else if (itemDirtyBits & HdChangeTracker::DirtyTransform) {
        stateToCommit._worldMatrix = &drawItemData._worldMatrix;
    }

    // If the prim is instanced, create one new instance per transform.
    // The current instancer invalidation tracking makes it hard for
    // us to tell whether transforms will be dirty, so this code
    // pulls them every time something changes.
    // If the mesh is instanced but has 0 instance transforms remember that
    // so the render item can be hidden.
    bool instancerWithNoInstances = false;
    if (!GetInstancerId().IsEmpty()) {

        // Retrieve instance transforms from the instancer.
        HdInstancer *instancer = renderIndex.GetInstancer(GetInstancerId());
        VtMatrix4dArray transforms = static_cast<HdVP2Instancer*>(instancer)->
            ComputeInstanceTransforms(id);

        MMatrix instanceMatrix;
        const unsigned int instanceCount = transforms.size();

        if (0 == instanceCount) {
            instancerWithNoInstances = true;
        }
        else {
            stateToCommit._instanceTransforms.setLength(instanceCount);
            for (unsigned int i = 0; i < instanceCount; ++i) {
                transforms[i].Get(instanceMatrix.matrix);
                stateToCommit._instanceTransforms[i] = worldMatrix * instanceMatrix;
            }

            // If the item is used for both regular draw and selection highlight,
            // it needs to display both wireframe color and selection highlight
            // with one color vertex buffer.
            if (drawItem->ContainsUsage(HdVP2DrawItem::kSelectionHighlight)) {
                const MColor colors[] = {
                    drawScene.GetWireframeColor(),
                    drawScene.GetSelectionHighlightColor(false),
                    drawScene.GetSelectionHighlightColor(true)
                };

                // Store the indices to colors.
                std::vector<unsigned char> colorIndices;

                // Assign with the index to the dormant wireframe color by default.
                colorIndices.resize(instanceCount, 0);

                // Assign with the index to the active selection highlight color.
                if (auto state = drawScene.GetActiveSelectionState(id)) {
                    for (const auto& indexArray : state->instanceIndices) {
                        for (const auto index : indexArray) {
                            colorIndices[index] = 1;
                        }
                    }
                }

                // Assign with the index to the lead selection highlight color.
                if (auto state = drawScene.GetLeadSelectionState(id)) {
                    for (const auto& indexArray : state->instanceIndices) {
                        for (const auto index : indexArray) {
                            colorIndices[index] = 2;
                        }
                    }
                }

                // Fill per-instance colors.
                stateToCommit._instanceColors.setLength(instanceCount * kNumColorChannels);
                unsigned int offset = 0;

                for (unsigned int i = 0; i < instanceCount; ++i) {
                    unsigned char colorIndex = colorIndices[i];
                    const MColor& color = colors[colorIndex];
                    for (unsigned int j = 0; j < kNumColorChannels; j++) {
                        stateToCommit._instanceColors[offset++] = color[j];
                    }
                }
            }
        }
    }
    else {
        // Non-instanced Rprims.
        if (itemDirtyBits & (DirtySelectionHighlight | HdChangeTracker::DirtyDisplayStyle)) {
            if (drawItem->ContainsUsage(HdVP2DrawItem::kRegular) &&
                drawItem->ContainsUsage(HdVP2DrawItem::kSelectionHighlight)) {
                MHWRender::MShaderInstance* shader = nullptr;

                auto primitiveType = MHWRender::MGeometry::kLines;
                int primitiveStride = 0;

                const MColor& color = (_selectionStatus != kUnselected ?
                    drawScene.GetSelectionHighlightColor(_selectionStatus == kFullyLead) :
                    drawScene.GetWireframeColor());

                if (desc.geomStyle == HdBasisCurvesGeomStylePatch) {
                    if (_selectionStatus != kUnselected) {
                        if (refineLevel <= 0) {
                            shader = _delegate->Get3dSolidShader(color);
                        } else {
                            shader = _delegate->GetBasisCurvesFallbackShader(type, basis, color);
                            primitiveType = MHWRender::MGeometry::kPatch;
                            primitiveStride = (type == HdTokens->linear ? 2 : 4);
                        }
                    }
                }
                else {
                    shader = _delegate->Get3dSolidShader(color);
                }

                if (shader != nullptr && shader != drawItemData._shader) {
                    drawItemData._shader = shader;
                    stateToCommit._shader = shader;
                    stateToCommit._isTransparent = false;

                    if (primitiveType != drawItemData._primitiveType ||
                        primitiveStride != drawItemData._primitiveStride) {
                        drawItemData._primitiveType = primitiveType;
                        stateToCommit._primitiveType = &drawItemData._primitiveType;

                        drawItemData._primitiveStride = primitiveStride;
                        stateToCommit._primitiveStride = &drawItemData._primitiveStride;
                    }
                }
            }
        }
    }

    // Determine if the render item should be enabled or not.
    if (!GetInstancerId().IsEmpty() ||
        (itemDirtyBits & (HdChangeTracker::DirtyVisibility |
                         HdChangeTracker::DirtyRenderTag |
                         HdChangeTracker::DirtyPoints |
                         HdChangeTracker::DirtyExtent |
                         DirtySelectionHighlight))) {
        bool enable = drawItem->GetVisible() && !_curvesSharedData._points.empty() && !instancerWithNoInstances;

        if (isPointSnappingItem) {
            enable = enable && (_selectionStatus == kUnselected);
        } else if (isBoundingBoxItem) {
            enable = enable && !range.IsEmpty();
        }

        enable = enable && drawScene.DrawRenderTag(_curvesSharedData._renderTag);

        if (drawItemData._enabled != enable) {
            drawItemData._enabled = enable;
            stateToCommit._enabled = &drawItemData._enabled;
        }
    }

    stateToCommit._geometryDirty = (itemDirtyBits & (
        HdChangeTracker::DirtyPoints |
        HdChangeTracker::DirtyNormals |
        HdChangeTracker::DirtyPrimvar |
        HdChangeTracker::DirtyTopology |
        DirtySelectionHighlight));

    // Reset dirty bits because we've prepared commit state for this draw item.
    drawItem->ResetDirtyBits();

    // Capture the valid position buffer and index buffer
    MHWRender::MVertexBuffer* positionsBuffer = _curvesSharedData._positionsBuffer.get();
    MHWRender::MIndexBuffer* indexBuffer = drawItemData._indexBuffer.get();

    if (isBoundingBoxItem) {
        const HdVP2BBoxGeom& sharedBBoxGeom = _delegate->GetSharedBBoxGeom();
        positionsBuffer = const_cast<MHWRender::MVertexBuffer*>(
            sharedBBoxGeom.GetPositionBuffer());
        indexBuffer = const_cast<MHWRender::MIndexBuffer*>(
            sharedBBoxGeom.GetIndexBuffer());
    }

    _delegate->GetVP2ResourceRegistry().EnqueueCommit(
        [drawItem, stateToCommit, param, positionsBuffer, indexBuffer]()
    {
        MHWRender::MRenderItem* renderItem = drawItem->GetRenderItem();
        if (ARCH_UNLIKELY(!renderItem))
            return;

        MProfilingScope profilingScope(HdVP2RenderDelegate::sProfilerCategory,
            MProfiler::kColorC_L2, drawItem->GetRenderItemName().asChar(), "Commit");

        const HdVP2DrawItem::RenderItemData& drawItemData = stateToCommit._drawItemData;

        MHWRender::MVertexBuffer* colorBuffer = drawItemData._colorBuffer.get();
        MHWRender::MVertexBuffer* normalsBuffer = drawItemData._normalsBuffer.get();

        const HdVP2DrawItem::PrimvarBufferMap& primvarBuffers = drawItemData._primvarBuffers;

        // If available, something changed
        if (stateToCommit._colorBufferData)
            colorBuffer->commit(stateToCommit._colorBufferData);

        // If available, something changed
        if (stateToCommit._normalsBufferData)
            normalsBuffer->commit(stateToCommit._normalsBufferData);

        // If available, something changed
        for (const auto& entry : stateToCommit._primvarBufferDataMap) {
            const TfToken& primvarName = entry.first;
            void* primvarBufferData = entry.second;
            if (primvarBufferData) {
                const auto it = primvarBuffers.find(primvarName);
                if (it != primvarBuffers.end()) {
                    MHWRender::MVertexBuffer* primvarBuffer = it->second.get();
                    if (primvarBuffer) {
                        primvarBuffer->commit(primvarBufferData);
                    }
                }
            }
        }

        // If available, something changed
        if (stateToCommit._indexBufferData)
            indexBuffer->commit(stateToCommit._indexBufferData);

        // If available, something changed
        if (stateToCommit._shader != nullptr) {
            renderItem->setShader(stateToCommit._shader);
            renderItem->setTreatAsTransparent(stateToCommit._isTransparent);
        }

        // If the enable state is changed, then update it.
        if (stateToCommit._enabled != nullptr) {
            renderItem->enable(*stateToCommit._enabled);
        }

#if defined(HDVP2_ENABLE_BASISCURVES_TESSELLATION)
        // If the primitive type and stride are changed, then update them.
        if (stateToCommit._primitiveType != nullptr &&
            stateToCommit._primitiveStride != nullptr) {
            auto primitive = *stateToCommit._primitiveType;
            int stride = *stateToCommit._primitiveStride;
            renderItem->setPrimitive(primitive, stride);

            const bool wantConsolidation =
                !stateToCommit._drawItemData._usingInstancedDraw &&
                primitive != MHWRender::MGeometry::kPatch;
            setWantConsolidation(*renderItem, wantConsolidation);
        }
#endif

        ProxyRenderDelegate& drawScene = param->GetDrawScene();

        if (stateToCommit._geometryDirty || stateToCommit._boundingBox) {
            MHWRender::MVertexBufferArray vertexBuffers;
            vertexBuffers.addBuffer(kPositionsStr, positionsBuffer);

            if (colorBuffer)
                vertexBuffers.addBuffer(kDiffuseColorStr, colorBuffer);

            if (normalsBuffer)
                vertexBuffers.addBuffer(kNormalsStr, normalsBuffer);

            for (auto& entry : primvarBuffers) {
                const TfToken& primvarName = entry.first;
                MHWRender::MVertexBuffer* primvarBuffer = entry.second.get();
                if (primvarBuffer) {
                    vertexBuffers.addBuffer(primvarName.GetText(), primvarBuffer);
                }
            }

            // The API call does three things:
            // - Associate geometric buffers with the render item.
            // - Update bounding box.
            // - Trigger consolidation/instancing update.
            drawScene.setGeometryForRenderItem(*renderItem,
                vertexBuffers, *indexBuffer, stateToCommit._boundingBox);
        }

        // Important, update instance transforms after setting geometry on render items!
        auto& oldInstanceCount = stateToCommit._drawItemData._instanceCount;
        auto newInstanceCount = stateToCommit._instanceTransforms.length();

        // GPU instancing has been enabled. We cannot switch to consolidation
        // without recreating render item, so we keep using GPU instancing.
        if (stateToCommit._drawItemData._usingInstancedDraw) {
            if (oldInstanceCount == newInstanceCount) {
                for (unsigned int i = 0; i < newInstanceCount; i++) {
                    // VP2 defines instance ID of the first instance to be 1.
                    drawScene.updateInstanceTransform(*renderItem,
                        i+1, stateToCommit._instanceTransforms[i]);
                }
            } else {
                drawScene.setInstanceTransformArray(*renderItem,
                    stateToCommit._instanceTransforms);
            }

            if (stateToCommit._instanceColors.length() ==
                newInstanceCount * kNumColorChannels) {
                drawScene.setExtraInstanceData(*renderItem,
                    kSolidColorStr, stateToCommit._instanceColors);
            }
        }
#if MAYA_API_VERSION >= 20210000
        else if (newInstanceCount >= 1) {
#else
        // In Maya 2020 and before, GPU instancing and consolidation are two separate systems that
        // cannot be used by a render item at the same time. In case of single instance, we keep
        // the original render item to allow consolidation with other prims. In case of multiple
        // instances, we need to disable consolidation to allow GPU instancing to be used.
        else if (newInstanceCount == 1) {
            renderItem->setMatrix(&stateToCommit._instanceTransforms[0]);
        }
        else if (newInstanceCount > 1) {
            setWantConsolidation(*renderItem, false);
#endif
            drawScene.setInstanceTransformArray(*renderItem,
                stateToCommit._instanceTransforms);

            if (stateToCommit._instanceColors.length() ==
                newInstanceCount * kNumColorChannels) {
                drawScene.setExtraInstanceData(*renderItem,
                    kSolidColorStr, stateToCommit._instanceColors);
            }

            stateToCommit._drawItemData._usingInstancedDraw = true;
        }
        else if (stateToCommit._worldMatrix != nullptr) {
            // Regular non-instanced prims. Consolidation has been turned on by
            // default and will be kept enabled on this case.
            renderItem->setMatrix(stateToCommit._worldMatrix);
        }

        oldInstanceCount = newInstanceCount;
    });
}

/*! \brief  Add additional dirty bits

    This callback from Rprim gives the prim an opportunity to set
    additional dirty bits based on those already set.  This is done
    before the dirty bits are passed to the scene delegate, so can be
    used to communicate that extra information is needed by the prim to
    process the changes.

    The return value is the new set of dirty bits, which replaces the bits
    passed in.

    See HdRprim::PropagateRprimDirtyBits()
*/
HdDirtyBits HdVP2BasisCurves::_PropagateDirtyBits(HdDirtyBits bits) const
{
    // Propagate dirty bits to all draw items.
    for (const std::pair<TfToken, HdReprSharedPtr>& pair : _reprs) {
        const HdReprSharedPtr& repr = pair.second;
        const auto& items = repr->GetDrawItems();
#if HD_API_VERSION < 35
        for (HdDrawItem* item : items) {
            if (HdVP2DrawItem* drawItem = static_cast<HdVP2DrawItem*>(item)) {
#else
        for (const HdRepr::DrawItemUniquePtr &item : items) {
            if (HdVP2DrawItem * const drawItem =
                        static_cast<HdVP2DrawItem*>(item.get())) {
#endif
                drawItem->SetDirtyBits(bits);
            }
        }
    }

    return bits;
}

/*! \brief  Initialize the given representation of this Rprim.

    This is called prior to syncing the prim, the first time the repr
    is used.

    \param  reprToken   the name of the repr to initalize.  HdRprim has already
                        resolved the reprName to its final value.

    \param  dirtyBits   an in/out value.  It is initialized to the dirty bits
                        from the change tracker.  InitRepr can then set additional
                        dirty bits if additional data is required from the scene
                        delegate when this repr is synced.

    InitRepr occurs before dirty bit propagation.

    See HdRprim::InitRepr()
*/
void HdVP2BasisCurves::_InitRepr(TfToken const &reprToken, HdDirtyBits *dirtyBits)
{
    auto* const param = static_cast<HdVP2RenderParam*>(_delegate->GetRenderParam());
    MSubSceneContainer* subSceneContainer = param->GetContainer();
    if (ARCH_UNLIKELY(!subSceneContainer))
        return;

    // Update selection state on demand or when it is a new Rprim. DirtySelection
    // will be propagated to all draw items, to trigger sync for each repr.
    if (reprToken == HdVP2ReprTokens->selection || _reprs.empty()) {
        const HdVP2SelectionStatus selectionStatus =
            param->GetDrawScene().GetSelectionStatus(GetId());
        if (_selectionStatus != selectionStatus) {
            _selectionStatus = selectionStatus;
            *dirtyBits |= DirtySelection;
        }
        else if (_selectionStatus == kPartiallySelected) {
            *dirtyBits |= DirtySelection;
        }

        // We don't create a repr for the selection token because it serves for
        // selection state update only. Return from here.
        if (reprToken == HdVP2ReprTokens->selection)
            return;
    }

    // If the repr has any draw item with the DirtySelection bit, mark the
    // DirtySelectionHighlight bit to invoke the synchronization call.
    _ReprVector::iterator it = std::find_if(
        _reprs.begin(), _reprs.end(), _ReprComparator(reprToken));
    if (it != _reprs.end()) {
        const HdReprSharedPtr& repr = it->second;
        const auto& items = repr->GetDrawItems();
#if HD_API_VERSION < 35
        for (const HdDrawItem* item : items) {
            const HdVP2DrawItem* drawItem = static_cast<const HdVP2DrawItem*>(item);
#else
        for (const HdRepr::DrawItemUniquePtr &item : items) {
            const HdVP2DrawItem * const drawItem =
                static_cast<const HdVP2DrawItem*>(item.get());
#endif
            if (drawItem && (drawItem->GetDirtyBits() & DirtySelection)) {
                *dirtyBits |= DirtySelectionHighlight;
                break;
            }
        }
        return;
    }

    // add new repr
#if USD_VERSION_NUM > 2002
    _reprs.emplace_back(reprToken, std::make_shared<HdRepr>());
#else
    _reprs.emplace_back(reprToken, boost::make_shared<HdRepr>());
#endif
    HdReprSharedPtr repr = _reprs.back().second;

    // set dirty bit to say we need to sync a new repr
    *dirtyBits |= HdChangeTracker::NewRepr;

    _BasisCurvesReprConfig::DescArray descs = _GetReprDesc(reprToken);

    for (size_t descIdx = 0; descIdx < descs.size(); ++descIdx) {
        const HdBasisCurvesReprDesc& desc = descs[descIdx];
        if (desc.geomStyle == HdBasisCurvesGeomStyleInvalid) {
            continue;
        }

#if HD_API_VERSION < 35
        auto* drawItem = new HdVP2DrawItem(_delegate, &_sharedData);
#else
        std::unique_ptr<HdVP2DrawItem> drawItem =
            std::make_unique<HdVP2DrawItem>(_delegate, &_sharedData);
#endif

        const MString& renderItemName = drawItem->GetRenderItemName();

        MHWRender::MRenderItem* renderItem = nullptr;

        switch (desc.geomStyle) {
        case HdBasisCurvesGeomStylePatch:
            renderItem = _CreatePatchRenderItem(renderItemName);
            drawItem->AddUsage(HdVP2DrawItem::kSelectionHighlight);
            break;
        case HdBasisCurvesGeomStyleWire:
            // The item is used for wireframe display and selection highlight.
            if (reprToken == HdReprTokens->wire) {
                renderItem = _CreateWireRenderItem(renderItemName);
                drawItem->AddUsage(HdVP2DrawItem::kSelectionHighlight);
            }
            // The item is used for bbox display and selection highlight.
            else if (reprToken == HdVP2ReprTokens->bbox) {
                renderItem = _CreateBBoxRenderItem(renderItemName);
                drawItem->AddUsage(HdVP2DrawItem::kSelectionHighlight);
            }
            break;
        case HdBasisCurvesGeomStylePoints:
            renderItem = _CreatePointsRenderItem(renderItemName);
            break;
        default:
            TF_WARN("Unsupported geomStyle");
            break;
        }

        if (renderItem) {
            // Store the render item pointer to avoid expensive lookup in the
            // subscene container.
            drawItem->SetRenderItem(renderItem);

            _delegate->GetVP2ResourceRegistry().EnqueueCommit(
                [subSceneContainer, renderItem]() {
                    subSceneContainer->add(renderItem);
                }
            );
        }
#if HD_API_VERSION < 35
        repr->AddDrawItem(drawItem);
#else
        repr->AddDrawItem(std::move(drawItem));
#endif
    }
}

/*! \brief  Update the named repr object for this Rprim.

    Repr objects are created to support specific reprName tokens, and contain a list of
    HdVP2DrawItems and corresponding RenderItems.
*/
void HdVP2BasisCurves::_UpdateRepr(
    HdSceneDelegate *sceneDelegate,
    TfToken const &reprToken)
{
    HdReprSharedPtr const &repr = _GetRepr(reprToken);
    if (!repr) {
        return;
    }

    const _BasisCurvesReprConfig::DescArray &descs = _GetReprDesc(reprToken);
    const size_t numDescs = descs.size();
    int drawItemIndex = 0;

    for (size_t i = 0; i < numDescs; ++i) {
        const HdBasisCurvesReprDesc &desc = descs[i];
        if (desc.geomStyle != HdBasisCurvesGeomStyleInvalid) {
            HdVP2DrawItem *drawItem = static_cast<HdVP2DrawItem*>(
                repr->GetDrawItem(drawItemIndex++));
            if (drawItem) {
                _UpdateDrawItem(sceneDelegate, drawItem, desc);
            }
        }
    }
}

/*! \brief  Returns the minimal set of dirty bits to place in the
            change tracker for use in the first sync of this prim.
*/
HdDirtyBits HdVP2BasisCurves::GetInitialDirtyBitsMask() const
{
    constexpr HdDirtyBits bits = 
        HdChangeTracker::InitRepr |
        HdChangeTracker::DirtyExtent |
        HdChangeTracker::DirtyInstanceIndex |
        HdChangeTracker::DirtyNormals |
        HdChangeTracker::DirtyPoints |
        HdChangeTracker::DirtyPrimID |
        HdChangeTracker::DirtyPrimvar |
        HdChangeTracker::DirtyDisplayStyle |
        HdChangeTracker::DirtyRepr |
        HdChangeTracker::DirtyMaterialId |
        HdChangeTracker::DirtyTopology |
        HdChangeTracker::DirtyTransform |
        HdChangeTracker::DirtyVisibility |
        HdChangeTracker::DirtyWidths |
        HdChangeTracker::DirtyComputationPrimvarDesc |
        HdChangeTracker::DirtyRenderTag |
        DirtySelectionHighlight;

    return bits;
}

void HdVP2BasisCurves::_HideAllDrawItems(const TfToken& reprToken) {
    HdReprSharedPtr const& curRepr = _GetRepr(reprToken);
    if (!curRepr) {
        return;
    }

    _BasisCurvesReprConfig::DescArray reprDescs = _GetReprDesc(reprToken);

    // For each relevant draw item, update dirty buffer sources.
    int drawItemIndex = 0;
    for (size_t descIdx = 0; descIdx < reprDescs.size(); ++descIdx) {
        const HdBasisCurvesReprDesc& desc = reprDescs[descIdx];
        if (desc.geomStyle == HdBasisCurvesGeomStyleInvalid) {
            continue;
        }

        auto* drawItem = static_cast<HdVP2DrawItem*>(curRepr->GetDrawItem(drawItemIndex++));
        if (!drawItem)
            continue;
        MHWRender::MRenderItem* renderItem = drawItem->GetRenderItem();
        if (!renderItem)
            continue;

        drawItem->GetRenderItemData()._enabled = false;

        _delegate->GetVP2ResourceRegistry().EnqueueCommit([renderItem]() {
            renderItem->enable(false);
        });
    }
}

/*! \brief  Update _primvarSourceMap, our local cache of raw primvar data.

    This function pulls data from the scene delegate, but defers processing.

    While iterating primvars, we skip "points" (vertex positions) because
    the points primvar is processed separately for direct access later. We
    only call GetPrimvar on primvars that have been marked dirty.
*/
void HdVP2BasisCurves::_UpdatePrimvarSources(
    HdSceneDelegate* sceneDelegate,
    HdDirtyBits dirtyBits,
    const TfTokenVector& requiredPrimvars)
{
    const SdfPath& id = GetId();

    TfTokenVector::const_iterator begin = requiredPrimvars.cbegin();
    TfTokenVector::const_iterator end = requiredPrimvars.cend();

    for (size_t i = 0; i < HdInterpolationCount; i++) {
        const HdInterpolation interp = static_cast<HdInterpolation>(i);

        const HdPrimvarDescriptorVector primvars =
            GetPrimvarDescriptors(sceneDelegate, interp);

        for (const HdPrimvarDescriptor& pv: primvars) {
            if (std::find(begin, end, pv.name) == end) {
                _curvesSharedData._primvarSourceMap.erase(pv.name);
            }
            else if (HdChangeTracker::IsPrimvarDirty(dirtyBits, id, pv.name)) {
                const VtValue value = GetPrimvar(sceneDelegate, pv.name);
                _curvesSharedData._primvarSourceMap[pv.name] = { value, interp };
            }
        }
    }
}

/*! \brief  Create render item for wireframe repr.
*/
MHWRender::MRenderItem*
HdVP2BasisCurves::_CreateWireRenderItem(const MString& name) const
{
    MHWRender::MRenderItem* const renderItem = MHWRender::MRenderItem::Create(
        name,
        MHWRender::MRenderItem::DecorationItem,
        MHWRender::MGeometry::kLines
    );

    renderItem->setDrawMode(MHWRender::MGeometry::kWireframe);
    renderItem->depthPriority(MHWRender::MRenderItem::sDormantWireDepthPriority);
    renderItem->castsShadows(false);
    renderItem->receivesShadows(false);
    renderItem->setShader(_delegate->Get3dSolidShader(kOpaqueGray));
    renderItem->setSelectionMask(MSelectionMask::kSelectCurves);

    setWantConsolidation(*renderItem, true);

    return renderItem;
}

/*! \brief  Create render item for bbox repr.
*/
MHWRender::MRenderItem*
HdVP2BasisCurves::_CreateBBoxRenderItem(const MString& name) const
{
    MHWRender::MRenderItem* const renderItem = MHWRender::MRenderItem::Create(
        name,
        MHWRender::MRenderItem::DecorationItem,
        MHWRender::MGeometry::kLines
    );

    renderItem->setDrawMode(MHWRender::MGeometry::kBoundingBox);
    renderItem->castsShadows(false);
    renderItem->receivesShadows(false);
    renderItem->setShader(_delegate->Get3dSolidShader(kOpaqueGray));
    renderItem->setSelectionMask(MSelectionMask::kSelectCurves);

    setWantConsolidation(*renderItem, true);

    return renderItem;
}

/*! \brief  Create render item for smoothHull repr.
*/
MHWRender::MRenderItem*
HdVP2BasisCurves::_CreatePatchRenderItem(const MString& name) const
{
    MHWRender::MRenderItem* const renderItem = MHWRender::MRenderItem::Create(
        name,
        MHWRender::MRenderItem::MaterialSceneItem,
        MHWRender::MGeometry::kLines
    );

    renderItem->setDrawMode(static_cast<MHWRender::MGeometry::DrawMode>(
        MHWRender::MGeometry::kShaded | MHWRender::MGeometry::kTextured));
    renderItem->castsShadows(false);
    renderItem->receivesShadows(false);
    renderItem->setShader(_delegate->Get3dSolidShader(kOpaqueGray));
    renderItem->setSelectionMask(MSelectionMask::kSelectCurves);

    setWantConsolidation(*renderItem, true);

    return renderItem;
}

/*! \brief  Create render item for points repr.
*/
MHWRender::MRenderItem*
HdVP2BasisCurves::_CreatePointsRenderItem(const MString& name) const
{
    MHWRender::MRenderItem* const renderItem = MHWRender::MRenderItem::Create(
        name,
        MHWRender::MRenderItem::DecorationItem,
        MHWRender::MGeometry::kPoints
    );

    renderItem->setDrawMode(MHWRender::MGeometry::kSelectionOnly);
    renderItem->castsShadows(false);
    renderItem->receivesShadows(false);
    renderItem->setShader(_delegate->Get3dFatPointShader());

    MSelectionMask selectionMask(MSelectionMask::kSelectPointsForGravity);
    selectionMask.addMask(MSelectionMask::kSelectCurves);
    renderItem->setSelectionMask(selectionMask);

    setWantConsolidation(*renderItem, true);

    return renderItem;
}

PXR_NAMESPACE_CLOSE_SCOPE
