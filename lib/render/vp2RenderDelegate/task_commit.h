#ifndef HD_VP2_TASK_COMMIT
#define HD_VP2_TASK_COMMIT

// ===========================================================================
// Copyright 2019 Autodesk, Inc. All rights reserved.
//
// Use of this software is subject to the terms of the Autodesk license
// agreement provided at the time of installation or download, or which
// otherwise accompanies this software in either electronic or hard copy form.
// ===========================================================================

#include <tbb/tbb_allocator.h>

PXR_NAMESPACE_OPEN_SCOPE

/*! \brief  Base commit task class
    \class  HdVP2TaskCommit
*/
class HdVP2TaskCommit
{
public:
    virtual ~HdVP2TaskCommit() = default;

    //! Execute the task
    virtual void operator()() = 0;

    //! Destroy & deallocated this task
    virtual void destroy() = 0;
};

/*! \brief  Wrapper of a task body into commit task.
    \class  HdVP2TaskCommit
*/
template <typename Body>
class HdVP2TaskCommitBody final : public HdVP2TaskCommit {
    //! Use scalable allocator to prevent heap contention
    using my_allocator_type = tbb::tbb_allocator<HdVP2TaskCommitBody<Body>>;

    //! Private constructor to force usage of construct method & allocation with 
    //! scalable allocator.
    HdVP2TaskCommitBody(const Body& body) : fBody(body) {}

public:
    ~HdVP2TaskCommitBody() override = default;

    //! Execute body task.
    void operator()() override {
        fBody();
    }

    //! Objects of this type are allocated with tbb_allocator.
    //! Release the memory using same allocator by calling destroy method.
    void destroy() override {
        my_allocator_type().destroy(this);
        my_allocator_type().deallocate(this, 1);
    }

    /*! Allocate a new object of type Body using scalable allocator.
        Always free this object by calling destroy method!
    */
    static HdVP2TaskCommitBody<Body>* construct(const Body& body) {
        void* mem = my_allocator_type().allocate(1);
        return new (mem) HdVP2TaskCommitBody<Body>(body);
    }

private:
    Body fBody; //!< Function object providing execution "body" for this task
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif