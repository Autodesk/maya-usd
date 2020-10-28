//
// Copyright 2020 Autodesk
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
#ifndef MAYA_USD_LAYER_MANAGER
#define MAYA_USD_LAYER_MANAGER

#include <mayaUsd/base/api.h>

#include "pxr/usd/sdf/types.h"
#include "pxr/usd/sdf/layer.h"

#include <maya/MMessage.h>
#include <maya/MString.h>
#include <maya/MTypeId.h>
#include <maya/MObject.h>
#include <maya/MPxNode.h>

#include <string>

PXR_NAMESPACE_USING_DIRECTIVE

namespace MAYAUSD_NS_DEF {

    /*! \brief Maya dependency node responsible for serializing unsaved Usd edits to a Maya file.

        In a pre-save callback, if there are any Usd layers with unsaved edits, a single LayerManager node 
        will be created that stores the Usd identifiers of all layers under the parent Proxy Shape as well 
        as the dirty Usd layer itself exported to a string.  Dirty layers will include any anonymous layers, 
        a Session layer with edits, and any file-backed Usd layers with edits that have not been saved to disk.

        In a post-read callback, the LayerManager will recreate the Usd layers by importing the data saved as a string 
        attribute on the node, and will update all sub layer paths to use the identifier of the newly created layers.

        \note The LayerManager will only consider Usd stages that exist under a support Proxy Shape class 
        derived from MayaUsdProxyShapeBase and have requested the support by adding the shapes MTypeId by calling 
        LayerManager::addSupportForNodeType(MTypeId).
    */
class MAYAUSD_CORE_PUBLIC LayerManager
  : public MPxNode
{
public:
    /*! \brief  
    */

    /*! \brief  Called by any MayaUsdProxyShapeBase derived class that wants to be included in the LayerManager
        serialization of Usd edits.  
        \param nodeId The MTypeId of the derived Proxy Shape node that should be supported.
    */
    static void addSupportForNodeType(MTypeId nodeId);
    static void removeSupportForNodeType(MTypeId nodeId);

    /*! \brief  Supported Proxy Shapes should call this to possibly retrieve their Root and Session layers before
        calling Sdf::FindOrOpen.  If a handle is found and returned then it will be the recreated layer, and all
        sublayers, with edits from a previous Maya session and should be used to initialize the Proxy Shape in
        a call to UsdStage::Open().
    */
    static SdfLayerHandle findLayer(std::string identifier);

    static const MString typeName;
    static const MTypeId typeId;

    static void* creator(); 
    static MStatus initialize();

    static MObject layers;
    static MObject identifier;
    static MObject serialized;
    static MObject anonymous;

protected:
    LayerManager();
    ~LayerManager() override;

}; // LayerManager

} // MAYAUSD_NS_DEF

#endif // MAYA_USD_LAYER_MANAGER
