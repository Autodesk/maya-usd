#include "AL/usd/transaction/Notice.h"
#include "AL/usd/transaction/Transaction.h"

#include <pxr/pxr.h>
#include <pxr/usd/usd/attribute.h>
#include <pxr/usd/usd/stage.h>

#include <gtest/gtest.h>

using namespace AL::usd::transaction;
PXR_NAMESPACE_USING_DIRECTIVE

//----------------------------------------------------------------------------------------------------------------------
/// \brief  Test some of the functionality of the AL_USDTransaction
//----------------------------------------------------------------------------------------------------------------------

/// shorthand function for empty paths vector
SdfPathVector empty() { return SdfPathVector(); }

/// helper function to return sorted paths vector from list of strings
SdfPathVector sorted(const std::initializer_list<const char*>& paths)
{
    SdfPathVector result(paths.size());
    std::transform(
        paths.begin(), paths.end(), result.begin(), [](const char* path) { return SdfPath(path); });
    std::sort(result.begin(), result.end());
    return result;
}

/// helper function to return sorted paths vector
SdfPathVector sorted(const SdfPathVector& paths)
{
    SdfPathVector result = paths;
    std::sort(result.begin(), result.end());
    return result;
}

// The fixture for testing transactions.
class TransactionTest
    : public TfWeakBase
    , public ::testing::Test
{
public:
    TransactionTest() { }
    ~TransactionTest() override { }

    size_t               opened() const { return m_opened; }
    size_t               closed() const { return m_closed; }
    const SdfPathVector& getChanged() const { return m_changed; }
    const SdfPathVector& getResynced() const { return m_resynced; }

    /// helper method to create prim at given path with attribute
    void createPrimWithAttribute(const char* path, const char* attributeName = "prop")
    {
        auto prim = m_stage->DefinePrim(SdfPath(path));
        EXPECT_TRUE(prim);
        if (attributeName) {
            auto attr = prim.CreateAttribute(TfToken(attributeName), SdfValueTypeNames->Int);
            EXPECT_TRUE(attr.Set(1));
        }
    }

    /// helper method to change prim attribute at given path to given value
    void changePrimAttribute(const char* path, int value, const char* attributeName = "prop")
    {
        auto prim = m_stage->GetPrimAtPath(SdfPath(path));
        EXPECT_TRUE(prim);
        auto attr = prim.GetAttribute(TfToken(attributeName));
        EXPECT_TRUE(attr.Set(value));
    };

protected:
    void openNotification(const OpenNotice& notice, const UsdStageWeakPtr& stage)
    {
        EXPECT_EQ(stage, m_stage);
        ++m_opened;
    }

    void closeNotification(const CloseNotice& notice, const UsdStageWeakPtr& stage)
    {
        EXPECT_EQ(stage, m_stage);
        ++m_closed;
        m_changed = notice.GetChangedInfoOnlyPaths();
        m_resynced = notice.GetResyncedPaths();
    }

    void SetUp() override
    {
        m_stage = UsdStage::CreateInMemory();
        m_stage->SetEditTarget(m_stage->GetSessionLayer());
        TfWeakPtr<TransactionTest> self(this);
        m_openNoticeKey = TfNotice::Register(self, &TransactionTest::openNotification, m_stage);
        ASSERT_TRUE(m_openNoticeKey);
        m_closeNoticeKey = TfNotice::Register(self, &TransactionTest::closeNotification, m_stage);
        ASSERT_TRUE(m_closeNoticeKey);
    }

    void TearDown() override
    {
        TfNotice::Revoke(m_openNoticeKey);
        TfNotice::Revoke(m_closeNoticeKey);
    }

    UsdStageRefPtr m_stage;

private:
    TfNotice::Key m_openNoticeKey;
    TfNotice::Key m_closeNoticeKey;
    size_t        m_opened = 0;
    size_t        m_closed = 0;
    SdfPathVector m_changed;
    SdfPathVector m_resynced;
};

