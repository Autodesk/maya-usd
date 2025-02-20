//
// Copyright 2018 Pixar
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
#include "diagnosticDelegate.h"

#include <mayaUsd/base/debugCodes.h>

#include <pxr/base/arch/threads.h>
#include <pxr/base/tf/envSetting.h>
#include <pxr/base/tf/stackTrace.h>

#include <maya/MGlobal.h>
#include <maya/MSceneMessage.h>

#include <ghc/filesystem.hpp>

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <limits>
#include <memory>
#include <thread>

PXR_NAMESPACE_OPEN_SCOPE

// The design goes like this:
//
//     - All messages are accumulated by the diagnostic delegates. (See below.)
//     - Another delegate (see below) tells the diagnostic message flusher when
//       any message arrives.
//     - The diagnostic message flusher has two purposes:
//       - the first purpose is to detect bursts of messages and to withhold
//         further messages from being written out when a burst is detected,
//       - the second purpose is to write out (flush) the messages periodically.
//     - The condition for flushing are either:
//       - that a forced flush is requested,
//       - or that fewer than a maximum consecutive messages have been received,
//       - or that one second has elapsed since the last time messages were flushed.
//     - Flushing can either be immediate or delayed.
//       - Immediate flushing is done when a forced flush is requested or when
//         *outside* of bursts of messages.
//       - Delayed flushing is done when a burst of messages is detected,
//         to avoid writing too many messages in the log.
//     - Requesting a flushing of accumulated messages is done either directly
//       or indirectly.
//       - Direct flushing is done when the flushing request is triggered in the
//         main thread.
//       - Indirect flushing is done by queuing a task to be run on-idle in the
//         main thread. If a task is already queued, nothing is done, to avoid
//         queuing multiple redundant tasks to do the same thing.
//     - The actual flushing takes (extract and removes) all accumulated messages
//       and prints them in the script console via MGlobal.
//     - This can only be done in the main thread due to the limitations of MGlobal.
//     - Printing of messages is done either fully or coalesced.
//       - All messages are printed fully when not in a burst.
//       - All messages are printed coalesced when in a burst. Coalesced messages
//         only print a sample of the message followed by "and X similar".

TF_DEFINE_ENV_SETTING(
    PIXMAYA_DIAGNOSTICS_BATCH,
    true,
    "Whether to batch diagnostics coming from the same call site.");

TF_DEFINE_ENV_SETTING(
    MAYAUSD_SHOW_FULL_DIAGNOSTICS,
    false,
    "This env flag controls the granularity of TF error/warning/status messages "
    "being displayed in Maya.");

TF_DEFINE_ENV_SETTING(
    MAYAUSD_MAXIMUM_UNBATCHED_DIAGNOSTICS,
    10,
    "This env flag controls the maximum number of diagnostic messages that can "
    "be emitted in one second before automatic batching of messages is used.");

