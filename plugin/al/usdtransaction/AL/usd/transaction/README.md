# Transaction

This module provides simple batching functionality for clients that are interested in sparse notifications when many small changes are performed.

Transaction is defined for given `stage` and `layer`. When transaction is opened current state of layer is registered and will be compared with state upon transaction close.

It's possible to open same transaction (identified by `stage` and `layer` pair) multiple times, however state and notices will be emitted only for outermost pair.

**Note:** It's client responsibility to pair `Open` and `Close` calls, otherwise clients might stop responding to updates. As such it's advised to use helper class `ScopedTransaction` whenever possible.


## Support for transactions

When client is interested in supporting transactions just a couple steps are required. This example is based on implementation in `UsdProcedural` from `USDGlimpse`.

1. Register for transaction notifications same like for `ObjectChanged` notice
```
    TfWeakPtr<UsdProcedural> me(this);
    m_transactionNoticeKey = TfNotice::Register(me, &UsdProcedural::onTransaction, stage);
```
2. Disregard intermediate `ObjectChanged` notices during transaction
```
  void UsdProcedural::onObjectsChanged(UsdNotice::ObjectsChanged const& notice, UsdStageWeakPtr const& sender)
  {
    (...)
    /// If there are no transactions in process, update content immediately.
    /// Otherwise transaction close notice will provide information about changes.
    if (!AL::usd::transaction::Manager::InProgress(sender))
    {
      processPaths(SdfPathVector(notice.GetChangedInfoOnlyPaths()), m_changedPaths);
      processPaths(SdfPathVector(notice.GetResyncedPaths()), m_resynchedPaths);
      updateContent();
    }
  }
```
3. Respond to transaction close notice similarly to `ObjectChanged` handling
```
  void UsdProcedural::onTransaction(const AL::usd::transaction::CloseNotice& notice, const UsdStageWeakPtr& sender)
  {
    processPaths(notice.GetChangedInfoOnlyPaths(), m_changedPaths);
    processPaths(notice.GetResyncedPaths(), m_resynchedPaths);
    updateContent();
  }
```

## Using transactions

Using transactions is even simpler. Just create `ScopedTransaction` instance / context.

```
AL::usd::transaction::ScopedTransaction transaction(stage, layer);
/// perform some operations
/// going out of scope will close transaction
```

Alternatively in python:

```
with AL.usd.transaction.ScopedTransaction(stage, layer):
    ## perform some operations
    ## going out of context will close transaction
```

See unit tests for more information.