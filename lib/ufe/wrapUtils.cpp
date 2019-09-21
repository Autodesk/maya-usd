// ===========================================================================
// Copyright 2019 Autodesk, Inc. All rights reserved.
//
// Use of this software is subject to the terms of the Autodesk license
// agreement provided at the time of installation or download, or which
// otherwise accompanies this software in either electronic or hard copy form.
// ===========================================================================

#include <boost/python.hpp>

#include "UsdSceneItem.h"
#include "Utils.h"

using namespace MayaUsd;
using namespace boost::python;

UsdPrim ufePathToPrim(Ufe::Rtid id1, const std::string& sep1, const std::string& seg1, Ufe::Rtid id2, const std::string& sep2, const std::string& seg2)
{
	Ufe::PathSegment ps1(seg1, id1, sep1[0]);
	Ufe::PathSegment ps2(seg2, id2, sep2[0]);
	Ufe::Path::Segments segs;
	segs.push_back(ps1);
	segs.push_back(ps2);
	Ufe::Path path(segs);
	return ufe::ufePathToPrim(path);
}

BOOST_PYTHON_MODULE(ufe)
{
	def("ufePathToPrim", ufePathToPrim);
}