namespace {

class DiagnosticFlusher;

std::unique_ptr<UsdUtilsCoalescingDiagnosticDelegate> _batchedStatuses;
std::unique_ptr<UsdUtilsCoalescingDiagnosticDelegate> _batchedWarnings;
std::unique_ptr<UsdUtilsCoalescingDiagnosticDelegate> _batchedErrors;
std::unique_ptr<TfDiagnosticMgr::Delegate>            _waker;
std::unique_ptr<DiagnosticFlusher>                    _flusher;
MCallbackId                                           _exitCallback;

// The delegate can be installed by multiple plugins (e.g. pxrUsd and
// mayaUsdPlugin), so keep track of installations to ensure that we only add
// the delegate for the first installation call, and that we only remove it for
// the last removal call.
std::atomic<int> _installationCount;

std::atomic<int> _batchedContextCount;

bool IsDiagnosticBatchingEnabled() { return TfGetEnvSetting(PIXMAYA_DIAGNOSTICS_BATCH); }

/// @brief USD diagnostic delegate that accumulates all status messages.
class StatusOnlyDelegate : public UsdUtilsCoalescingDiagnosticDelegate
{
    void IssueWarning(const TfWarning&) override { }
    void IssueError(const TfError&) override { }
    void IssueFatalError(const TfCallContext&, const std::string&) override { }
};

/// @brief USD diagnostic delegate that accumulates all warning messages.
class WarningOnlyDelegate : public UsdUtilsCoalescingDiagnosticDelegate
{
    void IssueStatus(const TfStatus&) override { }
    void IssueError(const TfError&) override { }
    void IssueFatalError(const TfCallContext&, const std::string&) override { }
};

/// @brief USD diagnostic delegate that accumulates all error messages.
class ErrorOnlyDelegate : public UsdUtilsCoalescingDiagnosticDelegate
{
    void IssueWarning(const TfWarning&) override { }
    void IssueStatus(const TfStatus&) override { }
    void IssueError(const TfError& err) override
    {
        // Note: UsdUtilsCoalescingDiagnosticDelegate does not coalesces errors!
        //       So, the only way to make it keep errors is to convert the error
        //       into a warning.
        //
        // Note: USD warnings and errors have the exact same layout, only a different concrete type.
        //       More-over, USD made all its diagnostic classes have private constructors! So
        //       the only way to convert an error into a warning is through a rei-interpret cast.
        const TfWarning& warning = reinterpret_cast<const TfWarning&>(err);
        UsdUtilsCoalescingDiagnosticDelegate ::IssueWarning(warning);
    }
    void IssueFatalError(const TfCallContext& context, const std::string& msg) override
    {
        UsdMayaDiagnosticDelegate::Flush();

        // Note: TfLogCrash will do a hard abort of the process.
        //       The dynamic libraries will be unloaded in a random order and
        //       global variable will be destroyed. Unfortunately, USD holds a
        //       mutex while in the TfLogCrash function and trying to remove the
        //       diagnostic delegate (_waker) will also try to hold that mutex
        //       resulting in a dead-lock. Avoid this by avoiding to destroy
        //       the delegates
        //
        //       Yes, this means we are leaking the delegates, but we are in
        //       the middle of crashing anyway, so it does not really matter.

        _waker.release();
        _flusher.release();
        _batchedStatuses.release();
        _batchedWarnings.release();
        _batchedErrors.release();

        TfLogCrash(
            "FATAL ERROR",
            msg,
            /*additionalInfo*/ std::string(),
            context,
            /*logToDb*/ true);
        _UnhandledAbort();
    }
};

/// @brief Periodically flushes the accumulated messages.
class DiagnosticFlusher
{
public:
    DiagnosticFlusher()
    {
        if (!IsDiagnosticBatchingEnabled()) {
            _maximumUnbatchedDiagnostics = std::numeric_limits<int>::max();
        }
    }

    ~DiagnosticFlusher() { }

    /// @brief Force all mesage to be immediately flushed.
    void forceFlush()
    {
        _burstDiagnosticCount = 0;
        resetLastFlushTime();
        triggerFlushInMainThread();
    }

    /// @brief Sets the maximum number of consecutive messages before they
    ///        are considered a burst.
    void setMaximumUnbatchedDiagnostics(int count) { _maximumUnbatchedDiagnostics = count; }

    /// @brief Gets the maximum number of consecutive messages before they
    ///        are considered a burst.
    int getMaximumUnbatchedDiagnostics() const { return _maximumUnbatchedDiagnostics; }

    /// @brief Gets the default maximum number of consecutive messages
    ///        before they are considered a burst.
    static int getDefaultMaximumUnbatchedDiagnostics()
    {
        return TfGetEnvSetting<int>(MAYAUSD_MAXIMUM_UNBATCHED_DIAGNOSTICS);
    }

    /// @brief Called when a diagnostic message is created to be printed.
    void receivedDiagnostic()
    {
        // On the first diagnostic message, check how long since we flushed the diagnostics.
        // If it is less than a minimum, we assume we are in a burst of messages and delay
        // writing messages.
        //
        // If it is the first message in a long time, we flush it immediately.
        const int burstCount = _burstDiagnosticCount.fetch_add(1);
        if (_pendingDiagnosticCount.fetch_add(1) >= _maximumUnbatchedDiagnostics)
            return triggerFlushInMainThreadLaterIfNeeded();

        const double elapsed = getElapsedSecondsSinceLastFlush();
        if (elapsed < _flushingPeriod) {
            if (burstCount >= _maximumUnbatchedDiagnostics) {
                return triggerFlushInMainThreadLaterIfNeeded();
            }
        } else {
            // Note: clear the burst count since the time elapsed since the last
            //       diagnostic is greater than the flushing period. We reset to
            //       one instead of zero since this message is part of the new
            //       potential burst of diagnostic messages.
            _burstDiagnosticCount = 1;
        }

        triggerFlushInMainThreadIfNeeded();
    }

private:
    using Clock = std::chrono::steady_clock;
    using TimePoint = Clock::time_point;
    using Sec = std::chrono::seconds;

    static MString formatCoalescedDiagnostic(const UsdUtilsCoalescingDiagnosticDelegateItem& item)
    {
        const size_t      numItems = item.unsharedItems.size();
        const std::string suffix
            = numItems == 1 ? std::string() : TfStringPrintf(" -- and %zu similar", numItems - 1);
        const std::string message
            = TfStringPrintf("%s%s", item.unsharedItems[0].commentary.c_str(), suffix.c_str());

        return message.c_str();
    }

