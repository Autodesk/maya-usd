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
#ifndef MAYAUSD_PROGRSSBARSCOPE_H
#define MAYAUSD_PROGRSSBARSCOPE_H

#include <mayaUsd/base/api.h>

#include <maya/MComputation.h>
#include <maya/MString.h>

#include <memory>

namespace MAYAUSD_NS_DEF {

//! \brief Helper class for displaying a progress bar during an operation.
/*!
    This class is a helper class that can be used to show a progress bar for
    a given operation. When starting a long operation (such as import USD file)
    one would use of the first two constructors to create the top-level scope
    which will create the internal MComputation (for displaying progress bar).
    Then in any methods called from this top-level you use the third constructor
    to add steps to the existing scope.
*/
class MAYAUSD_CORE_PUBLIC ProgressBarScope
{
public:
    // Constructors which will create the internal MComputation if not already
    // exists. They are meant to be used at a top-level for starting an operation,
    // such as import USD file.
    ProgressBarScope(const int nbSteps, const MString& progressStr);
    ProgressBarScope(
        bool           showProgress,
        bool           interruptible,
        const int      nbSteps,
        const MString& progressStr);

    // Constructor which uses a previously created MComputation. If not exists
    // then this scope will do nothing. Meant to be used by method called from a
    // top-level operation (above two constructors).
    ProgressBarScope(const int nbSteps);

    ~ProgressBarScope();

    // Delete the copy/move constructors assignment operators.
    ProgressBarScope(const ProgressBarScope&) = delete;
    ProgressBarScope& operator=(const ProgressBarScope&) = delete;
    ProgressBarScope(ProgressBarScope&&) = delete;
    ProgressBarScope& operator=(ProgressBarScope&&) = delete;

    // Add the input number of steps to the current range.
    void addSteps(int nbSteps);

    void setProgressString(const MString&);

    // Advance the current progress by n step(s).
    void advance(const int steps = 1);

    bool isInterruptRequested() const;

private:
    bool _created { false };
    int  _nbSteps { 0 };

    // There can be only one progress bar during a given operation.
    static std::unique_ptr<MComputation> progBar;

    // Keep track of the total number of steps added so at the end we can
    // verify that the progress bar was advanced that many steps.
    static int totalStepsAdded;
};

//! \brief Helper class to add steps for a loop to a progress bar.
/*!
    At the start of a loop creating a stack variable of this type will add a
    given number of steps to the progress bar. Internally this class will
    limit that number to a max so as to not overwhelm the process with updates
    to the progress bar.

    The loopAdvance() method internally knows how many loop iterations need
    to be performed before advancing the progress bar.
*/
class MAYAUSD_CORE_PUBLIC ProgressBarLoopScope : public ProgressBarScope
{
public:
    ProgressBarLoopScope(const int nbLoopSteps);

    // Delete the copy/move constructors assignment operators.
    ProgressBarLoopScope(const ProgressBarLoopScope&) = delete;
    ProgressBarLoopScope& operator=(const ProgressBarLoopScope&) = delete;
    ProgressBarLoopScope(ProgressBarLoopScope&&) = delete;
    ProgressBarLoopScope& operator=(ProgressBarLoopScope&&) = delete;

    // Advance the current progress of the loop by 1 step if we have
    // run thru the required number of loop iterations.
    void loopAdvance();

private:
    // Add a variable number of steps for a new for-loop.
    // The number of steps will be capped at a known value (maxStepsForLoops)
    // so we don't overwhelm the action with progress bar updates.
    void addLoopSteps(int loopSize);

    // The current loop iteration counter.
    int _currLoopCounter { 0 };

    // Number of loop iterations to perform before advancing progress bar.
    int _minProgressStep { 0 };

    // Just like Maya don't add to many steps as the progress bar update
    // will overwhelm the process. So for a loop we'll limit the number of
    // steps added to this value.
    static const int maxStepsForLoops { 20 };
};

} // namespace MAYAUSD_NS_DEF

#endif
