//
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
#ifndef HD_VP2_DRAW_ITEM
#define HD_VP2_DRAW_ITEM

#include <pxr/base/gf/vec3f.h>
#include <pxr/base/vt/array.h>
#include <pxr/imaging/hd/drawItem.h>
#include <pxr/imaging/hd/geomSubset.h>
#include <pxr/imaging/hd/mesh.h>
#include <pxr/pxr.h>
#include <pxr/usd/usd/timeCode.h>

#include <maya/MBoundingBox.h>
#include <maya/MHWGeometry.h>
#include <maya/MMatrix.h>
#include <maya/MString.h>

PXR_NAMESPACE_OPEN_SCOPE

class HdVP2RenderDelegate;

/*! \brief  Draw Item holds information necessary for accessing and updating VP2 render items
    \class  HdVP2DrawItem
*/
class HdVP2DrawItem final : public HdDrawItem
{
public:
    //! Helper struct providing storage for render item data
    struct RenderItemData
    {
        //! Unique name for easier debugging and profiling.
        MString _renderItemName;
        //! Pointer of the render item for fast access. No ownership is held.
        MHWRender::MRenderItem* _renderItem { nullptr };

        //! The geom subset this render item represents. _geomSubset.id is StdPath::EmptyPath() if
        //! there is no geom subset.
        HdGeomSubset _geomSubset;

        //! Render item index buffer - use when updating data
        std::unique_ptr<MHWRender::MIndexBuffer> _indexBuffer;
        //! Bounding box of the render item.
        MBoundingBox _boundingBox;
        //! World matrix of the render item.
        MMatrix _worldMatrix;

        //! Shader instance assigned to the render item. No ownership is held.
        MHWRender::MShaderInstance* _shader { nullptr };

        //! Is _shader a fallback material?
        bool _shaderIsFallback { true };

        //! Whether or not the render item is enabled
        bool _enabled { true };

        //! Whether or not the render item is transparent
        bool _transparent { false };

        //! Primitive type of the render item
        MHWRender::MGeometry::Primitive _primitiveType {
            MHWRender::MGeometry::kInvalidPrimitive
        }; // TODO: this is stored on the MRenderItem, do we need it here too?
        //! Primitive stride of the render item (valid only if the primitive type is kPatch)
        int _primitiveStride { 0 };

        //! Number of instances currently allocated for render item
        unsigned int _instanceCount { 0 };

        //! Whether or not the render item is using GPU instanced draw.
        bool _usingInstancedDraw { false };

        //! Dirty bits to control data update of draw item
        HdDirtyBits _dirtyBits { HdChangeTracker::AllDirty };

        /*! \brief  Bitwise OR with the input dirty bits.
         */
        void SetDirtyBits(HdDirtyBits bits) { _dirtyBits |= bits; }

        /*! \brief  Reset the dirty bits to clean.
         */
        void ResetDirtyBits() { _dirtyBits = 0; }

        /*! \brief  Get the dirty bits of the draw items.
         */
        HdDirtyBits GetDirtyBits() const { return _dirtyBits; }
    };

    //! Bit fields indicating what the render item is created for. A render item
    //! can be created for multiple usages.
    enum RenderItemUsage
    {
        kRegular = 1 << 0,           //!< Regular drawing (shaded, wireframe etc.)
        kSelectionHighlight = 1 << 1 //!< Selection highlight.
    };

public:
    HdVP2DrawItem(HdVP2RenderDelegate* delegate, const HdRprimSharedData* sharedData);

    ~HdVP2DrawItem();

    void AddRenderItem(MHWRender::MRenderItem* item, const HdGeomSubset* geomSubset = nullptr);

    using RenderItemDataVector = std::vector<RenderItemData>;
    const RenderItemDataVector& GetRenderItems() const { return _renderItems; }
    RenderItemDataVector&       GetRenderItems() { return _renderItems; }

    /*! \brief  Get access to render item data.
     */
    RenderItemData& GetRenderItemData()
    {
        TF_VERIFY(_renderItems.size() == 1);
        return _renderItems[0];
    }

    /*! \brief Get acces to render item data.
     */
    const RenderItemData& GetRenderItemData() const
    {
        TF_VERIFY(_renderItems.size() == 1);
        return _renderItems[0];
    }

    /*! \brief  Get draw item name
     */
    const MString& GetDrawItemName() const { return _drawItemName; }

    /*! \brief  Get render item name
     */
    const MString& GetRenderItemName() const { return GetRenderItemData()._renderItemName; }

    /*! \brief  Get pointer of the associated render item
     */
    MHWRender::MRenderItem* GetRenderItem() const { return GetRenderItemData()._renderItem; }

    /*! \brief  Set pointer of the associated render item
     */
    void SetRenderItem(MHWRender::MRenderItem* item)
    {
        TF_VERIFY(_renderItems.size() == 0);
        AddRenderItem(item);
    }

    /*! \brief  Set a usage to the render item
     */
    void SetUsage(RenderItemUsage usage) { _renderItemUsage = usage; }

    /*! \brief  Add a usage to the render item
     */
    void AddUsage(RenderItemUsage usage) { _renderItemUsage |= usage; }

    /*! \brief  Is the render item created for this usage?
     */
    bool ContainsUsage(RenderItemUsage usage) const { return (_renderItemUsage & usage) != 0; }

    /*! \brief  Is the render item created for this usage only?
     */
    bool MatchesUsage(RenderItemUsage usage) const { return _renderItemUsage == usage; }

    /*! \brief  Bitwise OR with the input dirty bits.
     */
    void SetDirtyBits(HdDirtyBits bits) { GetRenderItemData().SetDirtyBits(bits); }

    /*! \brief  Reset the dirty bits to clean.
     */
    void ResetDirtyBits() { GetRenderItemData().ResetDirtyBits(); }

    /*! \brief  Get the dirty bits of the draw items.
     */
    HdDirtyBits GetDirtyBits() const { return GetRenderItemData().GetDirtyBits(); }

private:
    /*
        Data stored directly on the HdVP2DrawItem can be shared across all the MRenderItems
        *this represents.
    */

    //! VP2 render delegate for which this draw item was created
    HdVP2RenderDelegate* _delegate { nullptr };
    //! Unique name for easier debugging and profiling.
    MString _drawItemName;
    //!< What is the render item created for
    uint32_t _renderItemUsage { kRegular };

    /*
        The list of MRenderItems used to represent *this in VP2.
    */
    RenderItemDataVector _renderItems; //!< VP2 render item data
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
