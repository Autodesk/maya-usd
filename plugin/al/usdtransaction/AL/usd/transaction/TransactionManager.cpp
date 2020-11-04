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
#include "AL/usd/transaction/TransactionManager.h"

PXR_NAMESPACE_USING_DIRECTIVE

namespace AL {
namespace usd {
namespace transaction {

namespace {
template <class T, class CompareEqualFunc>
void compareSpecViews(
    const T&         a,
    const T&         b,
    SdfPathVector&   output,
    SdfPathVector&   extra,
    CompareEqualFunc compareEqual)
{
    const auto aEnd = a.end();
    const auto bEnd = b.end();
    auto       aIt = a.begin();
    auto       bIt = b.begin();
    while (aIt != aEnd && bIt != bEnd) {
        const auto aName = (*aIt)->GetNameToken();
        const auto bName = (*bIt)->GetNameToken();
        if (aName == bName) {
            compareEqual(*aIt, *bIt, output, extra);
            ++aIt;
            ++bIt;
        } else if (aName > bName) {
            output.push_back((*aIt)->GetPath());
            ++aIt;
        } else {
            output.push_back((*bIt)->GetPath());
            ++bIt;
        }
    }
    /// Append remaining specs
    size_t remaining = std::distance(aIt, aEnd) + std::distance(bIt, bEnd);
    if (remaining) {
        auto   transformToPath = [](const typename T::value_type& spec) { return spec->GetPath(); };
        size_t offset = output.size();
        output.resize(offset + remaining);
        std::transform(aIt, aEnd, output.begin() + offset, transformToPath);
        offset += std::distance(aIt, aEnd);
        std::transform(bIt, bEnd, output.begin() + offset, transformToPath);
    }
}

void comparePrims(
    const SdfPrimSpecHandle& a,
    const SdfPrimSpecHandle& b,
    SdfPathVector&           resynced,
    SdfPathVector&           changed)
{
    assert(a && b);
    /// Compare children
    compareSpecViews(a->GetNameChildren(), b->GetNameChildren(), resynced, changed, comparePrims);
    // Compare properties
    auto compareProps = [](const SdfPropertySpecHandle& a,
                           const SdfPropertySpecHandle& b,
                           SdfPathVector&               output,
                           SdfPathVector&               extra) {
        const auto aFields = a->ListFields();
        const auto bFields = b->ListFields();

        if (aFields != bFields)
            output.push_back(a->GetPath());
        else {
            /// Check field values
            for (const auto& name : aFields) {
                if (a->GetField(name) != b->GetField(name)) {
                    output.push_back(a->GetPath());
                    return;
                }
            }
        }
    };
    compareSpecViews(a->GetProperties(), b->GetProperties(), changed, resynced, compareProps);
}
} // anonymous namespace

//----------------------------------------------------------------------------------------------------------------------
TransactionManager::StageManagerMap& TransactionManager::GetManagers()
{
    static StageManagerMap managers;
    return managers;
}

//----------------------------------------------------------------------------------------------------------------------
bool TransactionManager::InProgress(const SdfLayerHandle& layer) const
{
    return m_transactions.find(get_pointer(layer)) != m_transactions.end();
}

//----------------------------------------------------------------------------------------------------------------------
bool TransactionManager::AnyInProgress() const { return !m_transactions.empty(); }

//----------------------------------------------------------------------------------------------------------------------
bool TransactionManager::Open(const SdfLayerHandle& layer)
{
    if (m_stage && layer) {
        auto pair = m_transactions.emplace(get_pointer(layer), TransactionData { nullptr, 1 });
        if (pair.second) {
            auto& base = pair.first->second.base;
            base = SdfLayer::CreateAnonymous("transaction_base");
            base->TransferContent(layer);
            OpenNotice(layer).Send(m_stage);
        } else {
            ++pair.first->second.count;
        }
        return true;
    }
    TF_WARN("Provided layer is invalid");
    return false;
}

//----------------------------------------------------------------------------------------------------------------------
bool TransactionManager::Close(const SdfLayerHandle& layer)
{
    if (m_stage && layer) {
        auto it = m_transactions.find(get_pointer(layer));
        if (it != m_transactions.end()) {
            if (--it->second.count == 0) {
                SdfPathVector changedInfo, resynched;
                comparePrims(
                    it->second.base->GetPseudoRoot(),
                    layer->GetPseudoRoot(),
                    resynched,
                    changedInfo);
                CloseNotice(layer, std::move(changedInfo), std::move(resynched)).Send(m_stage);
                m_transactions.erase(it);
            }
            return true;
        }
        TF_WARN("Tried to close unopened transaction");
        return false;
    }
    TF_WARN("Provided layer is invalid");
    return false;
}

// static inteface //

//----------------------------------------------------------------------------------------------------------------------
TransactionManager& TransactionManager::Get(const UsdStageWeakPtr& stage)
{
    auto& managers = GetManagers();
    auto  pair = managers.emplace(stage, TransactionManager(stage));
    return pair.first->second;
}

//----------------------------------------------------------------------------------------------------------------------
bool TransactionManager::InProgress(const UsdStageWeakPtr& stage)
{
    const auto& managers = GetManagers();
    auto        it = managers.find(stage);
    return it != managers.end() ? it->second.AnyInProgress() : false;
}

//----------------------------------------------------------------------------------------------------------------------
bool TransactionManager::InProgress(const UsdStageWeakPtr& stage, const SdfLayerHandle& layer)
{
    const auto& managers = GetManagers();
    auto        it = managers.find(stage);
    return it != managers.end() ? it->second.InProgress(layer) : false;
}

//----------------------------------------------------------------------------------------------------------------------
bool TransactionManager::Open(const UsdStageWeakPtr& stage, const SdfLayerHandle& layer)
{
    auto& managers = GetManagers();
    auto  pair = managers.emplace(stage, TransactionManager(stage));
    return pair.first->second.Open(layer);
}

//----------------------------------------------------------------------------------------------------------------------
bool TransactionManager::Close(const UsdStageWeakPtr& stage, const SdfLayerHandle& layer)
{
    auto& managers = GetManagers();
    auto  it = managers.find(stage);
    if (it != managers.end()) {
        return it->second.Close(layer);
    }
    return false;
}

//----------------------------------------------------------------------------------------------------------------------
} // namespace transaction
} // namespace usd
} // namespace AL
//----------------------------------------------------------------------------------------------------------------------