    static MString formatDiagnostic(const std::unique_ptr<TfDiagnosticBase>& item)
    {
        if (!item)
            return MString();
        if (!TfGetEnvSetting(MAYAUSD_SHOW_FULL_DIAGNOSTICS)) {
            return item->GetCommentary().c_str();
        } else {
            const std::string msg = TfStringPrintf(
                "%s -- %s in %s at line %zu of %s",
                item->GetCommentary().c_str(),
                TfDiagnosticMgr::GetCodeName(item->GetDiagnosticCode()).c_str(),
                item->GetContext().GetFunction(),
                item->GetContext().GetLine(),
                ghc::filesystem::path(item->GetContext().GetFile())
                    .relative_path()
                    .string()
                    .c_str());
            return msg.c_str();
        }
    }

    static void flushDiagnostics(
        const std::unique_ptr<UsdUtilsCoalescingDiagnosticDelegate>& delegate,
        const bool                                                   printBatched,
        const std::function<void(const MString&)>&                   printer)
    {
        if (!delegate)
            return;

        if (printBatched) {
            const UsdUtilsCoalescingDiagnosticDelegateVector messages
                = delegate->TakeCoalescedDiagnostics();
            for (const UsdUtilsCoalescingDiagnosticDelegateItem& item : messages) {
                printer(formatCoalescedDiagnostic(item));
            }
        } else {
            std::vector<std::unique_ptr<TfDiagnosticBase>> messages
                = delegate->TakeUncoalescedDiagnostics();
            for (const std::unique_ptr<TfDiagnosticBase>& item : messages) {
                printer(formatDiagnostic(item));
            }
        }
    }

    void flushPerformedInMainThread()
    {
        TF_AXIOM(ArchIsMainThread());

        _triggeredFlush = false;
        const bool printBatched
            = _pendingDiagnosticCount.fetch_and(0) > _maximumUnbatchedDiagnostics;

        updateLastFlushTime();

        // Note that we must be in the main thread here, so it's safe to call
        // displayInfo/displayWarning.
        flushDiagnostics(_batchedStatuses, printBatched, MGlobal::displayInfo);
        flushDiagnostics(_batchedWarnings, printBatched, MGlobal::displayWarning);
        flushDiagnostics(_batchedErrors, printBatched, MGlobal::displayError);
    }

    static void flushPerformedInMainThreadCallback(void* data)
    {
        DiagnosticFlusher* self = static_cast<DiagnosticFlusher*>(data);
        self->flushPerformedInMainThread();
    }

    void triggerFlushInMainThreadLaterIfNeeded()
    {
        if (_triggeredFlush)
            return;
        _triggeredFlush = true;

        // Note: the periodic thread access member variables, so it must be initialized
        //       after all members are initialized. That is why we create it in the
        //       constructor body and not the initialization list.
        std::thread([this]() {
            using namespace std::chrono_literals;
            std::this_thread::sleep_for(1s);
            triggerFlushInMainThread();
        }).detach();
    }

    void triggerFlushInMainThreadIfNeeded()
    {
        if (_triggeredFlush)
            return;
        _triggeredFlush = true;
        triggerFlushInMainThread();
    }

    void triggerFlushInMainThread()
    {
        if (ArchIsMainThread()) {
            flushPerformedInMainThread();
        } else {
            MGlobal::executeTaskOnIdle(flushPerformedInMainThreadCallback, this);
        }
    }

    static TimePoint getElapsedTimePoint() { return Clock::now(); }

    void updateLastFlushTime()
    {
        const TimePoint              now = getElapsedTimePoint();
        std::unique_lock<std::mutex> lock(_pendingDiagnosticsMutex);
        _lastFlushTime = now;
    }

    void resetLastFlushTime()
    {
        std::unique_lock<std::mutex> lock(_pendingDiagnosticsMutex);
        _lastFlushTime = TimePoint();
    }

    double getElapsedSecondsSinceLastFlush()
    {
        const auto                   now = getElapsedTimePoint();
        std::unique_lock<std::mutex> lock(_pendingDiagnosticsMutex);
        return std::chrono::duration<double>(now - _lastFlushTime).count();
    }

    std::mutex        _pendingDiagnosticsMutex;
    TimePoint         _lastFlushTime = TimePoint();
    std::atomic<bool> _triggeredFlush;
    std::atomic<int>  _pendingDiagnosticCount;
    std::atomic<int>  _burstDiagnosticCount;

    int _maximumUnbatchedDiagnostics = getDefaultMaximumUnbatchedDiagnostics();

