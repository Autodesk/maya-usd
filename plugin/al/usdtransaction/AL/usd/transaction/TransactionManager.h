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
#include "AL/usd/transaction/Notice.h"

#include <pxr/base/tf/weakPtr.h>
#include <pxr/pxr.h>

namespace AL {
namespace usd {
namespace transaction {

//----------------------------------------------------------------------------------------------------------------------
/// \brief  This is a transaction management class which provides interface for opening and closing
/// multiple
///         transactions targeting any stage and layer.
///         It provides both direct interface used by Transaction class, which avoids stage lookup,
///         as well as static interface where stage needs to be provided.
///
///         Whenever a new transaction (first one targeting given layer) is opened an OpenNotice is
///         being emitted and snapshot of given layer is taken. Whenever last transaction targeting
///         given layer for given stage is closed, targetted layer content is being compared against
///         previously taken snapshot and CloseNotice is emitted with delta information.
///
/// \note   It's user responsibilty to pair Open with Close calls, otherwise clients might not
/// respond to any
///         further changes. As such it's advisable to prefer ScopedTransaction whenever possible.
//----------------------------------------------------------------------------------------------------------------------
class TransactionManager
{
public:
    /// \brief  provides information whether transaction was opened and wasn't closed yet.
    /// \param  layer targetted by transaction
    /// \return true when transaction is in progress, otherwise false
    AL_USD_TRANSACTION_PUBLIC
    bool InProgress(const PXR_NS::SdfLayerHandle& layer) const;

    /// \brief  opens transaction, when transaction is opened for the first time OpenNotice is
    /// emitted and current
    ///         state of layer is recorded.
    /// \note   It's valid to call Open multiple times, but they need to balance Close calls
    /// \param  layer targetted by transaction
    /// \return true on success, false when layer or stage became invalid
    AL_USD_TRANSACTION_PUBLIC
    bool Open(const PXR_NS::SdfLayerHandle& layer);

    /// \brief  closes transaction, when transaction is closed for the last time CloseNotice is
    /// emitted with change
    ///         information based of difference between current and recorded layer states.
    /// \note   It's valid to call Close multiple times, but they need to balance Open calls
    /// \param  layer targetted by transaction
    /// \return true on success, false when layer or stage became invalid or transaction wasn't
    /// opened
    AL_USD_TRANSACTION_PUBLIC
    bool Close(const PXR_NS::SdfLayerHandle& layer);

    /// \brief  provides information whether any transaction was opened and wasn't closed yet.
    /// \return true when any transaction is in progress, otherwise false
    AL_USD_TRANSACTION_PUBLIC
    bool AnyInProgress() const;

    /// static inteface ///

    /// \brief  provides reference for TransactionManager dealing with given stage.
    /// \param  stage that is managed by TransactionManager
    /// \return reference to shared TransactionManager for given stage
    AL_USD_TRANSACTION_PUBLIC
    static TransactionManager& Get(const PXR_NS::UsdStageWeakPtr& stage);

    /// \brief  provides information whether any transaction was opened and wasn't closed yet.
    /// \param  stage that is managed by TransactionManager
    /// \return true when any transaction is in progress, otherwise false
    AL_USD_TRANSACTION_PUBLIC
    static bool InProgress(const PXR_NS::UsdStageWeakPtr& stage);

    /// \brief  provides information whether transaction was opened and wasn't closed yet.
    /// \param  stage that is managed by TransactionManager
    /// \param  layer targetted by transaction
    /// \return true when transaction is in progress, otherwise false
    AL_USD_TRANSACTION_PUBLIC
    static bool
    InProgress(const PXR_NS::UsdStageWeakPtr& stage, const PXR_NS::SdfLayerHandle& layer);

    /// \brief  opens transaction, when transaction is opened for the first time OpenNotice is
    /// emitted and current
    ///         state of layer is recorded.
    /// \note   It's valid to call Open multiple times, but they need to balance Close calls
    /// \param  stage that will be notified about transaction open/close
    /// \param  layer targetted by transaction
    /// \return true on success, false when layer or stage became invalid
    AL_USD_TRANSACTION_PUBLIC
    static bool Open(const PXR_NS::UsdStageWeakPtr& stage, const PXR_NS::SdfLayerHandle& layer);

    /// \brief  closes transaction, when transaction is closed for the last time CloseNotice is
    /// emitted with change
    ///         information based of difference between current and recorded layer states.
    /// \note   It's valid to call Close multiple times, but they need to balance Open calls
    /// \param  stage that will be notified about transaction open/close
    /// \param  layer targetted by transaction
    /// \return true on success, false when layer or stage became invalid or transaction wasn't
    /// opened
    AL_USD_TRANSACTION_PUBLIC
    static bool Close(const PXR_NS::UsdStageWeakPtr& stage, const PXR_NS::SdfLayerHandle& layer);

    /// \brief  clears the transaction manager of all active transactions, effectively closing them
    /// all. Intended to be used for File->New and on exit.
    AL_USD_TRANSACTION_PUBLIC
    static void CloseAll();

private:
    typedef std::map<PXR_NS::UsdStageWeakPtr, TransactionManager> StageManagerMap;
    static StageManagerMap&                                       GetManagers();

private:
    TransactionManager(const PXR_NS::UsdStageWeakPtr& stage)
        : m_stage(stage)
    {
    }
    struct TransactionData
    {
        PXR_NS::SdfLayerRefPtr base;
        int                    count;
    };
    const PXR_NS::UsdStageWeakPtr                          m_stage;
    std::unordered_map<PXR_NS::SdfLayer*, TransactionData> m_transactions;
};

//----------------------------------------------------------------------------------------------------------------------
} // namespace transaction
} // namespace usd
} // namespace AL
//----------------------------------------------------------------------------------------------------------------------
