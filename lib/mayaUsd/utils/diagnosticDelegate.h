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
#ifndef PXRUSDMAYA_DIAGNOSTICDELEGATE_H
#define PXRUSDMAYA_DIAGNOSTICDELEGATE_H

#include <mayaUsd/base/api.h>

#include <pxr/base/tf/diagnosticMgr.h>
#include <pxr/pxr.h>
#include <pxr/usd/usdUtils/coalescingDiagnosticDelegate.h>

PXR_NAMESPACE_OPEN_SCOPE

/// Converts Tf diagnostics into native Maya infos, warnings, and errors.
///
/// Provides an optional batching mechanism for diagnostics; see
/// UsdMayaDiagnosticBatchContext for more information. Note that errors
/// are never batched.
///
/// The IssueError(), IssueStatus(), etc. functions are thread-safe, since Tf
/// may issue diagnostics from secondary threads. Note that, when not batching,
/// secondary threads' diagnostic messages are posted to stderr instead of to
/// the Maya script window. When batching, secondary threads' diagnostic
/// messages will be posted by the main thread to the Maya script window when
/// batching ends.
///
/// Installing and removing this diagnostic delegate is not thread-safe, and
/// must be done only on the main thread.
class UsdMayaDiagnosticDelegate
{
public:
    /// Installs a shared delegate globally.
    /// If this is invoked on a secondary thread, issues a fatal coding error.
    MAYAUSD_CORE_PUBLIC
    static void InstallDelegate();
    /// Removes the global shared delegate, if it exists.
    /// If this is invoked on a secondary thread, issues a fatal coding error.
    MAYAUSD_CORE_PUBLIC
    static void RemoveDelegate();

    /// @brief Write all accumulated diagnostic messages.
    MAYAUSD_CORE_PUBLIC
    static void Flush();

    /// @brief Sets the maximum number of diagnostics messages that can be emitted in
    ///        one second before we start to batch messages. Default is 100.
    MAYAUSD_CORE_PUBLIC
    static void SetMaximumUnbatchedDiagnostics(int count);

    /// @brief Gets the maximum number of diagnostics messages that can be emitted in
    ///        one second before we start to batch messages. Default is 100.
    MAYAUSD_CORE_PUBLIC
    static int GetMaximumUnbatchedDiagnostics();
};

/// As long as a batch context remains alive (process-wide), the
/// UsdMayaDiagnosticDelegate will save diagnostic messages that exceed the given
/// maximum count, which defaults to 0.
///
/// The messages will be emmited when the last batch context is destructed.
///
/// Batch contexts can be constructed and destructed out of "scope" order, e.g.,
/// this is allowed:
///   1. Context A constructed
///   2. Context B constructed
///   3. Context A destructed
///   4. Context B destructed
class UsdMayaDiagnosticBatchContext
{
public:
    /// Constructs a batch context, causing all subsequent diagnostic messages
    /// to be batched on all threads.
    /// If this is invoked on a secondary thread, issues a fatal coding error.
    MAYAUSD_CORE_PUBLIC
    UsdMayaDiagnosticBatchContext(int maximumUnbatchedCount = 0);
    MAYAUSD_CORE_PUBLIC
    ~UsdMayaDiagnosticBatchContext();

    UsdMayaDiagnosticBatchContext(const UsdMayaDiagnosticBatchContext&) = delete;
    UsdMayaDiagnosticBatchContext& operator=(const UsdMayaDiagnosticBatchContext&) = delete;

private:
    int previousCount = 0;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