    static constexpr double _flushingPeriod = 1.;
};

/// @brief USD diagnostic delegate that wakes up the flushing thread.
class WakeUpDelegate : public TfDiagnosticMgr::Delegate
{
public:
    WakeUpDelegate() { TfDiagnosticMgr::GetInstance().AddDelegate(this); }
    ~WakeUpDelegate()
    {
        try {
            TfDiagnosticMgr::GetInstance().RemoveDelegate(this);
        } catch (const std::exception&) {
        }
    }

    void IssueError(const TfError&) override { _flusher->receivedDiagnostic(); }
    void IssueWarning(const TfWarning&) override { _flusher->receivedDiagnostic(); }
    void IssueStatus(const TfStatus&) override { _flusher->receivedDiagnostic(); }
    void IssueFatalError(const TfCallContext&, const std::string&) override { }
};

void beforeExitCallback(void* /*clientData*/)
{
    // Make sure the diagnostic messages are flushed when Maya exits.
    UsdMayaDiagnosticDelegate::Flush();
}

} // anonymous namespace

void UsdMayaDiagnosticDelegate::InstallDelegate()
{
    if (!ArchIsMainThread()) {
        // Don't crash, but inform user about failure to install
        // the USD diagnostic message handler.
        TF_RUNTIME_ERROR(
            "Cannot install the USD diagnostic message printer from a secondary thread");
        return;
    }

    if (_installationCount++ > 0) {
        return;
    }

    _batchedStatuses = std::make_unique<StatusOnlyDelegate>();
    _batchedWarnings = std::make_unique<WarningOnlyDelegate>();
    _batchedErrors = std::make_unique<ErrorOnlyDelegate>();

    // Note: flusher accesses the batched status, so the flusher must be created
    //       after the batcher.
    _flusher = std::make_unique<DiagnosticFlusher>();

    // Note: waker accesses the flusher, so the waker must be created
    //       after the flusher.
    _waker = std::make_unique<WakeUpDelegate>();

    _exitCallback = MSceneMessage::addCallback(MSceneMessage::kMayaExiting, beforeExitCallback);
}

void UsdMayaDiagnosticDelegate::RemoveDelegate()
{
    if (!ArchIsMainThread()) {
        // Don't crash, but inform user about failure to remove
        // the USD diagnostic message handler.
        TF_RUNTIME_ERROR(
            "Cannot remove the USD diagnostic message printer from a secondary thread");
        return;
    }

    if (_installationCount == 0 || _installationCount-- > 1) {
        return;
    }

    Flush();

    MMessage::removeCallback(_exitCallback);

    // Note: waker accesses the flusher, so the waker must be destroyed
    //       before the flusher.
    _waker.reset();

    // Note: flusher accesses the batched status, so the flusher must be destroyed
    //       before the batcher.
    _flusher.reset();

    _batchedStatuses.reset();
    _batchedWarnings.reset();
    _batchedErrors.reset();
}

void UsdMayaDiagnosticDelegate::Flush()
{
    if (_flusher)
        _flusher->forceFlush();
}

void UsdMayaDiagnosticDelegate::SetMaximumUnbatchedDiagnostics(int count)
{
    if (_flusher)
        _flusher->setMaximumUnbatchedDiagnostics(count);
}

int UsdMayaDiagnosticDelegate::GetMaximumUnbatchedDiagnostics()
{
    if (_flusher)
        return _flusher->getMaximumUnbatchedDiagnostics();
    else
        return DiagnosticFlusher::getDefaultMaximumUnbatchedDiagnostics();
}

MAYAUSD_VERIFY_CLASS_NOT_MOVE_OR_COPY(UsdMayaDiagnosticBatchContext);

UsdMayaDiagnosticBatchContext::UsdMayaDiagnosticBatchContext(int maximumUnbatchedCount)
    : previousCount(UsdMayaDiagnosticDelegate::GetMaximumUnbatchedDiagnostics())
{
    if (!IsDiagnosticBatchingEnabled())
        return;

    TF_DEBUG(PXRUSDMAYA_DIAGNOSTICS).Msg(">> Entering batch context\n");

    UsdMayaDiagnosticDelegate::SetMaximumUnbatchedDiagnostics(maximumUnbatchedCount);

    _batchedContextCount.fetch_add(1);
}

UsdMayaDiagnosticBatchContext::~UsdMayaDiagnosticBatchContext()
{
    if (!IsDiagnosticBatchingEnabled())
        return;

    TF_DEBUG(PXRUSDMAYA_DIAGNOSTICS).Msg("!! Exiting batch context\n");

    UsdMayaDiagnosticDelegate::SetMaximumUnbatchedDiagnostics(previousCount);

    if (_batchedContextCount.fetch_sub(1) <= 1)
        UsdMayaDiagnosticDelegate::Flush();
}

PXR_NAMESPACE_CLOSE_SCOPE
