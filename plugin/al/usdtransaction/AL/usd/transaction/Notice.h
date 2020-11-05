//
// Copyright 2017 Animal Logic
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
#include "AL/usd/transaction/Api.h"

#include <pxr/base/tf/notice.h>
#include <pxr/pxr.h>
#include <pxr/usd/usd/stage.h>

namespace AL {
namespace usd {
namespace transaction {

//----------------------------------------------------------------------------------------------------------------------
/// \brief  This transaction notice is sent when transaction is opened.
//----------------------------------------------------------------------------------------------------------------------
class OpenNotice : public PXR_NS::TfNotice
{
public:
    /// \brief  the ctor sets tracked by transaction
    /// \param  layer that transaction is tracking
    AL_USD_TRANSACTION_PUBLIC
    OpenNotice(const PXR_NS::SdfLayerHandle& layer)
        : m_layer(layer)
    {
    }

    /// \brief  gets layer tracked by transaction
    /// \return layer that transaction is tracking
    inline const PXR_NS::SdfLayerHandle& GetLayer() const { return m_layer; }

private:
    PXR_NS::SdfLayerHandle m_layer;
};

//----------------------------------------------------------------------------------------------------------------------
/// \brief  This transaction notice is sent when transaction is closed.
//----------------------------------------------------------------------------------------------------------------------
class CloseNotice : public PXR_NS::TfNotice
{
public:
    /// \brief  the ctor sets tracked by transaction and changed as well as resynced paths
    /// \param  layer that transaction is tracking
    /// \param  changed vector of paths that changed properties
    /// \param  resynced vector of topmost paths for which hierarchy has changed
    inline CloseNotice(
        const PXR_NS::SdfLayerHandle& layer,
        PXR_NS::SdfPathVector         changed,
        PXR_NS::SdfPathVector         resynced)
        : m_layer(layer)
        , m_changed(std::move(changed))
        , m_resynced(std::move(resynced))
    {
    }

    /// \brief  gets layer tracked by transaction
    /// \return layer that transaction is tracking
    inline const PXR_NS::SdfLayerHandle& GetLayer() const { return m_layer; }

    /// \brief  gets vector of paths that changed properties
    /// \return const reference to vector of paths
    inline const PXR_NS::SdfPathVector& GetChangedInfoOnlyPaths() const { return m_changed; }

    /// \brief  gets vector of topmost paths for which hierarchy has changed
    /// \return const reference to vector of paths
    inline const PXR_NS::SdfPathVector& GetResyncedPaths() const { return m_resynced; }

    /// \brief  provides information if any changes were registered
    /// \return true when any changes were registered, otherwise false
    inline bool AnyChanges() const { return !m_changed.empty() || !m_resynced.empty(); }

private:
    PXR_NS::SdfLayerHandle m_layer;
    PXR_NS::SdfPathVector  m_changed, m_resynced;
};

//----------------------------------------------------------------------------------------------------------------------
} // namespace transaction
} // namespace usd
} // namespace AL
//----------------------------------------------------------------------------------------------------------------------
