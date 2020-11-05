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
#include "AL/usd/transaction/Transaction.h"

#include "AL/usd/transaction/TransactionManager.h"

#include <iostream>
PXR_NAMESPACE_USING_DIRECTIVE

namespace AL {
namespace usd {
namespace transaction {

Transaction::Transaction(const UsdStageWeakPtr& stage, const SdfLayerHandle& layer)
    : m_manager(TransactionManager::Get(stage))
    , m_layer(layer)
{
}

//----------------------------------------------------------------------------------------------------------------------
bool Transaction::Open() const { return m_manager.Open(m_layer); }

//----------------------------------------------------------------------------------------------------------------------
bool Transaction::Close() const { return m_manager.Close(m_layer); }

//----------------------------------------------------------------------------------------------------------------------
bool Transaction::InProgress() const { return m_manager.InProgress(m_layer); }

//----------------------------------------------------------------------------------------------------------------------
} // namespace transaction
} // namespace usd
} // namespace AL
//----------------------------------------------------------------------------------------------------------------------
