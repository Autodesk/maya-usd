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

#include <pxr/base/tf/notice.h>
#include <pxr/pxr.h>
#include <pxr/usd/sdf/layer.h>
#include <pxr/usd/sdf/types.h>
#include <pxr/usd/usd/prim.h>
#include <pxr/usd/usd/stage.h>

#include <maya/MDagPath.h>
#include <maya/MDagPathArray.h>
#include <maya/MMessage.h>
#include <maya/MObject.h>
#include <maya/MPxNode.h>
#include <maya/MString.h>
#include <maya/MTypeId.h>

#include <functional>
#include <string>
#include <vector>

PXR_NAMESPACE_USING_DIRECTIVE

namespace MAYAUSD_NS_DEF {

enum BatchSaveResult
{
    kAbort,             // User has chosen to abort the file operation.
    kNotHandled,        // Callback did not handle any of the stages passed to it.
    kCompleted,         // Callback handled all stages.  Layer Manager should not continue
                        // to process anything.
    kPartiallyCompleted // Callback has handled the saving of some stages, but not all. Layer
                        // Manager should continue to look for unsaved stages.
};

/*! \brief Information about the stages that need to be saved.
 */
struct LayerSavingInfo
{
    MDagPath    dagPath;
    UsdStagePtr stage;
    bool        shareable = true;
    bool        isIncoming = false;
};

/*! \brief Callback function to handle saving of Usd edits.  In a default build of the
    plugin a delegate will be installed that posts a UI dialog that provides an opportunity
    to choose file names and locations of all anonymous layers that need to be saved to disk.
 */
using BatchSaveDelegate = std::function<BatchSaveResult(const std::vector<LayerSavingInfo>&)>;

/*! \brief Maya dependency node responsible for serializing unsaved Usd edits.

    In a pre-save or export callback, Maya will check if there are any proxy shapes of a supported
    type that have UsdStages with dirty Layers in them.  If stages are found, there are currently
    three options for how to handle the Usd edits:

    1. Save back to .usd files.
    There are three steps to handling this option.
    a) All anonymous layers must be saved to disk.  If a batch save delegate has been installed,
    and Maya is running in interactive mode, then a UI dialog can be displayed to provide a choice
    of file names and locations for all anonymous layers.  If Maya is not running in interactive
    mode, or there is no installed delegate to handle it, then Maya will automatically choose name
    and locations for all anonymous layers.
    b) All file-backed Usd layers will be saved.
    c) The Session Layer, if dirty, is handled as a special case.  It will be exported to a string
    and saved into the Maya file as an attribute on the LayerManager node.  When reading back in
    this file the Session Layer will be restored when recreating the Usd Stage.

    2. Save into the Maya file.
    With this option, if there are any Usd layers with unsaved edits, a single LayerManager
    node will be created that stores the Usd identifiers of all layers under the parent Proxy Shape
    as well as the dirty Usd layer itself exported to a string.  Dirty layers will include any
    anonymous layers, a Session layer with edits, and any file-backed Usd layers with edits that
   have not been saved to disk.

    3. Ignore all Usd edits.
    With this option, Maya will not attempt to save any dirty Usd layers, assuming the user is
    explicitly managing the state themselves.

    \note The LayerManager will only consider Usd stages that exist under a supported Proxy Shape
    class derived from MayaUsdProxyShapeBase and that has requested the support by adding the
    shapes MTypeId by calling LayerManager::addSupportForNodeType(MTypeId).
*/
class MAYAUSD_CORE_PUBLIC LayerManager : public MPxNode
{
public:
    /*! \brief Set a callback function to handle saving of Usd edits.  In a default build of the
        plugin a delegate will be installed that posts a UI dialog that provides an opportunity
        to choose file names and locations of all anonymous layers that need to be saved to disk.
     */
    static void SetBatchSaveDelegate(BatchSaveDelegate delegate);

    /*! \brief  Called by any MayaUsdProxyShapeBase derived class that wants to be included in the
       LayerManager serialization of Usd edits. \param nodeId The MTypeId of the derived Proxy Shape
       node that should be supported.
    */
    static void addSupportForNodeType(MTypeId nodeId);
    static void removeSupportForNodeType(MTypeId nodeId);
    static bool supportedNodeType(MTypeId nodeId);

    /*! \brief  Supported Proxy Shapes should call this to possibly retrieve their Root and Session
       layers before calling Sdf::FindOrOpen.  If a handle is found and returned then it will be the
       recreated layer, and all sublayers, with edits from a previous Maya session and should be
       used to initialize the Proxy Shape in a call to UsdStage::Open().
    */
    static SdfLayerHandle findLayer(std::string identifier);

    static const MString typeName;
    static const MTypeId typeId;

    static void*   creator();
    static MStatus initialize();

    static MObject layers;
    static MObject identifier;
    static MObject fileFormatId;
    static MObject serialized;
    static MObject anonymous;

protected:
    LayerManager();
    ~LayerManager() override;
}; // LayerManager

} // namespace MAYAUSD_NS_DEF

#endif // MAYA_USD_LAYER_MANAGER
