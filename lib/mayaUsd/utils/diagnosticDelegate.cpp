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

#include <ghc/filesystem.hpp>

#include <atomic>
#include <chrono>
#include <memory>
#include <thread>
#include <condition_variable>

PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_ENV_SETTING(
    MAYAUSD_SHOW_FULL_DIAGNOSTICS,
    false,
    "This env flag controls the granularity of TF error/warning/status messages "
    "being displayed in Maya.");

TF_DEFINE_ENV_SETTING(
    MAYAUSD_MAXIMUM_UNBATCHED_DIAGNOSTICS,
    100,
    "This env flag controls the maximum number of diagnostic messages that can "
    "be emitted in one second before automatic batching of messages is used.");

namespace {

class DiagnosticFlusher;

std::unique_ptr<UsdUtilsCoalescingDiagnosticDelegate> _batchedStatuses;
std::unique_ptr<UsdUtilsCoalescingDiagnosticDelegate> _batchedWarnings;
std::unique_ptr<UsdUtilsCoalescingDiagnosticDelegate> _batchedErrors;
std::unique_ptr<TfDiagnosticMgr::Delegate>            _waker;
std::unique_ptr<DiagnosticFlusher>                    _flusher;

// The delegate can be installed by multiple plugins (e.g. pxrUsd and
// mayaUsdPlugin), so keep track of installations to ensure that we only add
// the delegate for the first installation call, and that we only remove it for
// the last removal call.
std::atomic<int> _installationCount;

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
///
/// The design goes like this:
///
///     - All messages are accumulated by the above delegates.
///     - A thread, periodicFlusher, wakes up every second to conditionally flush
///       pending messages.
///     - The condition for flushing are that a forced flush is requested or some messages
//        have been received and one second has elapsed.
///     - Requesting a flushing of accumulated messages is done by queuing a task to be
///       run on idle in the main thread. If a task is already queued, nothing is done.
///     - The main-thread task takes (extract and removes) all accumulted messages and
///       prints them in the script console via MGlobal.
///     - This can only be done in the main thread because that is what MGlobal supports.
class DiagnosticFlusher
{
public:
    DiagnosticFlusher()
    {
        // Note: the periodic thread access member variables, so it must be initialized
        //       after all members are initialized. That is why we create it in the
        //       constructor body and not the initialization list.
        _periodicThread = std::make_unique<std::thread>([this]() { periodicFlusher(); });
    }

    ~DiagnosticFlusher()
    {
        try {
            if (_periodicThread) {
                _periodicThread->join();
                _periodicThread.reset();
            }
        } catch (const std::exception&) {
        }
    }

    void forceFlush()
    {
        ElapsedTimePoint elapsed;
        {
            std::unique_lock<std::mutex> lock(_pendingDiagnosticsMutex);
            elapsed = getElapsedSeconds();
            elapsed.second = TimePoint<Clock::duration>();
            _burstDiagnosticCount = 0;
        }
        triggerFlushInMainThread(elapsed);
    }

    void setMaximumUnbatchedDiagnostics(int count) { _maximumUnbatchedDiagnostics = count; }

    void receivedDiagnostic()
    {
        // On the first diagnostic message, check how long since we flushed the diagnostics.
        // If it is less than a minimum, we assume we are in a burst of messages and delay
        // writing messages.
        //
        // If it is the first message in a long time, we flush it immediately.
        if (_pendingDiagnosticCount.fetch_add(1) >= _maximumUnbatchedDiagnostics)
            return;

        ElapsedTimePoint elapsed;
        {
            std::unique_lock<std::mutex> lock(_pendingDiagnosticsMutex);
            elapsed = getElapsedSeconds();
            if (elapsed.first < _flushingPeriod) {
                // Note: because the first of a burst was outside of the flushinh period,
                //       we must take the maximum minus one to account for that unaccounted
                //       diagnostic.
                if (_burstDiagnosticCount.fetch_add(1) >= _maximumUnbatchedDiagnostics - 1) {
                    return;
                }
            } else {
                _burstDiagnosticCount = 0;
            }
        }

        triggerFlushInMainThread(elapsed);
    }

private:
    using Clock = std::chrono::steady_clock;
    template <class Duration> using TimePoint = std::chrono::time_point<Clock, Duration>;
    using Sec = std::chrono::seconds;
    using ElapsedTimePoint = std::pair<double, Clock::time_point>;

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

        bool printBatched = false;
        {
            std::unique_lock<std::mutex> lock(_pendingDiagnosticsMutex);
            _triggeredFlush = false;
            printBatched = _pendingDiagnosticCount > _maximumUnbatchedDiagnostics;
            _pendingDiagnosticCount = 0;
        }

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

    void triggerFlushInMainThread(const ElapsedTimePoint& elapsed)
    {
        _lastFlushTime = elapsed.second;
        if (ArchIsMainThread()) {
            flushPerformedInMainThread();
        } else {
            if (_triggeredFlush)
                return;
            _triggeredFlush = true;
            MGlobal::executeTaskOnIdle(flushPerformedInMainThreadCallback, this);
        }
    }

    ElapsedTimePoint getElapsedSeconds() const
    {
        const auto now = Clock::now();
        const auto elapsed = std::chrono::duration<double>(now - _lastFlushTime);
        return std::make_pair(elapsed.count(), now);
    }

    void periodicFlusher()
    {
        using namespace std::chrono_literals;

        while (_installationCount > 0) {
            try {
                std::unique_lock<std::mutex> lock(_pendingDiagnosticsMutex);
                _pendingDiagnosticCond.wait_for(lock, 1s);

                if (_pendingDiagnosticCount == 0)
                    continue;

                const ElapsedTimePoint elapsed = getElapsedSeconds();
                if (elapsed.first >= _flushingPeriod) {
                    triggerFlushInMainThread(elapsed);
                }
            } catch (const std::exception&) {
                // Do nothing.
            }
        }
    }

    std::unique_ptr<std::thread> _periodicThread;

    std::mutex              _pendingDiagnosticsMutex;
    std::condition_variable _pendingDiagnosticCond;
    Clock::time_point       _lastFlushTime = Clock::now();
    std::atomic<bool>       _triggeredFlush;
    std::atomic<int>        _pendingDiagnosticCount;
    std::atomic<int>        _burstDiagnosticCount;
    int _maximumUnbatchedDiagnostics = TfGetEnvSetting<int>(MAYAUSD_MAXIMUM_UNBATCHED_DIAGNOSTICS);

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

PXR_NAMESPACE_CLOSE_SCOPE
