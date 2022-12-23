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

PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_ENV_SETTING(
    MAYAUSD_SHOW_FULL_DIAGNOSTICS,
    false,
    "This env flag controls the granularity of TF error/warning/status messages "
    "being displayed in Maya.");

namespace {

class DiagnosticFlusher;

std::unique_ptr<UsdUtilsCoalescingDiagnosticDelegate> _batchedStatuses;
std::unique_ptr<UsdUtilsCoalescingDiagnosticDelegate> _batchedWarnings;
std::unique_ptr<UsdUtilsCoalescingDiagnosticDelegate> _batchedErrors;
std::unique_ptr<DiagnosticFlusher>                    _flusher;
std::atomic<bool>                                     _hasPendingDiagnostic;

// The delegate can be installed by multiple plugins (e.g. pxrUsd and
// mayaUsdPlugin), so keep track of installations to ensure that we only add
// the delegate for the first installation call, and that we only remove it for
// the last removal call.
std::atomic<int> _installationCount = 0;

/// @brief USD diagnostic delegate that accumulates all status messages.
class StatusOnlyDelegate : public UsdUtilsCoalescingDiagnosticDelegate
{
    void IssueWarning(const TfWarning&) override { _hasPendingDiagnostic = true; }
    void IssueError(const TfError&) override { _hasPendingDiagnostic = true; }
    void IssueFatalError(const TfCallContext&, const std::string&) override { }
};

/// @brief USD diagnostic delegate that accumulates all warning messages.
class WarningOnlyDelegate : public UsdUtilsCoalescingDiagnosticDelegate
{
    void IssueStatus(const TfStatus&) override { _hasPendingDiagnostic = true; }
    void IssueError(const TfError&) override { }
    void IssueFatalError(const TfCallContext&, const std::string&) override { }
};

/// @brief USD diagnostic delegate that accumulates all error messages.
class ErrorOnlyDelegate : public UsdUtilsCoalescingDiagnosticDelegate
{
    void IssueWarning(const TfWarning&) override { }
    void IssueStatus(const TfStatus&) override { }
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
        _periodicThread = std::make_unique<std::thread>([this]() { periodicFlusher(); });
    }

    ~DiagnosticFlusher()
    {
        if (_periodicThread) {
            _periodicThread->join();
            _periodicThread.reset();
        }
    }

    void forceFlush()
    {
        if (ArchIsMainThread()) {
            flushPerformedInMainThread();
        } else {
            _forceFlush = true;
            std::unique_lock<std::mutex> lock(_pendingDiagnosticsMutex);
            _pendingDiagnosticCond.notify_all();
        }
    }

private:
    using Clock = std::chrono::steady_clock;
    template <class Duration> using TimePoint = std::chrono::time_point<Clock, Duration>;
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

    static void flushDiagnostics(
        const std::unique_ptr<UsdUtilsCoalescingDiagnosticDelegate>& delegate,
        const std::function<void(const MString&)>&                   printer)
    {
        const UsdUtilsCoalescingDiagnosticDelegateVector messages = delegate
            ? delegate->TakeCoalescedDiagnostics()
            : UsdUtilsCoalescingDiagnosticDelegateVector();
        for (const UsdUtilsCoalescingDiagnosticDelegateItem& item : messages) {
            printer(formatCoalescedDiagnostic(item));
        }
    }

    void flushPerformedInMainThread()
    {
        TF_AXIOM(ArchIsMainThread());

        {
            std::unique_lock<std::mutex> lock(_pendingDiagnosticsMutex);
            _triggeredFlush = false;
            _hasPendingDiagnostic = false;
        }

        // Note that we must be in the main thread here, so it's safe to call
        // displayInfo/displayWarning.
        flushDiagnostics(_batchedStatuses, MGlobal::displayInfo);
        flushDiagnostics(_batchedWarnings, MGlobal::displayWarning);
        flushDiagnostics(_batchedErrors, MGlobal::displayError);
    }

    static void flushPerformedInMainThreadCallback(void* data)
    {
        DiagnosticFlusher* self = static_cast<DiagnosticFlusher*>(data);
        self->flushPerformedInMainThread();
    }

    void triggerFlushInMainThread()
    {
        if (_triggeredFlush)
            return;
        _triggeredFlush = true;
        MGlobal::executeTaskOnIdle(flushPerformedInMainThreadCallback, this);
    }

    void periodicFlusher()
    {
        using namespace std::chrono_literals;

        while (_installationCount > 0) {
            try {
                std::unique_lock<std::mutex> lock(_pendingDiagnosticsMutex);
                _pendingDiagnosticCond.wait_for(lock, 1s);
                if (_forceFlush) {
                    _forceFlush = false;
                    triggerFlushInMainThread();
                } else if (_hasPendingDiagnostic) {
                    const auto now = Clock::now();
                    const auto elapsed = std::chrono::duration<double>(now - _lastFlushTime);
                    if (elapsed.count() >= 1.) {
                        _lastFlushTime = now;
                        triggerFlushInMainThread();
                    }
                }
            } catch (const std::exception&) {
                // Do nothing.
            }
        }
    }

    std::unique_ptr<std::thread> _periodicThread;

    std::mutex              _pendingDiagnosticsMutex;
    std::condition_variable _pendingDiagnosticCond;
    Clock::time_point       _lastFlushTime;
    std::atomic<bool>       _forceFlush = false;
    std::atomic<bool>       _triggeredFlush = false;
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

    _flusher = std::make_unique<DiagnosticFlusher>();
    _batchedStatuses = std::make_unique<StatusOnlyDelegate>();
    _batchedWarnings = std::make_unique<WarningOnlyDelegate>();
    _batchedErrors = std::make_unique<ErrorOnlyDelegate>();
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

PXR_NAMESPACE_CLOSE_SCOPE
