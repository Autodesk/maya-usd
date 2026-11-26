//
// Copyright 2026 Autodesk
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
#include "gaussianSplatViewCache.h"

#include "gaussianSplatCallback.h"
#include "resourceRegistry.h"

#include <pxr/base/tf/diagnostic.h>

#include <maya/MDagPath.h>
#include <maya/MDrawContext.h>
#include <maya/MFrameContext.h>
#include <maya/MMatrix.h>
#include <maya/MStringArray.h>

#include <algorithm>
#include <map>

PXR_NAMESPACE_USING_DIRECTIVE

namespace MAYAUSD_NS_DEF {

namespace {

//! Parameters of the splat shader that tint a selected or highlighted splat.
constexpr const char* kHighlightEnabledParam = "highlightEnabled";
constexpr const char* kHighlightColorParam = "highlightColor";

//! Shader clone to cache. Only accessed on the main thread, so it needs no lock. The caches are
//! raw pointers: OnRprimDestroyed erases its entry while the Rprim still holds the cache.
std::map<const MHWRender::MShaderInstance*, GaussianSplatViewCache*>& _Registry()
{
    static std::map<const MHWRender::MShaderInstance*, GaussianSplatViewCache*> sRegistry;
    return sRegistry;
}

//! True for the beauty pass of a view: a color pass. Shadow, depth and normal-depth passes carry
//! other semantics (measured, see pass-context-notes.md).
bool _IsBeautyPass(const MHWRender::MDrawContext& context)
{
    const MStringArray& semantics = context.getPassContext().passSemantics();
    for (unsigned int i = 0; i < semantics.length(); ++i) {
        if (semantics[i] == MHWRender::MPassContext::kColorPassSemantic) {
            return true;
        }
    }
    return false;
}

//! Rewrites a vertex or index buffer at its existing size. Returns false when the buffer does
//! not have that size (nothing is touched then).
bool _RewriteIndexBuffer(MHWRender::MIndexBuffer* buffer, const uint32_t* order, size_t count)
{
    if (!buffer || buffer->size() != count) {
        return false;
    }
    void* const data = buffer->acquire(static_cast<unsigned int>(count), true);
    if (!data) {
        return false;
    }
    std::copy_n(order, count, static_cast<uint32_t*>(data));
    buffer->commit(data);
    return true;
}

bool _RewriteColorBuffer(MHWRender::MVertexBuffer* buffer, const float* colors, size_t count)
{
    if (!buffer || buffer->vertexCount() != count) {
        return false;
    }
    void* const data = buffer->acquire(static_cast<unsigned int>(count), true);
    if (!data) {
        return false;
    }
    std::copy_n(colors, count * 4, static_cast<float*>(data));
    buffer->commit(data);
    return true;
}

} // anonymous namespace

GaussianSplatViewCache* GaussianSplatViewCache::Find(const MHWRender::MShaderInstance* shader)
{
    const auto it = _Registry().find(shader);
    return it == _Registry().end() ? nullptr : it->second;
}

MHWRender::MShaderInstance* GaussianSplatViewCache::EnsureShader(const std::string& colorSpace)
{
    if (!_shader && !_creationFailed) {
        _shader = CreateSplatShaderClone(colorSpace);
        _creationFailed = !_shader;
        if (_shader) {
            _Registry()[_shader] = this;
        }
    }
    return _shader;
}

void GaussianSplatViewCache::OnDataSynced(
    std::shared_ptr<const GaussianSplatViewCore::SplatData> data)
{
    if (!data || data == _data) {
        return;
    }
    // The version bump makes every slot stale. Their order and colors are only read again after a
    // recomputation, which sizes them for the new count.
    _data = std::move(data);
    ++_dataVersion;
    _ForgetBufferedSlot();
}

void GaussianSplatViewCache::OnBuffersCommitted(
    MHWRender::MVertexBuffer* colorBuffer,
    MHWRender::MIndexBuffer*  indexBuffer,
    bool                      indexResized)
{
    // The color buffer is created by the first Sync with points, so it is refreshed every time.
    _colorBuffer = colorBuffer;
    if (indexBuffer
        && std::find(_indexBuffers.begin(), _indexBuffers.end(), indexBuffer)
            == _indexBuffers.end()) {
        _indexBuffers.push_back(indexBuffer);
    }
    if (indexResized) {
        _ForgetBufferedSlot();
    }
}

bool GaussianSplatViewCache::SetTint(bool enabled, const float color[4])
{
    if (!_shader) {
        return false;
    }
    const float enabledValue = enabled ? 1.0f : 0.0f;
    if (_shader->setParameter(kHighlightEnabledParam, enabledValue) != MStatus::kSuccess
        || _shader->setParameter(kHighlightColorParam, color) != MStatus::kSuccess) {
        TF_WARN("The Gaussian splat tint could not be set.");
        return false;
    }
    return true;
}

GaussianSplatViewCache::View
GaussianSplatViewCache::_ReadView(const MHWRender::MDrawContext& context)
{
    View view;

    MStatus        status;
    const MDagPath cameraPath = context.getCurrentCameraPath(&status);
    if (!status || !cameraPath.isValid()) {
        return view;
    }

    const MMatrix viewMatrix = context.getMatrix(MHWRender::MFrameContext::kViewMtx, &status);
    if (!status) {
        return view;
    }

    int originX = 0, originY = 0, width = 0, height = 0;
    if (!context.getViewportDimensions(originX, originY, width, height)) {
        return view;
    }

    view.cameraPath = cameraPath.fullPathName().asChar();
    view.viewportSize = GfVec2i(width, height);
    for (int row = 0; row < 4; ++row) {
        for (int col = 0; col < 4; ++col) {
            view.viewMatrix[row][col] = viewMatrix(row, col);
        }
    }
    return view;
}

const GaussianSplatViewCache::Slot* GaussianSplatViewCache::_SlotForView(const View& view)
{
    ++_sequence;
    if (!_data) {
        return nullptr;
    }

    if (view.cameraPath.empty()) {
        // Camera unknown: reuse the most recently used slot that is valid for this data.
        const Slot* best = nullptr;
        for (const Slot& s : _slots) {
            if (s.dataVersion == _dataVersion && (!best || s.lastUse > best->lastUse)) {
                best = &s;
            }
        }
        return best;
    }

    Slot* slot = nullptr;
    for (Slot& s : _slots) {
        if (s.cameraPath == view.cameraPath && s.viewportSize == view.viewportSize) {
            slot = &s;
            break;
        }
    }
    if (!slot) {
        if (_slots.size() < kMaxSlots) {
            _slots.emplace_back();
            slot = &_slots.back();
        } else {
            slot = &*std::min_element(
                _slots.begin(), _slots.end(), [](const Slot& a, const Slot& b) {
                    return a.lastUse < b.lastUse;
                });
            *slot = Slot();
        }
        slot->cameraPath = view.cameraPath;
        slot->viewportSize = view.viewportSize;
        // A new uid: the buffers can never be taken as holding a recycled slot.
        slot->uid = _nextUid++;
    }

    if (slot->dataVersion != _dataVersion || !(slot->viewMatrix == view.viewMatrix)) {
        // Order and color are computed for the same camera, then stored as one unit.
        GaussianSplatViewCore::SortBackToFront(*_data, view.viewMatrix, slot->order);
        GaussianSplatViewCore::ComputeColors(*_data, view.viewMatrix, slot->colors);
        slot->viewMatrix = view.viewMatrix;
        slot->dataVersion = _dataVersion;
        ++slot->revision;
    }
    slot->lastUse = _sequence;
    return slot;
}

void GaussianSplatViewCache::OnDraw(const MHWRender::MDrawContext& context)
{
    if (!_IsBeautyPass(context)) {
        return;
    }

    const Slot* const slot = _SlotForView(_ReadView(context));
    if (!slot) {
        return;
    }
    const bool held = _bufferUid == slot->uid && _bufferRevision == slot->revision;
    _bufferUid = slot->uid;
    _bufferRevision = slot->revision;
    if (held || slot->order.empty()) {
        return;
    }
    const size_t count = slot->order.size();

    // The buffers must have the size of the data (the commit lambda sizes them, the draw never
    // does). An index buffer of another size belongs to an item that was not committed yet for the
    // current count (new item, or a repr not synced after a count edit): it is skipped, not
    // counted as a failure. The commit that sizes it writes the identity order and reports it
    // through OnBuffersCommitted, so the next draw rewrites it with the sorted order. Only a buffer
    // that should have been written and was not (color mismatch, acquire failure) forces a retry.
    bool ok = _RewriteColorBuffer(_colorBuffer, slot->colors.data(), count);
    for (MHWRender::MIndexBuffer* const indexBuffer : _indexBuffers) {
        if (indexBuffer && indexBuffer->size() != count) {
            continue;
        }
        ok = _RewriteIndexBuffer(indexBuffer, slot->order.data(), count) && ok;
    }
    if (!ok) {
        _ForgetBufferedSlot();
    }
}

void GaussianSplatViewCache::OnRprimDestroyed(PXR_NS::HdVP2ResourceRegistry& registry)
{
    _destroyed = true;

    MHWRender::MShaderInstance* const shader = _shader;
    _shader = nullptr;
    if (shader) {
        _Registry().erase(shader);
        registry.EnqueueCommit([shader]() { ReleaseSplatShader(shader); });
    }
}

} // namespace MAYAUSD_NS_DEF
