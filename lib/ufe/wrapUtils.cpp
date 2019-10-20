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

#include "ufe/runTimeMgr.h"
#include "ufe/rtid.h"

using namespace MayaUsd;
using namespace boost::python;

UsdPrim getPrimFromRawItem(uint64_t rawItem)
{
    Ufe::SceneItem* item = reinterpret_cast<Ufe::SceneItem*>(rawItem);
    ufe::UsdSceneItem* usdItem = dynamic_cast<ufe::UsdSceneItem*>(item);
    if (nullptr != usdItem)
    {
        return usdItem->prim();
    }
    return UsdPrim();
}

#ifdef UFE_V2_FEATURES_AVAILABLE
std::string getNodeNameFromRawItem(uint64_t rawItem)
{
    std::string name;
    Ufe::SceneItem* item = reinterpret_cast<Ufe::SceneItem*>(rawItem);
    if (nullptr != item)
        name = item->nodeName();
    return name;
}
#endif

std::string getNodeTypeFromRawItem(uint64_t rawItem)
{
    std::string type;
    Ufe::SceneItem* item = reinterpret_cast<Ufe::SceneItem*>(rawItem);
    if (nullptr != item)
    {
        // Prepend the name of the runtime manager of this item to the type.
        type = Ufe::RunTimeMgr::instance().getName(item->runTimeId()) + item->nodeType();
    }
    return type;
}

BOOST_PYTHON_MODULE(ufe)
{
    def("getPrimFromRawItem", getPrimFromRawItem);
    
    #ifdef UFE_V2_FEATURES_AVAILABLE
        def("getNodeNameFromRawItem", getNodeNameFromRawItem);
    #endif

    def("getNodeTypeFromRawItem", getNodeTypeFromRawItem);
}