/// Test that Transaction Open / Close methods work as expected
TEST_F(TransactionTest, Transaction)
{
    Transaction transaction(m_stage, m_stage->GetSessionLayer());
    EXPECT_EQ(opened(), 0u);
    EXPECT_EQ(closed(), 0u);
    // Open notice should be triggered
    EXPECT_TRUE(transaction.Open());
    EXPECT_EQ(opened(), 1u);
    EXPECT_EQ(closed(), 0u);
    // Opening same transaction is allowed, but should not trigger notices
    EXPECT_TRUE(transaction.Open());
    EXPECT_EQ(opened(), 1u);
    EXPECT_EQ(closed(), 0u);
    // Close notices should not be emitted until last close
    EXPECT_TRUE(transaction.Close());
    EXPECT_EQ(opened(), 1u);
    EXPECT_EQ(closed(), 0u);
    // Close notice should be triggered
    EXPECT_TRUE(transaction.Close());
    EXPECT_EQ(opened(), 1u);
    EXPECT_EQ(closed(), 1u);
    // This should fail and no notices should be sent
    EXPECT_FALSE(transaction.Close());
    EXPECT_EQ(opened(), 1u);
    EXPECT_EQ(closed(), 1u);
}

/// Test that ScopedTransaction works as expected
TEST_F(TransactionTest, ScopedTransaction)
{
    EXPECT_EQ(opened(), 0u);
    {
        ScopedTransaction outer(m_stage, m_stage->GetSessionLayer());
        EXPECT_EQ(opened(), 1u);
        EXPECT_EQ(closed(), 0u);
        {
            /// Opening a transaction for same layer should not trigger notices
            ScopedTransaction inner(m_stage, m_stage->GetSessionLayer());
            EXPECT_EQ(opened(), 1u);
            EXPECT_EQ(closed(), 0u);
        }
        EXPECT_EQ(opened(), 1u);
        EXPECT_EQ(closed(), 0u);
        {
            /// Opening a transaction for different layer should trigger notices
            ScopedTransaction inner(m_stage, m_stage->GetRootLayer());
            EXPECT_EQ(opened(), 2u);
            EXPECT_EQ(closed(), 0u);
        }
        EXPECT_EQ(opened(), 2u);
        EXPECT_EQ(closed(), 1u);
    }
    EXPECT_EQ(closed(), 2u);
}

/// Test that CloseNotice reports changes as expected
TEST_F(TransactionTest, Changes)
{
    EXPECT_EQ(sorted(getChanged()), empty());
    EXPECT_EQ(sorted(getResynced()), empty());
    {
        ScopedTransaction transaction(m_stage, m_stage->GetSessionLayer());
        createPrimWithAttribute("/A");
    }
    EXPECT_EQ(sorted(getChanged()), empty());
    EXPECT_EQ(sorted(getResynced()), sorted({ "/A" }));
    {
        ScopedTransaction transaction(m_stage, m_stage->GetSessionLayer());
        changePrimAttribute("/A", 2);
    }
    EXPECT_EQ(sorted(getChanged()), sorted({ "/A.prop" }));
    EXPECT_EQ(sorted(getResynced()), empty());
    {
        ScopedTransaction transaction(m_stage, m_stage->GetSessionLayer());
        changePrimAttribute("/A", 4);
        changePrimAttribute("/A", 2); /// effectively no change
    }
    EXPECT_EQ(sorted(getChanged()), empty());
    EXPECT_EQ(sorted(getResynced()), empty());
}

