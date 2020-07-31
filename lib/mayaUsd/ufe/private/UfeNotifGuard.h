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

MAYAUSD_NS_DEF {
namespace ufe {

//! \brief Helper class to scope when we are in a path change operation.
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

//! \brief Helper class to scope when we are in an add or remove reference operation.
class InAddOrRemoveReference
{
public:
	InAddOrRemoveReference() { inGuard = true; }
	~InAddOrRemoveReference() { inGuard = false; }

	// Delete the copy/move constructors assignment operators.
	InAddOrRemoveReference(const InAddOrRemoveReference&) = delete;
	InAddOrRemoveReference& operator=(const InAddOrRemoveReference&) = delete;
	InAddOrRemoveReference(InAddOrRemoveReference&&) = delete;
	InAddOrRemoveReference& operator=(InAddOrRemoveReference&&) = delete;

	static bool inAddOrRemoveReference() { return inGuard; }

private:
	static bool inGuard;
};


} // namespace ufe
} // namespace MayaUsd

#endif // UFENOTIFGUARD_H
