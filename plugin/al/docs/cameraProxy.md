# Camera Proxy

## Overview
This Node is designed to be a connection between a Maya camera and a USD Camera to allow interactive edits to the USD cameras attributes.
The proxy node reads and writes directly to the USD camera attributes to avoid duplicate storage of thes attributes in both Maya and USD.
When a proxy node is created and connected to a Maya camera, this node can be used to drive the Maya camera and directly edit the USD attributes simultaneously.

## Usage
This node must be created and configured through MEL. Once created and connected to a Maya camera the node is intended to replace the attributes on the Maya camera attribute editor.

### Creating the Node
The first step is to create the node and attach it to a USD stage proxy.

    string $stageProxy = "AL_usdmaya_Proxy"; // <- for example, this should be the name of the stage proxy shape.
    string $cameraProxy = `createNode AL_usd_ProxyUsdGeomCamera`;
    connectAttr ($stageProxy + ".outStageData") ($cameraProxy + ".stage");
    select -r $cameraNode;

## Connecting the Node to a Camera
After the node is created it needs to be connected to an existing Maya camera node. It also needs to have specified the path to a `UsdGeomCamera` contained in the stage.

    string $usdCameraPrimPath = "/viewports/interactive_cam"; // <- for example, this needs to be defined in USD first.
    string $mayaCamera = "perspShape";
    setAttr ($cameraProxy + ".path") $usdCameraPrimPath;
    connectAttr -force -lock true ($cameraProxy + ".focalLength")            ($mayaCamera + ".focalLength");
    connectAttr -force -lock true ($cameraProxy + ".nearClipPlane")          ($mayaCamera + ".nearClipPlane");
    connectAttr -force -lock true ($cameraProxy + ".farClipPlane")           ($mayaCamera + ".farClipPlane");
    connectAttr -force -lock true ($cameraProxy + ".orthographic")           ($mayaCamera + ".orthographic");
    connectAttr -force -lock true ($cameraProxy + ".horizontalFilmAperture") ($mayaCamera + ".horizontalFilmAperture");
    connectAttr -force -lock true ($cameraProxy + ".horizontalFilmOffset")   ($mayaCamera + ".horizontalFilmOffset");
    connectAttr -force -lock true ($cameraProxy + ".verticalFilmAperture")   ($mayaCamera + ".verticalFilmAperture");
    connectAttr -force -lock true ($cameraProxy + ".verticalFilmOffset")     ($mayaCamera + ".verticalFilmOffset");
    connectAttr -force -lock true ($cameraProxy + ".focusDistance")          ($mayaCamera + ".focusDistance");
    connectAttr -force -lock true ($cameraProxy + ".fStop")                  ($mayaCamera + ".fStop");

In this example the perspective viewport camera will be connected to a camera in the USD scene and any changes made to attributes on the camera proxy will both: write directly to the USD camera attributes, and update the Maya camera viewport attributes.