/// Test that CloseNotice reports hierarchy changes as expected
TEST_F(TransactionTest, Hierarchy)
{
    EXPECT_EQ(sorted(getChanged()), empty());
    EXPECT_EQ(sorted(getResynced()), empty());
    {
        ScopedTransaction transaction(m_stage, m_stage->GetSessionLayer());
        createPrimWithAttribute("/root");
        createPrimWithAttribute("/root/A");
        createPrimWithAttribute("/root/A/C");
        createPrimWithAttribute("/root/A/D");
        createPrimWithAttribute("/root/B");
        createPrimWithAttribute("/root/B/E");
    }
    EXPECT_EQ(sorted(getChanged()), empty());
    EXPECT_EQ(sorted(getResynced()), sorted({ "/root" }));
    {
        ScopedTransaction transaction(m_stage, m_stage->GetSessionLayer());
        changePrimAttribute("/root", 2);
        changePrimAttribute("/root/A/D", 2);
    }
    EXPECT_EQ(sorted(getChanged()), sorted({ "/root.prop", "/root/A/D.prop" }));
    EXPECT_EQ(sorted(getResynced()), empty());
    {
        ScopedTransaction transaction(m_stage, m_stage->GetSessionLayer());
        changePrimAttribute("/root/B", 2);
        changePrimAttribute("/root/A/C", 2);
        changePrimAttribute("/root/A/C", 1); /// effectively no change
    }
    EXPECT_EQ(sorted(getChanged()), sorted({ "/root/B.prop" }));
    EXPECT_EQ(sorted(getResynced()), empty());
    {
        ScopedTransaction transaction(m_stage, m_stage->GetSessionLayer());
        createPrimWithAttribute("/root/B/F");
    }
    EXPECT_EQ(sorted(getChanged()), empty());
    EXPECT_EQ(sorted(getResynced()), sorted({ "/root/B/F" }));
}

/// Test that CloseNotice reports property changes as expected
TEST_F(TransactionTest, Properties)
{
    EXPECT_EQ(sorted(getChanged()), empty());
    EXPECT_EQ(sorted(getResynced()), empty());
    {
        ScopedTransaction transaction(m_stage, m_stage->GetSessionLayer());
        createPrimWithAttribute("/root", "foo");
        createPrimWithAttribute("/root/A", nullptr);
    }
    EXPECT_EQ(sorted(getChanged()), empty());
    EXPECT_EQ(sorted(getResynced()), sorted({ "/root" }));
    {
        ScopedTransaction transaction(m_stage, m_stage->GetSessionLayer());
        createPrimWithAttribute("/root", "bar");
        createPrimWithAttribute("/root/A", "foo");
    }
    EXPECT_EQ(sorted(getChanged()), sorted({ "/root.bar", "/root/A.foo" }));
    EXPECT_EQ(sorted(getResynced()), empty());
    {
        ScopedTransaction transaction(m_stage, m_stage->GetSessionLayer());
        changePrimAttribute("/root", 2, "foo");
        changePrimAttribute("/root", 4, "bar");
        changePrimAttribute("/root", 1, "bar"); /// effectively no change
        changePrimAttribute("/root/A", 2, "foo");
    }
    EXPECT_EQ(sorted(getChanged()), sorted({ "/root.foo", "/root/A.foo" }));
    EXPECT_EQ(sorted(getResynced()), empty());
}

/// Test that CloseNotice reports clearing layers as expected
TEST_F(TransactionTest, Clear)
{
    EXPECT_EQ(sorted(getChanged()), empty());
    EXPECT_EQ(sorted(getResynced()), empty());
    {
        ScopedTransaction transaction(m_stage, m_stage->GetSessionLayer());
        createPrimWithAttribute("/root");
        createPrimWithAttribute("/root/A");
        createPrimWithAttribute("/root/A/B");
    }
    EXPECT_EQ(sorted(getChanged()), empty());
    EXPECT_EQ(sorted(getResynced()), sorted({ "/root" }));
    {
        ScopedTransaction transaction(m_stage, m_stage->GetSessionLayer());
        m_stage->GetSessionLayer()->Clear();
    }
    EXPECT_EQ(sorted(getChanged()), empty());
    EXPECT_EQ(sorted(getResynced()), sorted({ "/root" }));
    {
        ScopedTransaction transaction(m_stage, m_stage->GetSessionLayer());
        createPrimWithAttribute("/root");
        createPrimWithAttribute("/root/A");
        createPrimWithAttribute("/root/A/B");
    }
    EXPECT_EQ(sorted(getChanged()), empty());
    EXPECT_EQ(sorted(getResynced()), sorted({ "/root" }));
    {
        ScopedTransaction transaction(m_stage, m_stage->GetSessionLayer());
        m_stage->GetSessionLayer()->Clear();
        createPrimWithAttribute("/root");
        createPrimWithAttribute("/root/A");
        createPrimWithAttribute("/root/A/B");
        /// effectively no change
    }
    EXPECT_EQ(sorted(getChanged()), empty());
    EXPECT_EQ(sorted(getResynced()), empty());
}
