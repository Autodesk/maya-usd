//
// Copyright 2019 Autodesk
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
#ifndef HD_VP2_RESOURCE_REGISTRY
#define HD_VP2_RESOURCE_REGISTRY

#include "task_commit.h"

#include <tbb/concurrent_queue.h>
#include <tbb/tbb_allocator.h>

PXR_NAMESPACE_OPEN_SCOPE

/*! \brief  Central place to manage GPU resources commits and any resources not managed by VP2
   directly \class  HdVP2ResourceRegistry
*/
class HdVP2ResourceRegistry
{
public:
    //! \brief  Default constructor
    HdVP2ResourceRegistry() = default;
    //! \brief  Default destructor
    ~HdVP2ResourceRegistry() = default;

    //! \brief  Execute commit tasks (called by render delegate)
    void Commit()
    {
        HdVP2TaskCommit* commitTask;
        while (_commitTasks.try_pop(commitTask)) {
            (*commitTask)();
            commitTask->destroy();
        }
    }

    //! \brief  Enqueue commit task. Call is thread safe.
    template <typename Body> void EnqueueCommit(Body taskBody)
    {
        _commitTasks.push(HdVP2TaskCommitBody<Body>::construct(taskBody));
    }

private:
    //! Concurrent queue for commit tasks
    tbb::concurrent_queue<HdVP2TaskCommit*, tbb::tbb_allocator<HdVP2TaskCommit*>> _commitTasks;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif