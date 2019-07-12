// ===========================================================================
// Copyright 2019 Autodesk, Inc. All rights reserved.
//
// Use of this software is subject to the terms of the Autodesk license
// agreement provided at the time of installation or download, or which
// otherwise accompanies this software in either electronic or hard copy form.
// ===========================================================================
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
