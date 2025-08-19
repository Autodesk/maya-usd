//
// Copyright 2022 Autodesk
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

#include "progressBarScope.h"

#include <pxr/base/tf/diagnostic.h>
#include <pxr/base/tf/getenv.h>

#include <maya/MGlobal.h>

#include <cassert>

// For TF_WARN macro
PXR_NAMESPACE_USING_DIRECTIVE

namespace MAYAUSD_NS_DEF {

MAYAUSD_VERIFY_CLASS_NOT_MOVE_OR_COPY(ProgressBarScope);

std::unique_ptr<MComputation> ProgressBarScope::progBar;
int                           ProgressBarScope::totalStepsAdded = 0;

// Create a scope with default values for showProgress and interruptible.
ProgressBarScope::ProgressBarScope(const int nbSteps, const MString& progressStr)
    : ProgressBarScope(true, false, nbSteps, progressStr)
{
}

ProgressBarScope::ProgressBarScope(
    bool           showProgress,
    bool           interruptible,
    const int      nbSteps,
    const MString& progressStr)
{
    // Allow user (or QA) to disable the display of the progress bar.
    // Default display value is ON.
    bool createProgressBar = TfGetenvBool("MAYAUSD_ENABLE_PROGRESSBAR", true);

    // If this is the first scope being created for an operation we need
    // to create the MComputation which we use for the progress bar.
    if (createProgressBar && (progBar.get() == nullptr)) {
        progBar = std::make_unique<MComputation>();
        _created = true;
        progBar->beginComputation(showProgress, interruptible);
        setProgressString(progressStr);

        // In the usage from most cases, we do NOT know how many steps
        // there will be for any given task. So we start with a normal range
        // and can add extra steps along the way.
        progBar->setProgressRange(0, 100);
        totalStepsAdded = 0;
    }
    addSteps(nbSteps);
}

ProgressBarScope::ProgressBarScope(const int nbSteps)
{
    // Uses a previously created MComputation to add steps to.
    // If the MComputation (our progBar) is nullptr, then the steps added
    // don't do anything.
    addSteps(nbSteps);
}

ProgressBarScope::~ProgressBarScope()
{
    // Make sure to advance by all the steps for this scope.
    advance(_nbSteps);

    // If we created the MComputation we end and delete it.
    if (_created) {
        // Verify that we advances the number of steps added.
        const int progress = progBar->progress();

        // `progress == -1` means the query failed.
        //
        // The "did not advance" warning below is not necessarily relevant -- we
        // may have advanced the correct number of steps, but the `progBar`
        // failed for other reasons, for example, running without the UI.
        if (progress != -1 && progress != totalStepsAdded
            && MGlobal::mayaState() == MGlobal::kInteractive) {
            TF_WARN("ProgressBarScope: did not advance progress bar correct number of steps.");
        }
        totalStepsAdded = 0;

        progBar->setProgress(progBar->progressMax());
        progBar->endComputation();
        progBar.release();
    }
}

void ProgressBarScope::addSteps(int nbSteps)
{
    if ((nbSteps > 0) && (progBar != nullptr)) {
        // If the current computation doesn't have at least nbSteps left
        // adjust the range by adding in the new number of steps.
        const auto progMax = progBar->progressMax();
        if ((totalStepsAdded + nbSteps) > progMax) {
            progBar->setProgressRange(progBar->progressMin(), totalStepsAdded + nbSteps);
        }
        totalStepsAdded += nbSteps;
        _nbSteps += nbSteps;
    }
}

void ProgressBarScope::setProgressString(const MString& progressStr)
{
    if (progBar != nullptr) {
        progBar->setProgressStatus(progressStr);
    }
}

void ProgressBarScope::advance(const int steps /*= 1*/)
{
    if ((steps != 0) && (progBar != nullptr)) {
        progBar->setProgress(progBar->progress() + steps);
        _nbSteps -= steps;
    }
}

bool ProgressBarScope::isInterruptRequested() const
{
    if (progBar != nullptr) {
        return progBar->isInterruptRequested();
    }
    return false;
}

MAYAUSD_VERIFY_CLASS_NOT_MOVE_OR_COPY(ProgressBarLoopScope);

const int ProgressBarLoopScope::maxStepsForLoops;

ProgressBarLoopScope::ProgressBarLoopScope(int nbLoopSteps)
    : ProgressBarScope(0) // Start with adding 0 steps
    , _nbLoopSteps(std::max(0, nbLoopSteps))
    , _nbProgressSteps(std::min(std::max(0, nbLoopSteps), maxStepsForLoops))
{
    // We add the real number of steps for the loop here.
    // The number of steps is capped at a known value (maxStepsForLoops)
    // so we don't overwhelm the action with progress bar updates.
    addSteps(_nbProgressSteps);
}

void ProgressBarLoopScope::loopAdvance()
{
    assert(_nbLoopSteps > 0);

    // _nbProgressSteps might have been capped at a known value (maxStepsForLoops).
    // If we have run thru the loop the required number of steps we
    // will advance the progress bar.
    _remainder += _nbProgressSteps;
    if (_remainder >= _nbLoopSteps) {
        advance(1);
        _remainder -= _nbLoopSteps;
    }
}

} // namespace MAYAUSD_NS_DEF
