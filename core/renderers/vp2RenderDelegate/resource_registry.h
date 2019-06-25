#ifndef HD_VP2_RESOURCE_REGISTRY
#define HD_VP2_RESOURCE_REGISTRY

// ===========================================================================
// Copyright 2019 Autodesk, Inc. All rights reserved.
//
// Use of this software is subject to the terms of the Autodesk license
// agreement provided at the time of installation or download, or which
// otherwise accompanies this software in either electronic or hard copy form.
// ===========================================================================


#include "task_commit.h"

#include <tbb/concurrent_queue.h>
#include <tbb/tbb_allocator.h>

PXR_NAMESPACE_OPEN_SCOPE

/*! \brief  Central place to manage GPU resources commits and any resources not managed by VP2 directly
    \class  HdVP2ResourceRegistry
*/
class HdVP2ResourceRegistry
{
public:
    //! \brief  Default constructor
    HdVP2ResourceRegistry() = default;
    //! \brief  Default destructor
    ~HdVP2ResourceRegistry() = default;

    //! \brief  Execute commit tasks (called by render delegate)
    void Commit() {
        HdVP2TaskCommit* commitTask;
        while (_commitTasks.try_pop(commitTask)) {
            (*commitTask)();
            commitTask->destroy();
        }
    }
    
    //! \brief  Enqueue commit task. Call is thread safe.
    template<typename Body>
    void EnqueueCommit(Body taskBody) {
        _commitTasks.push(HdVP2TaskCommitBody<Body>::construct(taskBody));
    }
private:
    //! Concurrent queue for commit tasks
    tbb::concurrent_queue<HdVP2TaskCommit*, tbb::tbb_allocator<HdVP2TaskCommit*>> _commitTasks;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif