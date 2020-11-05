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

#include <pxr/pxr.h>
#include <pxr/usd/usd/stage.h>

namespace AL {
namespace usd {
namespace transaction {

class TransactionManager;

//----------------------------------------------------------------------------------------------------------------------
/// \brief  This is a transaction class which provides interface for opening and closing
/// transactions.
///         Management of transaction logic is performed by Manager class.
///         It's user responsibilty to pair Open with Close calls, otherwise clients might not
///         respond to any further changes. As such ScopedTransaction should be preferred whenever
///         possible.
//----------------------------------------------------------------------------------------------------------------------
class Transaction
{
public:
    /// \brief  the ctor retrieves manager for given stage and sets layer for tracking
    /// \param  stage that will be notified about transaction open/close
    /// \param  layer that will be tracked for changes
    AL_USD_TRANSACTION_PUBLIC
    Transaction(const PXR_NS::UsdStageWeakPtr& stage, const PXR_NS::SdfLayerHandle& layer);

    /// \brief  opens transaction, when transaction is opened for the first time OpenNotice is
    /// emitted and current
    ///         state of layer is recorded.
    /// \note   It's valid to call Open multiple times, but they need to balance Close calls
    /// \return true on success, false when layer or stage became invalid
    AL_USD_TRANSACTION_PUBLIC
    bool Open() const;

    /// \brief  closes transaction, when transaction is closed for the last time CloseNotice is
    /// emitted with change
    ///         information based of difference between current and recorded layer states.
    /// \note   It's valid to call Close multiple times, but they need to balance Open calls
    /// \return true on success, false when layer or stage became invalid or transaction wasn't
    /// opened
    AL_USD_TRANSACTION_PUBLIC
    bool Close() const;

    /// \brief  provides information whether transaction was opened and wasn't closed yet.
    /// \return true when transaction is in progress, otherwise false
    AL_USD_TRANSACTION_PUBLIC
    bool InProgress() const;

private:
    TransactionManager&    m_manager;
    PXR_NS::SdfLayerHandle m_layer;
};

//----------------------------------------------------------------------------------------------------------------------
/// \brief  This is a helper class that binds transaction session to lifetime of class instance.
///         Transaction is opened when ScopeTransaction is constructed and closed when destructed.
//----------------------------------------------------------------------------------------------------------------------
class ScopedTransaction
{
public:
    /// \brief  the ctor initializes transaction and opens it
    /// \param  stage that will be notified about transaction open/close
    /// \param  layer that will be tracked for changes
    inline ScopedTransaction(
        const PXR_NS::UsdStageWeakPtr& stage,
        const PXR_NS::SdfLayerHandle&  layer)
        : m_transaction(stage, layer)
    {
        m_transaction.Open();
    }

    /// \brief  the dtor closes transaction
    inline ~ScopedTransaction() { m_transaction.Close(); }

private:
    ScopedTransaction() = delete;
    ScopedTransaction(const ScopedTransaction&) = delete;
    ScopedTransaction& operator=(const ScopedTransaction&) = delete;
    Transaction        m_transaction;
};

//----------------------------------------------------------------------------------------------------------------------
} // namespace transaction
} // namespace usd
} // namespace AL
//----------------------------------------------------------------------------------------------------------------------
