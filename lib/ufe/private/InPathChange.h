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
#pragma once

#include "../../base/api.h"

MAYAUSD_NS_DEF {
namespace ufe {

//! \brief Helper class to scope when we are in a path change operation.
class InPathChange
{
public:
	InPathChange() { fInPathChange = true; }
	~InPathChange() { fInPathChange = false; }

	// Delete the copy/move constructors assignment operators.
	InPathChange(const InPathChange&) = delete;
	InPathChange& operator=(const InPathChange&) = delete;
	InPathChange(InPathChange&&) = delete;
	InPathChange& operator=(InPathChange&&) = delete;

	static bool inPathChange() { return fInPathChange; }

private:
	static bool fInPathChange;
};

} // namespace ufe
} // namespace MayaUsd
