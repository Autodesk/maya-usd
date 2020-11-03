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
#ifndef UFENOTIFGUARD_H
#define UFENOTIFGUARD_H

#include <mayaUsd/base/api.h>

#include <ufe/ufe.h>

UFE_NS_DEF {
class Path;
}

namespace MAYAUSD_NS_DEF {
namespace ufe {

//! \brief Helper class to scope when we are in a path change operation.
//
// This simple guard class can be used within a single scope, but does not have
// recursive scope capability.
class InPathChange
{
public:
	InPathChange() { inGuard = true; }
	~InPathChange() { inGuard = false; }

	// Delete the copy/move constructors assignment operators.
	InPathChange(const InPathChange&) = delete;
	InPathChange& operator=(const InPathChange&) = delete;
	InPathChange(InPathChange&&) = delete;
	InPathChange& operator=(InPathChange&&) = delete;

	static bool inPathChange() { return inGuard; }

private:
	static bool inGuard;
};

//! \brief Helper class to scope when we are in an add or delete operation.
//
// This simple guard class can be used within a single scope, but does not have
// recursive scope capability.
class InAddOrDeleteOperation
{
public:
	InAddOrDeleteOperation() { inGuard = true; }
	~InAddOrDeleteOperation() { inGuard = false; }

	// Delete the copy/move constructors assignment operators.
	InAddOrDeleteOperation(const InAddOrDeleteOperation&) = delete;
	InAddOrDeleteOperation& operator=(const InAddOrDeleteOperation&) = delete;
	InAddOrDeleteOperation(InAddOrDeleteOperation&&) = delete;
	InAddOrDeleteOperation& operator=(InAddOrDeleteOperation&&) = delete;

	static bool inAddOrDeleteOperation() { return inGuard; }

private:
	static bool inGuard;
};


//! \brief Helper class to scope when we are in a Transform3d change operation.
//
// This simple guard class can be used within a single scope, but does not have
// recursive scope capability.  On guard exit, will send a Transform3d
// notification.
class InTransform3dChange
{
public:
	InTransform3dChange(const Ufe::Path& path);
	~InTransform3dChange();

	// Delete the copy/move constructors assignment operators.
	InTransform3dChange(const InTransform3dChange&) = delete;
	InTransform3dChange& operator=(const InTransform3dChange&) = delete;
	InTransform3dChange(InTransform3dChange&&) = delete;
	InTransform3dChange& operator=(InTransform3dChange&&) = delete;

	static bool inTransform3dChange();
};

} // namespace ufe
} // namespace MayaUsd

#endif // UFENOTIFGUARD_H
